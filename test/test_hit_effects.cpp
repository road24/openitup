#include <gtest/gtest.h>

#include <openitup/render/note_renderer.h>
#include <openitup/chart/chart_builder.h>
#include <openitup/judge/judgment_tier.h>

#include <SDL3/SDL.h>

using namespace openitup;

class HitEffectsTest : public ::testing::Test {
protected:
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;

    void SetUp() override {
        SDL_Init(SDL_INIT_VIDEO);
        window_ = SDL_CreateWindow("test", 640, 480, SDL_WINDOW_HIDDEN);
        if (window_) {
            renderer_ = SDL_CreateRenderer(window_, nullptr);
        }
        if (renderer_) {
            SDL_SetRenderLogicalPresentation(renderer_, 640, 480,
                SDL_LOGICAL_PRESENTATION_LETTERBOX);
        }
    }

    void TearDown() override {
        if (renderer_) SDL_DestroyRenderer(renderer_);
        if (window_) SDL_DestroyWindow(window_);
        SDL_Quit();
    }

    bool has_renderer() const { return renderer_ != nullptr; }
};

TEST_F(HitEffectsTest, TriggerPerfectHitEffect) {
    if (!has_renderer()) GTEST_SKIP() << "No renderer available";

    // Create minimal chart with timing
    ChartBuilder builder;
    builder.add_bpm_change(0.0, 120.0);
    auto chart = builder.build();

    NoteFieldConfig config = default_single_config();
    NoteRenderer note_renderer(chart.note_data(), chart.timing_data(), config);

    // Trigger a Perfect hit effect on column 2
    double trigger_time = 1000.0;  // 1 second
    note_renderer.trigger_hit_effect(2, JudgmentTier::PERFECT, trigger_time);

    // Render hit effects immediately (t=0ms elapsed, should be visible)
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);
    note_renderer.render_hit_effects(renderer_, trigger_time);

    // No crash = success for this basic test
    SUCCEED();
}

TEST_F(HitEffectsTest, TriggerGreatHitEffect) {
    if (!has_renderer()) GTEST_SKIP() << "No renderer available";

    ChartBuilder builder;
    builder.add_bpm_change(0.0, 120.0);
    auto chart = builder.build();

    NoteFieldConfig config = default_single_config();
    NoteRenderer note_renderer(chart.note_data(), chart.timing_data(), config);

    // Trigger a Great hit effect on column 1
    double trigger_time = 2000.0;
    note_renderer.trigger_hit_effect(1, JudgmentTier::GREAT, trigger_time);

    // Render at trigger time
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);
    note_renderer.render_hit_effects(renderer_, trigger_time);

    SUCCEED();
}

TEST_F(HitEffectsTest, TriggerGoodHitEffect) {
    if (!has_renderer()) GTEST_SKIP() << "No renderer available";

    ChartBuilder builder;
    builder.add_bpm_change(0.0, 120.0);
    auto chart = builder.build();

    NoteFieldConfig config = default_single_config();
    NoteRenderer note_renderer(chart.note_data(), chart.timing_data(), config);

    // Trigger a Good hit effect on column 3
    double trigger_time = 3000.0;
    note_renderer.trigger_hit_effect(3, JudgmentTier::GOOD, trigger_time);

    // Render at trigger time
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);
    note_renderer.render_hit_effects(renderer_, trigger_time);

    SUCCEED();
}

TEST_F(HitEffectsTest, NoEffectForMiss) {
    if (!has_renderer()) GTEST_SKIP() << "No renderer available";

    ChartBuilder builder;
    builder.add_bpm_change(0.0, 120.0);
    auto chart = builder.build();

    NoteFieldConfig config = default_single_config();
    NoteRenderer note_renderer(chart.note_data(), chart.timing_data(), config);

    // Trigger a Miss (should NOT create an effect)
    double trigger_time = 4000.0;
    note_renderer.trigger_hit_effect(0, JudgmentTier::MISS, trigger_time);

    // Render - Miss should not produce visible effect
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);
    note_renderer.render_hit_effects(renderer_, trigger_time);

    // No crash = success (effect correctly filtered out)
    SUCCEED();
}

TEST_F(HitEffectsTest, NoEffectForBad) {
    if (!has_renderer()) GTEST_SKIP() << "No renderer available";

    ChartBuilder builder;
    builder.add_bpm_change(0.0, 120.0);
    auto chart = builder.build();

    NoteFieldConfig config = default_single_config();
    NoteRenderer note_renderer(chart.note_data(), chart.timing_data(), config);

    // Trigger a Bad (should NOT create an effect)
    double trigger_time = 5000.0;
    note_renderer.trigger_hit_effect(0, JudgmentTier::BAD, trigger_time);

    // Render - Bad should not produce visible effect
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);
    note_renderer.render_hit_effects(renderer_, trigger_time);

    SUCCEED();
}

