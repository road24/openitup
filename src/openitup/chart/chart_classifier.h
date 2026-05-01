#pragma once

#include <openitup/chart/chart.h>
#include <openitup/chart/play_mode.h>

namespace openitup {

class ChartClassifier {
public:
    // Automatically classify play mode based on which columns are used.
    // SINGLE if only columns 0-4 are used.
    // DOUBLE if any column 5-9 is used.
    static PlayMode auto_classify_mode(const NoteData& note_data);

    // Estimate difficulty rating (1-28 scale) based on note density.
    // Uses peak notes-per-second (NPS) over a sliding 4-second window.
    // NPS brackets:
    //   <2 NPS   = 1-5   (Easy)
    //   2-4 NPS  = 6-10  (Normal)
    //   4-6 NPS  = 11-15 (Hard)
    //   6-8 NPS  = 16-20 (Crazy)
    //   8+ NPS   = 21-28 (Freestyle)
    static int estimate_difficulty(const NoteData& note_data, const TimingData& timing_data);

private:
    // Find the maximum column index used in the chart
    static int find_max_column(const NoteData& note_data);

    // Calculate peak NPS over a sliding window
    static double calculate_peak_nps(const NoteData& note_data, const TimingData& timing_data);
};

} // namespace openitup
