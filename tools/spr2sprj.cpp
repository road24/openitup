// Converts .spr and .sp2 files to .sprj (JSON) format.
//
// Usage:
//   spr2sprj <input.spr|input.sp2> [output.sprj]
//   spr2sprj <input.spr> --asset-dir <path> [output.sprj]
//
// For SPR files, texture dimensions are needed to normalize UVs.
// The tool probes for texture files in the input file's directory
// (or --asset-dir if specified) using the .tga/.png/.dds fallback.
//
// For SP2 files, texture dimensions are not needed (fixed 256 divisor).

#include <openitup/gfx/image_loader.h>

#include <CLI/CLI.hpp>
#include <SDL3/SDL.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;
using json = nlohmann::json;

struct TexDims {
    int w, h;
};

static std::unordered_map<std::string, TexDims> tex_cache;

static std::string to_lower(const std::string& s) {
    std::string r = s;
    std::transform(r.begin(), r.end(), r.begin(),
        [](unsigned char c) { return std::tolower(c); });
    return r;
}

static TexDims get_texture_dims(const std::string& name, const fs::path& asset_dir) {
    auto it = tex_cache.find(name);
    if (it != tex_cache.end()) return it->second;

    auto stem = fs::path(name).stem().string();
    const char* exts[] = {".tga", ".png", ".dds"};

    // Fast path: exact case
    for (const char* ext : exts) {
        auto path = asset_dir / (stem + ext);
        if (fs::exists(path)) {
            SDL_Surface* surface = openitup::load_image(path);
            if (surface) {
                TexDims dims{surface->w, surface->h};
                SDL_DestroySurface(surface);
                tex_cache[name] = dims;
                return dims;
            }
        }
    }

    // Case-insensitive: scan directory
    auto stem_lower = to_lower(stem);
    for (const auto& entry : fs::directory_iterator(asset_dir)) {
        if (!entry.is_regular_file()) continue;
        if (to_lower(entry.path().stem().string()) != stem_lower) continue;
        auto ext_lower = to_lower(entry.path().extension().string());
        for (const char* ext : exts) {
            if (ext_lower != ext) continue;
            SDL_Surface* surface = openitup::load_image(entry.path());
            if (surface) {
                TexDims dims{surface->w, surface->h};
                SDL_DestroySurface(surface);
                tex_cache[name] = dims;
                return dims;
            }
        }
    }

    std::fprintf(stderr, "Error: texture '%s' not found in %s\n"
                 "SPR conversion requires texture files to read their dimensions for UV normalization.\n"
                 "Ensure all referenced textures (.tga/.png/.dds) are present, or use --asset-dir.\n",
                 name.c_str(), asset_dir.string().c_str());
    std::exit(1);
}

static std::string to_upper(const std::string& s) {
    std::string r = s;
    std::transform(r.begin(), r.end(), r.begin(), ::toupper);
    return r;
}

enum class Format { SPR, SP2 };

