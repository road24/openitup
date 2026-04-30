#pragma once

#include <string>

#include <openitup/chart/play_mode.h>

namespace openitup {

struct ChartMetadata {
    // Display info
    std::string title;
    std::string artist;
    std::string genre;
    std::string charter_name;

    // Difficulty
    std::string difficulty_name;   // "Easy", "Normal", "Hard", "Crazy", etc.
    int difficulty_rating = 0;     // numeric rating (1-28 classic, higher modern)

    // Play mode
    PlayMode mode = PlayMode::SINGLE;

    // File paths (relative to chart directory, resolved by parser)
    std::string audio_path;        // primary audio file (SONGFILE / AUDIOFILE)
    std::string intro_path;        // demo/intro music (INTROFILE)
    std::string banner_path;       // song banner image
    std::string background_path;   // background image

    // Display BPM (what the player sees; may differ from actual timing)
    double display_bpm = 0.0;

    // Preview audio (Phase 3, but included here to avoid struct changes later)
    double preview_start_seconds = -1.0;   // -1.0 = not set
    double preview_length_seconds = -1.0;  // -1.0 = not set, default 15.0
};

} // namespace openitup
