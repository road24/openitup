// Generates sample texture files for the test fixtures.
// Run once: ./generate_samples
// Creates .png files in the current directory.
// SPRJ files reference .tga names; the texture probe finds .png via fallback.

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <cstdio>

static bool make_solid(const char* name, int w, int h, uint8_t r, uint8_t g, uint8_t b) {
    SDL_Surface* s = SDL_CreateSurface(w, h, SDL_PIXELFORMAT_RGBA32);
    if (!s) return false;
    SDL_FillSurfaceRect(s, nullptr, SDL_MapSurfaceRGBA(s, r, g, b, 255));
    bool ok = IMG_SavePNG(s, name);
    SDL_DestroySurface(s);
    return ok;
}

static bool make_gradient(const char* name, int w, int h,
                          uint8_t r1, uint8_t g1, uint8_t b1,
                          uint8_t r2, uint8_t g2, uint8_t b2) {
    SDL_Surface* s = SDL_CreateSurface(w, h, SDL_PIXELFORMAT_RGBA32);
    if (!s) return false;
    for (int y = 0; y < h; ++y) {
        float t = static_cast<float>(y) / static_cast<float>(h - 1);
        uint8_t r = static_cast<uint8_t>(r1 + (r2 - r1) * t);
        uint8_t g = static_cast<uint8_t>(g1 + (g2 - g1) * t);
        uint8_t b = static_cast<uint8_t>(b1 + (b2 - b1) * t);
        SDL_Rect row{0, y, w, 1};
        SDL_FillSurfaceRect(s, &row, SDL_MapSurfaceRGBA(s, r, g, b, 255));
    }
    bool ok = IMG_SavePNG(s, name);
    SDL_DestroySurface(s);
    return ok;
}

int main() {
    SDL_Init(0);

    // Background: dark blue gradient 640x480
    make_gradient("title_bg.png", 640, 480, 10, 10, 60, 10, 10, 120);

    // Logo: white rectangle 240x80
    make_solid("logo.png", 240, 80, 255, 255, 255);

    // "Press Start" text placeholder: bright cyan 200x30
    make_solid("press_start.png", 200, 30, 0, 255, 255);

    // Colored frames for ANI and general testing (64x64)
    make_solid("frame_red.png", 64, 64, 255, 0, 0);
    make_solid("frame_green.png", 64, 64, 0, 255, 0);
    make_solid("frame_blue.png", 64, 64, 0, 0, 255);
    make_solid("frame_yellow.png", 64, 64, 255, 255, 0);

    // Regression test textures
    make_solid("white.png", 64, 64, 255, 255, 255);
    make_solid("gray128.png", 64, 64, 128, 128, 128);

    std::printf("Sample textures generated.\n");
    SDL_Quit();
    return 0;
}
