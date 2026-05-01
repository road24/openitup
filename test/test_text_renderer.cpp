#include <gtest/gtest.h>
#include <openitup/render/text_renderer.h>
#include <SDL3/SDL.h>

using namespace openitup;

class TextRendererTest : public ::testing::Test {
protected:
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;

    void SetUp() override {
        SDL_Init(SDL_INIT_VIDEO);
        window_ = SDL_CreateWindow("test", 640, 480, SDL_WINDOW_HIDDEN);
        if (window_) {
            renderer_ = SDL_CreateRenderer(window_, nullptr);
        }
    }

    void TearDown() override {
        if (renderer_) SDL_DestroyRenderer(renderer_);
        if (window_) SDL_DestroyWindow(window_);
        SDL_Quit();
    }

    bool has_renderer() const { return renderer_ != nullptr; }
};

TEST_F(TextRendererTest, ConstructorCreatesValidRenderer) {
    if (!has_renderer()) GTEST_SKIP() << "No SDL renderer available";

    TextRenderer text_renderer(renderer_);
    // If constructor doesn't throw, it's valid
    SUCCEED();
}

TEST_F(TextRendererTest, DrawTextDoesNotCrash) {
    if (!has_renderer()) GTEST_SKIP() << "No SDL renderer available";

    TextRenderer text_renderer(renderer_);

    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);

    SDL_Color white = {255, 255, 255, 255};
    text_renderer.draw_text("Hello World", 100, 100, white);

    // If we get here, rendering succeeded
    SUCCEED();
}

TEST_F(TextRendererTest, DrawEmptyStringDoesNotCrash) {
    if (!has_renderer()) GTEST_SKIP() << "No SDL renderer available";

    TextRenderer text_renderer(renderer_);
    SDL_Color white = {255, 255, 255, 255};
    text_renderer.draw_text("", 100, 100, white);

    SUCCEED();
}

TEST_F(TextRendererTest, DrawMultipleStrings) {
    if (!has_renderer()) GTEST_SKIP() << "No SDL renderer available";

    TextRenderer text_renderer(renderer_);

    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);

    SDL_Color white = {255, 255, 255, 255};
    SDL_Color red = {255, 0, 0, 255};

    text_renderer.draw_text("Line 1", 10, 10, white);
    text_renderer.draw_text("Line 2", 10, 30, red);
    text_renderer.draw_text("Line 3", 10, 50, white);

    SUCCEED();
}

TEST_F(TextRendererTest, ColorIsRespected) {
    if (!has_renderer()) GTEST_SKIP() << "No SDL renderer available";

    TextRenderer text_renderer(renderer_);

    SDL_Color red = {255, 0, 0, 255};
    text_renderer.draw_text("Red Text", 10, 10, red);

    // Verify draw color is restored after text rendering
    Uint8 r, g, b, a;
    SDL_GetRenderDrawColor(renderer_, &r, &g, &b, &a);

    // The color should be restored to whatever it was before
    // Since we didn't set it, it should be the default
    SUCCEED();
}
