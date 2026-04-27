#include <openitup/sprite/sprite.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include <spdlog/spdlog.h>

namespace openitup {

// --- JSON serialization ---

static SpriteMode mode_from_string(const std::string& s) {
    if (s == "ani") return SpriteMode::Ani;
    if (s == "pattern") return SpriteMode::Pattern;
    return SpriteMode::Tile;
}

static const char* mode_to_string(SpriteMode m) {
    switch (m) {
        case SpriteMode::Ani:     return "ani";
        case SpriteMode::Pattern: return "pattern";
        default:                  return "tile";
    }
}

void Sprite::from_json(const nlohmann::json& j) {
    source_format = j.at("source_format").get<std::string>();
    mode = mode_from_string(j.at("mode").get<std::string>());

    if (j.contains("pattern")) {
        const auto& p = j.at("pattern");
        auto dir_str = p.at("direction").get<std::string>();
        pattern.direction = (dir_str == "vertical")
            ? PatternDirection::Vertical
            : PatternDirection::Horizontal;
        pattern.grid_x = p.at("grid_x").get<int>();
        pattern.grid_y = p.at("grid_y").get<int>();
    }

    pictures.clear();
    for (const auto& pic_json : j.at("pictures")) {
        Picture pic;
        pic.from_json(pic_json);
        pictures.push_back(std::move(pic));
    }
}

nlohmann::json Sprite::to_json() const {
    nlohmann::json j;
    j["source_format"] = source_format;
    j["mode"] = mode_to_string(mode);

    if (mode == SpriteMode::Pattern) {
        j["pattern"] = {
            {"direction", pattern.direction == PatternDirection::Vertical
                ? "vertical" : "horizontal"},
            {"grid_x", pattern.grid_x},
            {"grid_y", pattern.grid_y}
        };
    }

    auto& pics = j["pictures"] = nlohmann::json::array();
    for (const auto& pic : pictures) {
        pics.push_back(pic.to_json());
    }

    return j;
}

// --- Static mode logic (no SDL, pure math) ---

int Sprite::ani_frame(int num_pictures, float t) {
    if (num_pictures <= 0) return 0;
    int frame = static_cast<int>(std::floor(num_pictures * t));
    return std::clamp(frame, 0, num_pictures - 1);
}

int Sprite::pattern_index(int linear_pos, int num_pictures, float t) {
    if (num_pictures <= 0) return 0;
    int state_offset = static_cast<int>(std::floor(num_pictures * (1.0f - t)));
    int idx = (linear_pos + state_offset) % num_pictures;
    if (idx < 0) idx += num_pictures;
    return idx;
}

std::vector<int> Sprite::tile_draw_order(int num_pictures) {
    std::vector<int> order;
    order.reserve(num_pictures);
    for (int i = num_pictures - 1; i >= 0; --i) {
        order.push_back(i);
    }
    return order;
}

std::vector<Sprite::CellInfo> Sprite::pattern_cells(
        int grid_x, int grid_y, PatternDirection direction,
        int num_pictures, float t) {
    std::vector<CellInfo> cells;
    cells.reserve(grid_x * grid_y);

    if (direction == PatternDirection::Horizontal) {
        for (int row = 0; row < grid_y; ++row) {
            for (int col = 0; col < grid_x; ++col) {
                int linear_pos = col + grid_x * row;
                cells.push_back({col, row,
                    pattern_index(linear_pos, num_pictures, t)});
            }
        }
    } else {
        for (int col = 0; col < grid_x; ++col) {
            for (int row = 0; row < grid_y; ++row) {
                int linear_pos = col * grid_y + row;
                cells.push_back({col, row,
                    pattern_index(linear_pos, num_pictures, t)});
            }
        }
    }

    return cells;
}

// --- SDL rendering ---

void Sprite::draw(SDL_Renderer* renderer, const TextureCache& cache,
                  float t, const LayerTransform& transform,
                  const ColorMod& color, SDL_BlendMode blend) const {
    switch (mode) {
        case SpriteMode::Tile:
            draw_tile(renderer, cache, transform, color, blend);
            break;
        case SpriteMode::Ani:
            draw_ani(renderer, cache, t, transform, color, blend);
            break;
        case SpriteMode::Pattern:
            draw_pattern(renderer, cache, t, transform, color, blend);
            break;
    }
}

void Sprite::draw_tile(SDL_Renderer* renderer, const TextureCache& cache,
                       const LayerTransform& transform,
                       const ColorMod& color, SDL_BlendMode blend) const {
    for (int i = static_cast<int>(pictures.size()) - 1; i >= 0; --i) {
        draw_picture(renderer, cache, pictures[i], 0, 0, transform, color, blend);
    }
}

void Sprite::draw_ani(SDL_Renderer* renderer, const TextureCache& cache,
                      float t, const LayerTransform& transform,
                      const ColorMod& color, SDL_BlendMode blend) const {
    if (pictures.empty()) return;
    int frame = ani_frame(static_cast<int>(pictures.size()), t);
    draw_picture(renderer, cache, pictures[frame], 0, 0, transform, color, blend);
}

void Sprite::draw_pattern(SDL_Renderer* renderer, const TextureCache& cache,
                          float t, const LayerTransform& transform,
                          const ColorMod& color, SDL_BlendMode blend) const {
    if (pictures.empty()) return;

    float cell_w = static_cast<float>(pictures[0].width());
    float cell_h = static_cast<float>(pictures[0].height());
    int n = static_cast<int>(pictures.size());

    auto cells = pattern_cells(pattern.grid_x, pattern.grid_y,
                               pattern.direction, n, t);
    for (const auto& cell : cells) {
        draw_picture(renderer, cache, pictures[cell.picture_index],
                     cell_w * cell.col, cell_h * cell.row,
                     transform, color, blend);
    }
}

void Sprite::draw_picture(SDL_Renderer* renderer, const TextureCache& cache,
                          const Picture& pic,
                          float cell_offset_x, float cell_offset_y,
                          const LayerTransform& transform,
                          const ColorMod& color,
                          SDL_BlendMode blend) const {
    SDL_Texture* tex = cache.get(pic.texture);
    if (!tex) {
        spdlog::error("null texture for picture '{}' (handle {})",
                      pic.texture_name, static_cast<uint16_t>(pic.texture));
        return;
    }

    float pic_x = static_cast<float>(pic.rect.x1) + cell_offset_x;
    float pic_y = static_cast<float>(pic.rect.y1) + cell_offset_y;
    float pic_w = static_cast<float>(pic.width());
    float pic_h = static_cast<float>(pic.height());

    float abs_sx = std::fabs(transform.scale_x);
    float abs_sy = std::fabs(transform.scale_y);

    SDL_FRect dstrect;
    dstrect.x = transform.translate_x + transform.pivot_x
              + (pic_x - transform.pivot_x) * transform.scale_x;
    dstrect.y = transform.translate_y + transform.pivot_y
              + (pic_y - transform.pivot_y) * transform.scale_y;
    dstrect.w = pic_w * abs_sx;
    dstrect.h = pic_h * abs_sy;

    SDL_FPoint center;
    center.x = (transform.pivot_x - pic_x) * abs_sx;
    center.y = (transform.pivot_y - pic_y) * abs_sy;

    SDL_FlipMode flip = SDL_FLIP_NONE;
    if (transform.scale_x < 0) flip = static_cast<SDL_FlipMode>(flip | SDL_FLIP_HORIZONTAL);
    if (transform.scale_y < 0) flip = static_cast<SDL_FlipMode>(flip | SDL_FLIP_VERTICAL);

    // Source rect from normalized UVs.
    // SPR format allows tex_x1 > tex_x2 to indicate horizontal flip, and the
    // SPRJ spec preserves this for lossless round-trip (u1 > u2 is valid).
    // Detect inverted UVs, normalize the source rect, and apply SDL flip.
    float tex_w_f, tex_h_f;
    SDL_GetTextureSize(tex, &tex_w_f, &tex_h_f);

    float su1 = pic.uv.u1, su2 = pic.uv.u2;
    float sv1 = pic.uv.v1, sv2 = pic.uv.v2;

    if (su1 > su2) {
        std::swap(su1, su2);
        flip = static_cast<SDL_FlipMode>(flip ^ SDL_FLIP_HORIZONTAL);
    }
    if (sv1 > sv2) {
        std::swap(sv1, sv2);
        flip = static_cast<SDL_FlipMode>(flip ^ SDL_FLIP_VERTICAL);
    }

    SDL_FRect srcrect;
    srcrect.x = su1 * tex_w_f;
    srcrect.y = sv1 * tex_h_f;
    srcrect.w = (su2 - su1) * tex_w_f;
    srcrect.h = (sv2 - sv1) * tex_h_f;

    SDL_SetTextureColorModFloat(tex, color.r, color.g, color.b);
    SDL_SetTextureAlphaModFloat(tex, color.a);
    SDL_SetTextureBlendMode(tex, blend);

    // BGA spec uses counter-clockwise rotation (OpenGL convention).
    // SDL uses clockwise. Negate to convert.
    SDL_RenderTextureRotated(renderer, tex, &srcrect, &dstrect,
                             static_cast<double>(-transform.rotate),
                             &center, flip);
}

} // namespace openitup
