// Converts .bga (binary) files to .bgaj (JSON) format.
//
// Usage:
//   bga2bgaj <input.bga> [output.bgaj]
//
// Pure binary parsing — no SDL or texture loading required.

#include <CLI/CLI.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using json = nlohmann::json;

static const char* effect_names[] = {"normal", "screen", "multiply", "dodge", "difference"};

struct BgaEvent {
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

static_assert(sizeof(BgaEvent) == 64, "BGA event must be 64 bytes");

template<typename T>
static T read_val(std::ifstream& f) {
    T val;
    f.read(reinterpret_cast<char*>(&val), sizeof(T));
    return val;
}

static std::string read_string(std::ifstream& f, size_t len) {
    std::vector<char> buf(len);
    f.read(buf.data(), len);
    auto end = std::find(buf.begin(), buf.end(), '\0');
    return std::string(buf.begin(), end);
}

static json convert(const fs::path& input_path) {
    std::ifstream f(input_path, std::ios::binary);
    if (!f.is_open()) {
        std::fprintf(stderr, "Error: cannot open '%s'\n", input_path.string().c_str());
        std::exit(1);
    }

    char magic[4];
    f.read(magic, 4);
    if (std::memcmp(magic, "BGA2", 4) != 0) {
        std::fprintf(stderr, "Error: invalid BGA magic (expected BGA2)\n");
        std::exit(1);
    }

    // Skip 12 bytes reserved header
    f.seekg(12, std::ios::cur);

    json layers = json::array();
    int active_count = 0;

    for (int layer_idx = 0; layer_idx < 50; ++layer_idx) {
        std::string sprite_name = read_string(f, 64);
        int32_t num_events = read_val<int32_t>(f);

        if (num_events < 0) num_events = 1;

        std::vector<BgaEvent> events(num_events);
        for (int i = 0; i < num_events; ++i) {
            f.read(reinterpret_cast<char*>(&events[i]), sizeof(BgaEvent));
        }

        // Skip inactive layers (empty name or starts with space)
        if (sprite_name.empty() || sprite_name[0] == ' ') continue;

        json keyframes = json::array();
        for (const auto& ev : events) {
            int effect_idx = ev.effect;
            if (effect_idx < 0 || effect_idx > 4) effect_idx = 0;

            keyframes.push_back({
                {"tick", ev.tick},
                {"translate", {ev.translate_x, ev.translate_y}},
                {"pivot", {ev.pivot_x, ev.pivot_y}},
                {"scale", {ev.scale_x, ev.scale_y}},
                {"rotate", ev.rotate},
                {"color", {ev.color_r, ev.color_g, ev.color_b, ev.color_a}},
                {"display", ev.display != 0},
                {"effect", effect_names[effect_idx]}
            });
        }

        // Remap .spr/.sp2 extensions to .sprj for the JSON engine
        auto sprite_path = fs::path(sprite_name);
        auto ext = sprite_path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext == ".spr" || ext == ".sp2") {
            sprite_path.replace_extension(".sprj");
        }

        layers.push_back({
            {"sprite", sprite_path.string()},
            {"keyframes", keyframes}
        });
        ++active_count;
    }

    return {{"version", 2}, {"layers", layers}};
}

int main(int argc, char* argv[]) {
    CLI::App app{"Convert .bga binary files to .bgaj (JSON)"};

    std::string input_str;
    std::string output_str;

    app.add_option("input", input_str, "Input .bga file")->required();
    app.add_option("output", output_str, "Output .bgaj file (default: input with .bgaj extension)");

    CLI11_PARSE(app, argc, argv);

    fs::path input_path(input_str);
    fs::path output_path = output_str.empty()
        ? fs::path(input_path).replace_extension(".bgaj")
        : fs::path(output_str);

    json result = convert(input_path);

    std::ofstream out(output_path);
    if (!out.is_open()) {
        std::fprintf(stderr, "Error: cannot write '%s'\n", output_path.string().c_str());
        return 1;
    }

    out << result.dump(2) << "\n";
    std::fprintf(stdout, "%s -> %s (%zu active layers)\n",
                 input_path.string().c_str(),
                 output_path.string().c_str(),
                 result["layers"].size());
    return 0;
}
