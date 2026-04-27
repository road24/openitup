#include <gtest/gtest.h>
#include <openitup/sprite/sprite.h>
#include <openitup/sprite/picture.h>
#include <cmath>
#include <set>

using namespace openitup;

// --- ANI frame selection ---

TEST(AniFrame, AtZero) {
    EXPECT_EQ(Sprite::ani_frame(6, 0.0f), 0);
}

TEST(AniFrame, AtOne) {
    EXPECT_EQ(Sprite::ani_frame(6, 1.0f), 5);
}

TEST(AniFrame, AtHalf) {
    // floor(6 * 0.5) = 3
    EXPECT_EQ(Sprite::ani_frame(6, 0.5f), 3);
}

TEST(AniFrame, JustBeforeOne) {
    // floor(6 * 0.999) = floor(5.994) = 5
    EXPECT_EQ(Sprite::ani_frame(6, 0.999f), 5);
}

TEST(AniFrame, FirstFrame) {
    // floor(4 * 0.1) = floor(0.4) = 0
    EXPECT_EQ(Sprite::ani_frame(4, 0.1f), 0);
}

TEST(AniFrame, LastFrame) {
    // floor(4 * 0.9) = floor(3.6) = 3
    EXPECT_EQ(Sprite::ani_frame(4, 0.9f), 3);
}

TEST(AniFrame, ClampsAboveOne) {
    EXPECT_EQ(Sprite::ani_frame(4, 1.5f), 3);
}

TEST(AniFrame, ClampsBelowZero) {
    EXPECT_EQ(Sprite::ani_frame(4, -0.1f), 0);
}

TEST(AniFrame, SinglePicture) {
    EXPECT_EQ(Sprite::ani_frame(1, 0.0f), 0);
    EXPECT_EQ(Sprite::ani_frame(1, 0.5f), 0);
    EXPECT_EQ(Sprite::ani_frame(1, 1.0f), 0);
}

TEST(AniFrame, EmptyPictures) {
    EXPECT_EQ(Sprite::ani_frame(0, 0.5f), 0);
}

// --- TILE draw order ---

TEST(TileDrawOrder, ReversesOrder) {
    auto order = Sprite::tile_draw_order(3);
    ASSERT_EQ(order.size(), 3u);
    EXPECT_EQ(order[0], 2);  // last picture drawn first
    EXPECT_EQ(order[1], 1);
    EXPECT_EQ(order[2], 0);  // first picture drawn last (on top)
}

TEST(TileDrawOrder, SinglePicture) {
    auto order = Sprite::tile_draw_order(1);
    ASSERT_EQ(order.size(), 1u);
    EXPECT_EQ(order[0], 0);
}

TEST(TileDrawOrder, Empty) {
    auto order = Sprite::tile_draw_order(0);
    EXPECT_TRUE(order.empty());
}

// --- PATTERN index ---

TEST(PatternIndex, AtZero) {
    // t=0: state_offset = floor(4 * (1.0 - 0.0)) = 4
    // index = (0 + 4) % 4 = 0
    EXPECT_EQ(Sprite::pattern_index(0, 4, 0.0f), 0);
}

TEST(PatternIndex, AtOne) {
    // t=1: state_offset = floor(4 * (1.0 - 1.0)) = 0
    // index = (0 + 0) % 4 = 0
    EXPECT_EQ(Sprite::pattern_index(0, 4, 1.0f), 0);
}

TEST(PatternIndex, ShiftsWithT) {
    // t=0.5: state_offset = floor(4 * 0.5) = 2
    // linear_pos=0: (0 + 2) % 4 = 2
    // linear_pos=1: (1 + 2) % 4 = 3
    // linear_pos=2: (2 + 2) % 4 = 0
    // linear_pos=3: (3 + 2) % 4 = 1
    EXPECT_EQ(Sprite::pattern_index(0, 4, 0.5f), 2);
    EXPECT_EQ(Sprite::pattern_index(1, 4, 0.5f), 3);
    EXPECT_EQ(Sprite::pattern_index(2, 4, 0.5f), 0);
    EXPECT_EQ(Sprite::pattern_index(3, 4, 0.5f), 1);
}

