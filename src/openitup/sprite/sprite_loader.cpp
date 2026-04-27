#include <openitup/sprite/sprite_loader.h>

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace openitup {

namespace fs = std::filesystem;

static std::string str_upper(const std::string& s) {
    std::string r = s;
    std::transform(r.begin(), r.end(), r.begin(), ::toupper);
    return r;
}

static std::string str_lower(const std::string& s) {
    std::string r = s;
    std::transform(r.begin(), r.end(), r.begin(), ::tolower);
    return r;
}

// --- SPRJ loader ---

std::unique_ptr<Sprite> load_sprj(const fs::path& path, TextureCache& cache) {
    spdlog::info("loading sprite (sprj): {}", path.string());

    std::ifstream file(path);
    if (!file.is_open()) {
        spdlog::error("failed to open sprite file: {}", path.string());
        throw std::runtime_error("Failed to open sprite file: " + path.string());
    }

    nlohmann::json j = nlohmann::json::parse(file);

    auto sprite = std::make_unique<Sprite>();
    sprite->from_json(j);

    auto base_dir = path.parent_path();
    for (auto& pic : sprite->pictures) {
        auto result = cache.load(pic.texture_name, base_dir);
        pic.texture = result.handle;
    }

    spdlog::info("sprite loaded: mode={}, {} pictures",
                 j.at("mode").get<std::string>(), sprite->pictures.size());
    return sprite;
}

// --- SPR loader (binary text format, needs texture dims for UV normalization) ---

std::unique_ptr<Sprite> load_spr(const fs::path& path, TextureCache& cache) {
    spdlog::info("loading sprite (spr): {}", path.string());

    std::ifstream file(path);
    if (!file.is_open()) {
        spdlog::error("failed to open spr file: {}", path.string());
        throw std::runtime_error("Failed to open SPR file: " + path.string());
    }

    auto sprite = std::make_unique<Sprite>();
    sprite->source_format = "spr";
    sprite->mode = SpriteMode::Tile;

    auto base_dir = path.parent_path();
    std::string line;

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string directive;
        if (!(iss >> directive)) continue;

        std::string dir_upper = str_upper(directive);

        if (dir_upper == "NUM") {
            continue;
        } else if (dir_upper == "TYPE") {
            std::string mode;
            iss >> mode;
            std::string mode_up = str_upper(mode);
            if (mode_up == "ANI") {
                sprite->mode = SpriteMode::Ani;
            } else if (mode_up == "PATTERN") {
                sprite->mode = SpriteMode::Pattern;
                int dir_int;
                iss >> dir_int >> sprite->pattern.grid_y >> sprite->pattern.grid_x;
                sprite->pattern.direction = (dir_int == 0)
                    ? PatternDirection::Horizontal : PatternDirection::Vertical;
            } else {
                sprite->mode = SpriteMode::Tile;
            }
        } else if (dir_upper == "T") {
            std::string tex_file;
            int x, y, w, h, tex_x1, tex_y1, tex_x2, tex_y2;
            iss >> tex_file >> x >> y >> w >> h
                >> tex_x1 >> tex_y1 >> tex_x2 >> tex_y2;

            auto result = cache.load(tex_file, base_dir);

            float tw = static_cast<float>(result.width);
            float th = static_cast<float>(result.height);

            Picture pic;
            pic.texture_name = tex_file;
            pic.texture = result.handle;
            pic.rect = {x, y, x + w, y + h};
            pic.uv = {
                static_cast<float>(tex_x1) / tw,
                static_cast<float>(tex_y1) / th,
                static_cast<float>(tex_x2) / tw,
                static_cast<float>(tex_y2) / th
            };
            sprite->pictures.push_back(std::move(pic));
        }
    }

    spdlog::info("spr loaded: mode={}, {} pictures",
                 static_cast<int>(sprite->mode), sprite->pictures.size());
    return sprite;
}

// --- SP2 loader (text format, fixed 256 divisor) ---

std::unique_ptr<Sprite> load_sp2(const fs::path& path, TextureCache& cache) {
    spdlog::info("loading sprite (sp2): {}", path.string());

    std::ifstream file(path);
    if (!file.is_open()) {
        spdlog::error("failed to open sp2 file: {}", path.string());
        throw std::runtime_error("Failed to open SP2 file: " + path.string());
    }

    auto sprite = std::make_unique<Sprite>();
    sprite->source_format = "sp2";
    sprite->mode = SpriteMode::Tile;

    auto base_dir = path.parent_path();
    std::string line;

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string directive;
        if (!(iss >> directive)) continue;

        std::string dir_upper = str_upper(directive);

        if (dir_upper == "NUM") {
            continue;
        } else if (dir_upper == "TYPE") {
            std::string mode;
            iss >> mode;
            std::string mode_up = str_upper(mode);
            if (mode_up == "ANI") {
                sprite->mode = SpriteMode::Ani;
            } else if (mode_up == "PATTERN") {
                sprite->mode = SpriteMode::Pattern;
                int dir_int;
                iss >> dir_int >> sprite->pattern.grid_y >> sprite->pattern.grid_x;
                sprite->pattern.direction = (dir_int == 0)
                    ? PatternDirection::Horizontal : PatternDirection::Vertical;
            } else {
                sprite->mode = SpriteMode::Tile;
            }
        } else if (dir_upper == "T") {
            std::string pic_name, tex_file;
            int x, y, w, h, tex_x, tex_y, tex_w, tex_h;
            iss >> pic_name >> tex_file >> x >> y >> w >> h
                >> tex_x >> tex_y >> tex_w >> tex_h;

            auto result = cache.load(tex_file, base_dir);

            float u1 = tex_x / 256.0f;
            float v1 = tex_y / 256.0f;

            Picture pic;
            pic.name = pic_name;
            pic.texture_name = tex_file;
            pic.texture = result.handle;
            pic.rect = {x, y, x + w, y + h};
            pic.uv = {u1, v1, u1 + tex_w / 256.0f, v1 + tex_h / 256.0f};
            sprite->pictures.push_back(std::move(pic));
        }
    }

    spdlog::info("sp2 loaded: mode={}, {} pictures",
                 static_cast<int>(sprite->mode), sprite->pictures.size());
    return sprite;
}

// --- Auto-detect by extension ---

std::unique_ptr<Sprite> load_sprite(const fs::path& path, TextureCache& cache) {
    auto ext = str_lower(path.extension().string());
    if (ext == ".sprj") return load_sprj(path, cache);
    if (ext == ".spr")  return load_spr(path, cache);
    if (ext == ".sp2")  return load_sp2(path, cache);
    spdlog::error("unknown sprite extension: {}", path.string());
    throw std::runtime_error("Unknown sprite extension: " + path.string());
}

} // namespace openitup