static json convert(const fs::path& input_path, const fs::path& asset_dir, Format fmt) {
    std::ifstream file(input_path);
    if (!file.is_open()) {
        std::fprintf(stderr, "Error: cannot open '%s'\n", input_path.string().c_str());
        std::exit(1);
    }

    std::string mode_str = "tile";
    std::string direction_str;
    int grid_x = 0, grid_y = 0;
    bool has_pattern = false;

    json pictures = json::array();

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string directive;
        if (!(iss >> directive)) continue;

        std::string dir_upper = to_upper(directive);

        if (dir_upper == "NUM") {
            continue;
        } else if (dir_upper == "TYPE") {
            std::string mode;
            iss >> mode;
            std::string mode_up = to_upper(mode);
            if (mode_up == "TILE") {
                mode_str = "tile";
            } else if (mode_up == "ANI") {
                mode_str = "ani";
            } else if (mode_up == "PATTERN") {
                mode_str = "pattern";
                has_pattern = true;
                int dir_int;
                iss >> dir_int >> grid_y >> grid_x;
                direction_str = (dir_int == 0) ? "horizontal" : "vertical";
            } else {
                mode_str = "tile";
            }
        } else if (dir_upper == "T") {
            json pic;

            if (fmt == Format::SP2) {
                std::string pic_name, tex_file;
                int x, y, w, h, tex_x, tex_y, tex_w, tex_h;
                iss >> pic_name >> tex_file >> x >> y >> w >> h
                    >> tex_x >> tex_y >> tex_w >> tex_h;

                float u1 = tex_x / 256.0f;
                float v1 = tex_y / 256.0f;

                pic = {
                    {"name", pic_name},
                    {"texture", tex_file},
                    {"rect", {x, y, x + w, y + h}},
                    {"uv", {u1, v1, u1 + tex_w / 256.0f, v1 + tex_h / 256.0f}}
                };
            } else {
                std::string tex_file;
                int x, y, w, h, tex_x1, tex_y1, tex_x2, tex_y2;
                iss >> tex_file >> x >> y >> w >> h
                    >> tex_x1 >> tex_y1 >> tex_x2 >> tex_y2;

                auto dims = get_texture_dims(tex_file, asset_dir);
                float tw = static_cast<float>(dims.w);
                float th = static_cast<float>(dims.h);

                pic = {
                    {"texture", tex_file},
                    {"rect", {x, y, x + w, y + h}},
                    {"uv", {tex_x1 / tw, tex_y1 / th, tex_x2 / tw, tex_y2 / th}}
                };
            }

            pictures.push_back(pic);
        }
    }

    json result;
    result["source_format"] = (fmt == Format::SP2) ? "sp2" : "spr";
    result["mode"] = mode_str;

    if (has_pattern) {
        result["pattern"] = {
            {"direction", direction_str},
            {"grid_x", grid_x},
            {"grid_y", grid_y}
        };
    }

    result["pictures"] = pictures;
    return result;
}

int main(int argc, char* argv[]) {
    CLI::App app{"Convert .spr/.sp2 sprite files to .sprj (JSON)"};

    std::string input_str;
    std::string output_str;
    std::string asset_dir_str;

    app.add_option("input", input_str, "Input .spr or .sp2 file")->required();
    app.add_option("output", output_str, "Output .sprj file (default: input with .sprj extension)");
    app.add_option("--asset-dir", asset_dir_str,
        "Directory to search for textures (default: input file dir).\n"
        "SPR files require texture files for UV normalization.\n"
        "SP2 files use a fixed 256 divisor (no texture loading needed).");

    CLI11_PARSE(app, argc, argv);

    fs::path input_path(input_str);
    fs::path asset_dir = asset_dir_str.empty()
        ? input_path.parent_path()
        : fs::path(asset_dir_str);
    if (asset_dir.empty()) asset_dir = ".";

    fs::path output_path = output_str.empty()
        ? fs::path(input_path).replace_extension(".sprj")
        : fs::path(output_str);

    auto ext = input_path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    Format fmt;
    if (ext == ".sp2") {
        fmt = Format::SP2;
    } else if (ext == ".spr") {
        fmt = Format::SPR;
    } else {
        std::fprintf(stderr, "Error: expected .spr or .sp2 file, got '%s'\n", ext.c_str());
        return 1;
    }

    if (fmt == Format::SPR) {
        SDL_Init(0);
    }

    json result = convert(input_path, asset_dir, fmt);

    if (fmt == Format::SPR) {
        SDL_Quit();
    }

    std::ofstream out(output_path);
    if (!out.is_open()) {
        std::fprintf(stderr, "Error: cannot write '%s'\n", output_path.string().c_str());
        return 1;
    }

    out << result.dump(2) << "\n";
    std::fprintf(stdout, "%s -> %s (%zu pictures)\n",
                 input_path.string().c_str(),
                 output_path.string().c_str(),
                 result["pictures"].size());
    return 0;
}
