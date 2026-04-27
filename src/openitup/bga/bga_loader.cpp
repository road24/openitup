#include <openitup/bga/bga_loader.h>
#include <openitup/sprite/sprite_loader.h>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <vector>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace openitup {

namespace fs = std::filesystem;

static std::string to_lower(const std::string& s) {
    std::string r = s;
    std::transform(r.begin(), r.end(), r.begin(),
        [](unsigned char c) { return std::tolower(c); });
    return r;
}

static bool is_image_ext(const std::string& ext_lower) {
    return ext_lower == ".tga" || ext_lower == ".png" || ext_lower == ".dds";
}

static bool is_sprite_ext(const std::string& ext_lower) {
    return ext_lower == ".spr" || ext_lower == ".sp2" || ext_lower == ".sprj";
}

// Case-insensitive file search: find a file in base_dir whose stem matches
// (case-insensitive) and has one of the given extensions.
static fs::path find_file_ci(const std::string& name,
                              const fs::path& base_dir) {
    // Fast path: exact match
    auto exact = base_dir / name;
    if (fs::exists(exact)) return exact;

    // Scan directory case-insensitively
    auto name_lower = to_lower(fs::path(name).stem().string());
    auto ext_lower = to_lower(fs::path(name).extension().string());

    for (const auto& entry : fs::directory_iterator(base_dir)) {
        if (!entry.is_regular_file()) continue;
        if (to_lower(entry.path().stem().string()) != name_lower) continue;
        if (to_lower(entry.path().extension().string()) == ext_lower) {
            return entry.path();
        }
    }

    return {};
}

static std::unique_ptr<Sprite> make_raw_texture_sprite(
        const std::string& texture_name,
        const fs::path& base_dir,
        TextureCache& cache) {
    auto result = cache.load(texture_name, base_dir);

    auto sprite = std::make_unique<Sprite>();
    sprite->source_format = "raw";
    sprite->mode = SpriteMode::Tile;

    Picture pic;
    pic.texture_name = texture_name;
    pic.texture = result.handle;
    pic.rect = {0, 0, result.width, result.height};
    pic.uv = {0.0f, 0.0f, 1.0f, 1.0f};
    sprite->pictures.push_back(std::move(pic));

    return sprite;
}

static fs::path resolve_sprite_path(const std::string& sprite_name,
                                     const fs::path& base_dir) {
    auto ext_lower = to_lower(fs::path(sprite_name).extension().string());
    auto stem = fs::path(sprite_name).stem().string();

    if (ext_lower == ".sprj") {
        auto found = find_file_ci(sprite_name, base_dir);
        if (!found.empty()) return found;
        spdlog::error("sprite file not found: '{}' in {}", sprite_name, base_dir.string());
        throw std::runtime_error("Sprite file not found: " + sprite_name +
                                 " in " + base_dir.string());
    }

    if (ext_lower == ".spr" || ext_lower == ".sp2") {
        auto sprj_name = stem + ".sprj";
        auto found = find_file_ci(sprj_name, base_dir);
        if (!found.empty()) return found;
        spdlog::error("no .sprj found for '{}' in {} (need to convert with spr2sprj first)",
                      sprite_name, base_dir.string());
        throw std::runtime_error(
            "No .sprj found for '" + sprite_name + "' in " + base_dir.string());
    }

    spdlog::error("unsupported sprite extension: '{}'", sprite_name);
    throw std::runtime_error("Unsupported sprite extension: " + sprite_name);
}

std::unique_ptr<BgaAnimation> load_bgaj(const fs::path& path,
                                         TextureCache& cache) {
    spdlog::info("loading bgaj: {}", path.string());

    std::ifstream file(path);
    if (!file.is_open()) {
        spdlog::error("failed to open bgaj file: {}", path.string());
        throw std::runtime_error("Failed to open BGA file: " + path.string());
    }

    nlohmann::json j = nlohmann::json::parse(file);

    int version = j.at("version").get<int>();
    if (version != 2) {
        spdlog::error("unsupported BGA version {} in {}", version, path.string());
        throw std::runtime_error("Unsupported BGA version: " + std::to_string(version));
    }

    auto animation = std::make_unique<BgaAnimation>();
    auto base_dir = path.parent_path();
    int layer_idx = 0;

    for (const auto& layer_json : j.at("layers")) {
        Layer layer;
        layer.sprite_name = layer_json.at("sprite").get<std::string>();

        for (const auto& kf_json : layer_json.at("keyframes")) {
            Keyframe kf;
            kf.from_json(kf_json);
            layer.keyframes.push_back(std::move(kf));
        }

        if (!layer.sprite_name.empty()) {
            auto ext_lower = to_lower(
                fs::path(layer.sprite_name).extension().string());

            std::unique_ptr<Sprite> sprite;
            if (is_image_ext(ext_lower)) {
                spdlog::info("layer {}: raw texture '{}'", layer_idx, layer.sprite_name);
                sprite = make_raw_texture_sprite(layer.sprite_name, base_dir, cache);
            } else if (is_sprite_ext(ext_lower)) {
                spdlog::info("layer {}: sprite '{}'", layer_idx, layer.sprite_name);
                auto sprite_path = resolve_sprite_path(layer.sprite_name, base_dir);
                sprite = load_sprj(sprite_path, cache);
            } else {
                spdlog::error("layer {}: unsupported sprite extension '{}'",
                              layer_idx, layer.sprite_name);
                throw std::runtime_error(
                    "Unsupported sprite extension: " + layer.sprite_name);
            }

            layer.sprite = sprite.get();
            animation->owned_sprites.push_back(std::move(sprite));
        } else {
            spdlog::debug("layer {}: inactive (empty sprite name)", layer_idx);
        }

        animation->layers.push_back(std::move(layer));
        ++layer_idx;
    }

    spdlog::info("bgaj loaded: {} layers, max_tick={}", animation->layers.size(),
                 animation->max_tick());
    return animation;
}

