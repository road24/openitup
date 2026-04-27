#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <SDL3/SDL.h>

#include <openitup/core/json_serializable.h>
#include <openitup/gfx/texture_cache.h>
#include <openitup/math/types.h>
#include <openitup/sprite/picture.h>

namespace openitup {

enum class SpriteMode : uint8_t { Tile, Ani, Pattern };
enum class PatternDirection : uint8_t { Horizontal, Vertical };

struct PatternParams {
    PatternDirection direction = PatternDirection::Horizontal;
    int grid_x = 1;
    int grid_y = 1;
};

class Sprite : public JsonSerializable {
public:
    std::string source_format;
    SpriteMode mode = SpriteMode::Tile;
    PatternParams pattern;
    std::vector<Picture> pictures;

    void draw(SDL_Renderer* renderer,
              const TextureCache& cache,
              float t,
              const LayerTransform& transform,
              const ColorMod& color,
              SDL_BlendMode blend) const;

    void from_json(const nlohmann::json& j) override;
    nlohmann::json to_json() const override;

    // --- Mode logic (public for testability) ---

    // ANI: which frame index for a given t
    static int ani_frame(int num_pictures, float t);

    // PATTERN: which picture index for a given cell and state
    static int pattern_index(int linear_pos, int num_pictures, float t);

    // TILE: the draw order (returns indices in the order they should be drawn)
    static std::vector<int> tile_draw_order(int num_pictures);

    // PATTERN: cell iteration order
    struct CellInfo {
        int col, row, picture_index;
    };
    static std::vector<CellInfo> pattern_cells(
        int grid_x, int grid_y, PatternDirection direction,
        int num_pictures, float t);

private:
    void draw_tile(SDL_Renderer*, const TextureCache&,
                   const LayerTransform&, const ColorMod&, SDL_BlendMode) const;
    void draw_ani(SDL_Renderer*, const TextureCache&, float t,
                  const LayerTransform&, const ColorMod&, SDL_BlendMode) const;
    void draw_pattern(SDL_Renderer*, const TextureCache&, float t,
                      const LayerTransform&, const ColorMod&, SDL_BlendMode) const;

    void draw_picture(SDL_Renderer*, const TextureCache&,
                      const Picture& pic,
                      float cell_offset_x, float cell_offset_y,
                      const LayerTransform& transform,
                      const ColorMod& color,
                      SDL_BlendMode blend) const;
};

} // namespace openitup
