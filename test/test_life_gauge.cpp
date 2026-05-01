#include <gtest/gtest.h>

#include <openitup/render/life_gauge.h>

using namespace openitup;

// Note: LifeGauge rendering is integration-tested via MinimalGameplayScene.
// These tests validate construction and basic interface.

TEST(LifeGauge, ConstructsSuccessfully) {
    LifeGauge gauge;
    // If we get here, construction succeeded
    SUCCEED();
}

TEST(LifeGauge, RenderAcceptsValidHPRatios) {
    LifeGauge gauge;
    // Null renderer is acceptable for basic interface test
    // (actual rendering requires SDL initialization, tested in integration tests)

    // These should not crash (though they won't render anything with nullptr)
    gauge.render(nullptr, 0.0f);   // Empty
    gauge.render(nullptr, 0.25f);  // Red threshold
    gauge.render(nullptr, 0.5f);   // Yellow threshold
    gauge.render(nullptr, 0.75f);  // Green range
    gauge.render(nullptr, 1.0f);   // Full

    SUCCEED();
}

TEST(LifeGauge, RenderAcceptsOutOfRangeValues) {
    LifeGauge gauge;

    // Should clamp internally without crashing
    gauge.render(nullptr, -0.5f);  // Below 0
    gauge.render(nullptr, 1.5f);   // Above 1
    gauge.render(nullptr, 999.0f); // Way above 1

    SUCCEED();
}

// Color selection logic is implicitly tested by the render method
// but cannot be unit tested without SDL renderer initialization.
// Integration tests in test_minimal_gameplay_scene verify visual correctness.