TEST(PatternIndex, WrapsAround) {
    // 6 cells, 4 pictures: indices should wrap
    // t=0: state_offset=4, (5 + 4) % 4 = 1
    EXPECT_EQ(Sprite::pattern_index(5, 4, 0.0f), 1);
}

// --- PATTERN cells ---

TEST(PatternCells, HorizontalDirection) {
    // 3x2 grid (3 cols, 2 rows), 4 pictures, t=0
    auto cells = Sprite::pattern_cells(3, 2, PatternDirection::Horizontal, 4, 0.0f);

    ASSERT_EQ(cells.size(), 6u);

    // Horizontal: iterates rows then cols within each row
    // Row 0: (0,0), (1,0), (2,0)
    // Row 1: (0,1), (1,1), (2,1)
    EXPECT_EQ(cells[0].col, 0); EXPECT_EQ(cells[0].row, 0);
    EXPECT_EQ(cells[1].col, 1); EXPECT_EQ(cells[1].row, 0);
    EXPECT_EQ(cells[2].col, 2); EXPECT_EQ(cells[2].row, 0);
    EXPECT_EQ(cells[3].col, 0); EXPECT_EQ(cells[3].row, 1);
    EXPECT_EQ(cells[4].col, 1); EXPECT_EQ(cells[4].row, 1);
    EXPECT_EQ(cells[5].col, 2); EXPECT_EQ(cells[5].row, 1);
}

TEST(PatternCells, VerticalDirection) {
    // 3x2 grid, 4 pictures, t=0
    auto cells = Sprite::pattern_cells(3, 2, PatternDirection::Vertical, 4, 0.0f);

    ASSERT_EQ(cells.size(), 6u);

    // Vertical: iterates cols then rows within each col
    // Col 0: (0,0), (0,1)
    // Col 1: (1,0), (1,1)
    // Col 2: (2,0), (2,1)
    EXPECT_EQ(cells[0].col, 0); EXPECT_EQ(cells[0].row, 0);
    EXPECT_EQ(cells[1].col, 0); EXPECT_EQ(cells[1].row, 1);
    EXPECT_EQ(cells[2].col, 1); EXPECT_EQ(cells[2].row, 0);
    EXPECT_EQ(cells[3].col, 1); EXPECT_EQ(cells[3].row, 1);
    EXPECT_EQ(cells[4].col, 2); EXPECT_EQ(cells[4].row, 0);
    EXPECT_EQ(cells[5].col, 2); EXPECT_EQ(cells[5].row, 1);
}

TEST(PatternCells, AllPicturesUsed) {
    // With enough cells, all picture indices should appear
    auto cells = Sprite::pattern_cells(4, 4, PatternDirection::Horizontal, 4, 0.0f);
    std::set<int> used;
    for (const auto& c : cells) {
        used.insert(c.picture_index);
    }
    EXPECT_EQ(used.size(), 4u);
    EXPECT_NE(used.find(0), used.end());
    EXPECT_NE(used.find(1), used.end());
    EXPECT_NE(used.find(2), used.end());
    EXPECT_NE(used.find(3), used.end());
}

TEST(PatternCells, ShiftsWithT) {
    auto cells_t0 = Sprite::pattern_cells(2, 2, PatternDirection::Horizontal, 4, 0.0f);
    auto cells_t5 = Sprite::pattern_cells(2, 2, PatternDirection::Horizontal, 4, 0.5f);

    // Different t values should produce different cell assignments
    bool any_different = false;
    for (size_t i = 0; i < cells_t0.size(); ++i) {
        if (cells_t0[i].picture_index != cells_t5[i].picture_index) {
            any_different = true;
            break;
        }
    }
    EXPECT_TRUE(any_different);
}

// --- Sprite JSON round-trip ---