// --- BGA binary loader ---

static const char* effect_names[] = {"normal", "screen", "multiply", "dodge", "difference"};

static std::unique_ptr<Sprite> load_sprite_for_layer(
        const std::string& sprite_name,
        const fs::path& base_dir,
        TextureCache& cache) {
    auto ext_lower = to_lower(fs::path(sprite_name).extension().string());

    if (is_image_ext(ext_lower)) {
        return make_raw_texture_sprite(sprite_name, base_dir, cache);
    }

    // Find actual file on disk (case-insensitive)
    auto found = find_file_ci(sprite_name, base_dir);
    if (!found.empty()) {
        return load_sprite(found, cache);
    }

    // Try .sprj equivalent
    if (ext_lower == ".spr" || ext_lower == ".sp2") {
        auto stem = fs::path(sprite_name).stem().string();
        auto sprj = find_file_ci(stem + ".sprj", base_dir);
        if (!sprj.empty()) {
            return load_sprite(sprj, cache);
        }
    }

    spdlog::error("sprite not found: '{}' in {}", sprite_name, base_dir.string());
    throw std::runtime_error("Sprite not found: " + sprite_name);
}

struct BgaEventRaw {
    float translate_x, translate_y;
    float pivot_x, pivot_y;
    float scale_x, scale_y;
    float rotate;
    float color_r, color_g, color_b, color_a;
    uint16_t tick;
    int16_t display;
    int16_t effect;
    char reserved[14];
};
static_assert(sizeof(BgaEventRaw) == 64);

std::unique_ptr<BgaAnimation> load_bga(const fs::path& path, TextureCache& cache) {
    spdlog::info("loading bga binary: {}", path.string());

    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        spdlog::error("failed to open bga file: {}", path.string());
        throw std::runtime_error("Failed to open BGA file: " + path.string());
    }

    char magic[4];
    file.read(magic, 4);
    if (std::memcmp(magic, "BGA2", 4) != 0) {
        spdlog::error("invalid BGA magic in {}", path.string());
        throw std::runtime_error("Invalid BGA magic (expected BGA2)");
    }

    file.seekg(12, std::ios::cur);

    auto animation = std::make_unique<BgaAnimation>();
    auto base_dir = path.parent_path();

    for (int layer_idx = 0; layer_idx < 50; ++layer_idx) {
        char name_buf[64];
        file.read(name_buf, 64);
        std::string sprite_name(name_buf, std::find(name_buf, name_buf + 64, '\0'));

        int32_t num_events;
        file.read(reinterpret_cast<char*>(&num_events), 4);
        if (num_events < 0) num_events = 1;

        std::vector<BgaEventRaw> events(num_events);
        for (int i = 0; i < num_events; ++i) {
            file.read(reinterpret_cast<char*>(&events[i]), sizeof(BgaEventRaw));
        }

        if (sprite_name.empty() || sprite_name[0] == ' ') {
            spdlog::debug("layer {}: inactive", layer_idx);
            continue;
        }

        Layer layer;
        layer.sprite_name = sprite_name;

        for (const auto& ev : events) {
            Keyframe kf;
            kf.tick = ev.tick;
            kf.translate_x = ev.translate_x;
            kf.translate_y = ev.translate_y;
            kf.pivot_x = ev.pivot_x;
            kf.pivot_y = ev.pivot_y;
            kf.scale_x = ev.scale_x;
            kf.scale_y = ev.scale_y;
            kf.rotate = ev.rotate;
            kf.color_r = ev.color_r;
            kf.color_g = ev.color_g;
            kf.color_b = ev.color_b;
            kf.color_a = ev.color_a;
            kf.display = (ev.display != 0);
            int eff = ev.effect;
            if (eff < 0 || eff > 4) eff = 0;
            kf.effect = static_cast<BlendEffect>(eff);
            layer.keyframes.push_back(std::move(kf));
        }

        spdlog::info("layer {}: sprite '{}', {} keyframes", layer_idx,
                     sprite_name, layer.keyframes.size());

        try {
            auto sprite = load_sprite_for_layer(sprite_name, base_dir, cache);
            layer.sprite = sprite.get();
            animation->owned_sprites.push_back(std::move(sprite));
        } catch (const std::exception& e) {
            spdlog::error("layer {}: failed to load sprite '{}': {}",
                          layer_idx, sprite_name, e.what());
        }

        animation->layers.push_back(std::move(layer));
    }

    spdlog::info("bga loaded: {} active layers, max_tick={}",
                 animation->layers.size(), animation->max_tick());
    return animation;
}

// --- Auto-detect ---

std::unique_ptr<BgaAnimation> load_bga_auto(const fs::path& path, TextureCache& cache) {
    auto ext = to_lower(path.extension().string());
    if (ext == ".bgaj") return load_bgaj(path, cache);
    if (ext == ".bga")  return load_bga(path, cache);
    spdlog::error("unknown animation extension: {}", path.string());
    throw std::runtime_error("Unknown animation extension: " + path.string());
}

} // namespace openitup