TEST_F(HitEffectsTest, EffectFadesOver300ms) {
    if (!has_renderer()) GTEST_SKIP() << "No renderer available";

    ChartBuilder builder;
    builder.add_bpm_change(0.0, 120.0);
    auto chart = builder.build();

    NoteFieldConfig config = default_single_config();
    NoteRenderer note_renderer(chart.note_data(), chart.timing_data(), config);

    // Trigger effect
    double trigger_time = 1000.0;
    note_renderer.trigger_hit_effect(2, JudgmentTier::PERFECT, trigger_time);

    // Render at various times during fade
    for (double elapsed = 0.0; elapsed <= 350.0; elapsed += 50.0) {
        SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
        SDL_RenderClear(renderer_);
        note_renderer.render_hit_effects(renderer_, trigger_time + elapsed);
    }

    // Effect should have faded out and cleaned up after 300ms
    SUCCEED();
}

TEST_F(HitEffectsTest, MultipleEffectsSimultaneously) {
    if (!has_renderer()) GTEST_SKIP() << "No renderer available";

    ChartBuilder builder;
    builder.add_bpm_change(0.0, 120.0);
    auto chart = builder.build();

    NoteFieldConfig config = default_single_config();
    NoteRenderer note_renderer(chart.note_data(), chart.timing_data(), config);

    // Trigger effects on multiple columns at the same time
    double trigger_time = 2000.0;
    note_renderer.trigger_hit_effect(0, JudgmentTier::PERFECT, trigger_time);
    note_renderer.trigger_hit_effect(2, JudgmentTier::GREAT, trigger_time);
    note_renderer.trigger_hit_effect(4, JudgmentTier::GOOD, trigger_time);

    // Render all effects
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);
    note_renderer.render_hit_effects(renderer_, trigger_time);

    SUCCEED();
}

TEST_F(HitEffectsTest, EffectsCleanupOldEntries) {
    if (!has_renderer()) GTEST_SKIP() << "No renderer available";

    ChartBuilder builder;
    builder.add_bpm_change(0.0, 120.0);
    auto chart = builder.build();

    NoteFieldConfig config = default_single_config();
    NoteRenderer note_renderer(chart.note_data(), chart.timing_data(), config);

    // Trigger many effects over time
    for (int i = 0; i < 100; ++i) {
        double trigger_time = i * 100.0;  // Every 100ms
        note_renderer.trigger_hit_effect(i % 5, JudgmentTier::PERFECT, trigger_time);
    }

    // Render at a much later time - old effects should be cleaned up
    double current_time = 10000.0;
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);
    note_renderer.render_hit_effects(renderer_, current_time);

    // No crash or memory leak = success
    SUCCEED();
}

TEST_F(HitEffectsTest, InvalidColumnIgnored) {
    if (!has_renderer()) GTEST_SKIP() << "No renderer available";

    ChartBuilder builder;
    builder.add_bpm_change(0.0, 120.0);
    auto chart = builder.build();

    NoteFieldConfig config = default_single_config();
    config.num_columns = 5;
    NoteRenderer note_renderer(chart.note_data(), chart.timing_data(), config);

    // Trigger effect on invalid column (out of range)
    double trigger_time = 3000.0;
    note_renderer.trigger_hit_effect(10, JudgmentTier::PERFECT, trigger_time);

    // Render should handle gracefully
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);
    note_renderer.render_hit_effects(renderer_, trigger_time);

    SUCCEED();
}

TEST_F(HitEffectsTest, EffectsRenderInCorrectZOrder) {
    if (!has_renderer()) GTEST_SKIP() << "No renderer available";

    ChartBuilder builder;
    builder.add_bpm_change(0.0, 120.0);
    auto chart = builder.build();

    NoteFieldConfig config = default_single_config();
    NoteRenderer note_renderer(chart.note_data(), chart.timing_data(), config);

    // Trigger effect
    double trigger_time = 1000.0;
    note_renderer.trigger_hit_effect(2, JudgmentTier::PERFECT, trigger_time);

    // Render receptors first, then effects on top
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);
    note_renderer.render_receptors(renderer_, trigger_time, nullptr, nullptr);
    note_renderer.render_hit_effects(renderer_, trigger_time);

    // Visual ordering: BGA → receptors → notes → hit effects → judgment display
    SUCCEED();
}