TEST(SpriteJson, TileRoundTrip) {
    Sprite spr;
    spr.source_format = "spr";
    spr.mode = SpriteMode::Tile;

    Picture pic;
    pic.texture_name = "bg.tga";
    pic.rect = {0, 0, 640, 480};
    pic.uv = {0.0f, 0.0f, 1.0f, 1.0f};
    spr.pictures.push_back(pic);

    auto j = spr.to_json();
    Sprite spr2;
    spr2.from_json(j);

    EXPECT_EQ(spr2.source_format, "spr");
    EXPECT_EQ(spr2.mode, SpriteMode::Tile);
    ASSERT_EQ(spr2.pictures.size(), 1u);
    EXPECT_EQ(spr2.pictures[0].texture_name, "bg.tga");
    EXPECT_EQ(spr2.pictures[0].rect.x2, 640);
    EXPECT_FLOAT_EQ(spr2.pictures[0].uv.u2, 1.0f);
}

TEST(SpriteJson, AniWithNames) {
    Sprite spr;
    spr.source_format = "sp2";
    spr.mode = SpriteMode::Ani;

    for (int i = 0; i < 4; ++i) {
        Picture pic;
        pic.texture_name = "char.tga";
        pic.name = "walk_" + std::to_string(i);
        pic.rect = {0, 0, 64, 96};
        pic.uv = {0.25f * i, 0.0f, 0.25f * (i + 1), 0.375f};
        spr.pictures.push_back(pic);
    }

    auto j = spr.to_json();
    Sprite spr2;
    spr2.from_json(j);

    EXPECT_EQ(spr2.mode, SpriteMode::Ani);
    ASSERT_EQ(spr2.pictures.size(), 4u);
    EXPECT_EQ(spr2.pictures[2].name, "walk_2");
    EXPECT_FLOAT_EQ(spr2.pictures[1].uv.u1, 0.25f);
}

TEST(SpriteJson, PatternRoundTrip) {
    Sprite spr;
    spr.source_format = "spr";
    spr.mode = SpriteMode::Pattern;
    spr.pattern.direction = PatternDirection::Vertical;
    spr.pattern.grid_x = 3;
    spr.pattern.grid_y = 2;

    Picture pic;
    pic.texture_name = "tile.tga";
    pic.rect = {0, 0, 64, 64};
    pic.uv = {0.0f, 0.0f, 1.0f, 1.0f};
    spr.pictures.push_back(pic);

    auto j = spr.to_json();
    Sprite spr2;
    spr2.from_json(j);

    EXPECT_EQ(spr2.mode, SpriteMode::Pattern);
    EXPECT_EQ(spr2.pattern.direction, PatternDirection::Vertical);
    EXPECT_EQ(spr2.pattern.grid_x, 3);
    EXPECT_EQ(spr2.pattern.grid_y, 2);
}

TEST(SpriteJson, PatternOmittedForNonPatternMode) {
    Sprite spr;
    spr.source_format = "spr";
    spr.mode = SpriteMode::Tile;
    spr.pictures.push_back(Picture{});
    spr.pictures[0].texture_name = "x.tga";
    spr.pictures[0].rect = {0, 0, 1, 1};
    spr.pictures[0].uv = {0, 0, 1, 1};

    auto j = spr.to_json();
    EXPECT_FALSE(j.contains("pattern"));
}

// --- Picture JSON ---

TEST(PictureJson, RoundTrip) {
    Picture pic;
    pic.texture_name = "arrow.tga";
    pic.rect = {10, 20, 74, 116};
    pic.uv = {0.0f, 0.0f, 0.25f, 0.375f};
    pic.name = "walk_0";

    auto j = pic.to_json();
    Picture pic2;
    pic2.from_json(j);

    EXPECT_EQ(pic2.texture_name, "arrow.tga");
    EXPECT_EQ(pic2.rect.x1, 10);
    EXPECT_EQ(pic2.rect.y2, 116);
    EXPECT_FLOAT_EQ(pic2.uv.u2, 0.25f);
    EXPECT_EQ(pic2.name, "walk_0");
    EXPECT_EQ(pic2.width(), 64);
    EXPECT_EQ(pic2.height(), 96);
}

TEST(PictureJson, OptionalName) {
    Picture pic;
    pic.texture_name = "bg.tga";
    pic.rect = {0, 0, 640, 480};
    pic.uv = {0, 0, 1, 1};
    // name is empty — should be omitted from JSON

    auto j = pic.to_json();
    EXPECT_FALSE(j.contains("name"));

    Picture pic2;
    pic2.from_json(j);
    EXPECT_TRUE(pic2.name.empty());
}
