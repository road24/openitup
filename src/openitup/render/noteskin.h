#pragma once

#include <array>
#include <filesystem>
#include <memory>
#include <string>

#include <openitup/sprite/sprite.h>

namespace openitup {

// Number of tracks in a single side (always 5 for PIU).
inline constexpr int NUM_TRACKS = 5;

// Hold sub-parts.
enum class HoldPart : uint8_t { HEAD = 0, BODY = 1, TAIL = 2 };

// Play mode for receptor sprites.
enum class PlayMode : uint8_t { SINGLE = 0, DOUBLE = 1, HALF = 2 };

class NoteSkin {
public:
    // The directory this skin was loaded from.
    std::filesystem::path directory;

    // The skin name (directory stem, e.g. "default").
    std::string name;

    // --- Per-track sprites (5 entries each, indexed by track 0-4) ---

    // TAP arrows.
    const Sprite* tap(int track) const;

    // FAKETAP arrows.
    const Sprite* faketap(int track) const;

    // LONG hold note parts.
    const Sprite* hold(int track, HoldPart part) const;

    // OTHER division mode notes.
    const Sprite* other_w(int track) const;
    const Sprite* other_g(int track) const;

    // PRESS overlay (one-shot, triggered on panel press).
    const Sprite* press(int track) const;

    // JUDGE overlay (one-shot, triggered on Perfect/Great/Good).
    const Sprite* judge(int track) const;

    // --- Receptor sprites (one per play mode) ---

    const Sprite* receptor(PlayMode mode) const;

    // --- Judgment tier sprites (one per tier) ---

    const Sprite* judgment_perfect() const;
    const Sprite* judgment_great() const;
    const Sprite* judgment_good() const;
    const Sprite* judgment_bad() const;
    const Sprite* judgment_miss() const;

    // --- Completeness query ---

    // True if all Phase 2 required sprites are loaded (all TAP, LONG, etc.).
    bool is_complete() const;

    // Count of successfully loaded sprites.
    int loaded_count() const;

    // Total expected for Phase 2 (48).
    static constexpr int EXPECTED_COUNT = 48;

private:
    // NoteSkinLoader is the only class that populates these.
    friend class NoteSkinLoader;

    std::array<std::unique_ptr<Sprite>, NUM_TRACKS> tap_;
    std::array<std::unique_ptr<Sprite>, NUM_TRACKS> faketap_;
    std::array<std::unique_ptr<Sprite>, NUM_TRACKS> hold_head_;
    std::array<std::unique_ptr<Sprite>, NUM_TRACKS> hold_body_;
    std::array<std::unique_ptr<Sprite>, NUM_TRACKS> hold_tail_;
    std::array<std::unique_ptr<Sprite>, NUM_TRACKS> other_w_;
    std::array<std::unique_ptr<Sprite>, NUM_TRACKS> other_g_;
    std::array<std::unique_ptr<Sprite>, NUM_TRACKS> press_;
    std::array<std::unique_ptr<Sprite>, NUM_TRACKS> judge_;
    std::array<std::unique_ptr<Sprite>, 3> receptor_;  // SINGLE, DOUBLE, HALF

    // Judgment tier sprites
    std::unique_ptr<Sprite> judgment_perfect_;
    std::unique_ptr<Sprite> judgment_great_;
    std::unique_ptr<Sprite> judgment_good_;
    std::unique_ptr<Sprite> judgment_bad_;
    std::unique_ptr<Sprite> judgment_miss_;
};

} // namespace openitup
