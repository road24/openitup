#include <openitup/chart/chart_validator.h>

#include <unordered_map>
#include <spdlog/spdlog.h>

namespace openitup {

std::vector<ValidationWarning> ChartValidator::validate(const Chart& chart) {
    std::vector<ValidationWarning> warnings;

    check_negative_beat_positions(chart, warnings);
    check_overlapping_notes(chart, warnings);
    check_orphan_hold_notes(chart, warnings);
    check_bpm_validity(chart, warnings);
    check_has_notes(chart, warnings);
    check_column_range(chart, warnings);

    // Log all warnings
    for (const auto& warning : warnings) {
        if (warning.severity == ValidationSeverity::ERROR) {
            if (warning.beat >= 0.0 && warning.column >= 0) {
                spdlog::error("Chart validation: {} (beat {}, column {})", warning.message, warning.beat, warning.column);
            } else if (warning.beat >= 0.0) {
                spdlog::error("Chart validation: {} (beat {})", warning.message, warning.beat);
            } else {
                spdlog::error("Chart validation: {}", warning.message);
            }
        } else {
            if (warning.beat >= 0.0 && warning.column >= 0) {
                spdlog::warn("Chart validation: {} (beat {}, column {})", warning.message, warning.beat, warning.column);
            } else if (warning.beat >= 0.0) {
                spdlog::warn("Chart validation: {} (beat {})", warning.message, warning.beat);
            } else {
                spdlog::warn("Chart validation: {}", warning.message);
            }
        }
    }

    return warnings;
}

void ChartValidator::check_negative_beat_positions(const Chart& chart, std::vector<ValidationWarning>& warnings) {
    const auto& events = chart.note_data().events();
    for (const auto& event : events) {
        if (event.beat < 0.0) {
            warnings.emplace_back(ValidationSeverity::ERROR,
                                  "Invalid negative beat position",
                                  event.beat,
                                  event.column);
        }
    }
}

void ChartValidator::check_overlapping_notes(const Chart& chart, std::vector<ValidationWarning>& warnings) {
    const auto& events = chart.note_data().events();

    // Group notes by (beat, column) and check for duplicates
    for (std::size_t i = 0; i < events.size(); ++i) {
        for (std::size_t j = i + 1; j < events.size(); ++j) {
            // Events are sorted by beat, so we can break when beat changes
            if (events[j].beat != events[i].beat) {
                break;
            }

            if (events[j].column == events[i].column) {
                warnings.emplace_back(ValidationSeverity::WARNING,
                                      "Overlapping notes in same column",
                                      events[i].beat,
                                      events[i].column);
                break;  // Only report once per position
            }
        }
    }
}

void ChartValidator::check_orphan_hold_notes(const Chart& chart, std::vector<ValidationWarning>& warnings) {
    const auto& events = chart.note_data().events();

    // Track active hold heads per column
    std::unordered_map<int, double> active_holds;  // column -> beat of hold_head

    for (const auto& event : events) {
        if (event.type == NoteType::HOLD_HEAD) {
            if (active_holds.count(event.column)) {
                // Previous hold head has no tail
                double orphan_beat = active_holds[event.column];
                warnings.emplace_back(ValidationSeverity::WARNING,
                                      "Hold note has no tail",
                                      orphan_beat,
                                      event.column);
            }
            active_holds[event.column] = event.beat;
        } else if (event.type == NoteType::HOLD_TAIL) {
            if (!active_holds.count(event.column)) {
                // Hold tail without preceding head
                warnings.emplace_back(ValidationSeverity::WARNING,
                                      "Hold tail without preceding hold head",
                                      event.beat,
                                      event.column);
            } else {
                // Valid hold, remove from active
                active_holds.erase(event.column);
            }
        }
    }

    // Check for any remaining active holds (no tail found)
    for (const auto& [column, beat] : active_holds) {
        warnings.emplace_back(ValidationSeverity::WARNING,
                              "Hold note has no tail",
                              beat,
                              column);
    }
}

void ChartValidator::check_bpm_validity(const Chart& chart, std::vector<ValidationWarning>& warnings) {
    const auto& events = chart.timing_data().events();

    for (const auto& event : events) {
        if (event.type == TimingEventType::BPM_CHANGE) {
            if (event.bpm <= 0.0) {
                warnings.emplace_back(ValidationSeverity::ERROR,
                                      "Invalid BPM value: " + std::to_string(event.bpm),
                                      event.beat);
            } else if (event.bpm > 999.0) {
                warnings.emplace_back(ValidationSeverity::WARNING,
                                      "Unreasonable BPM value: " + std::to_string(event.bpm),
                                      event.beat);
            }
        }
    }
}

void ChartValidator::check_has_notes(const Chart& chart, std::vector<ValidationWarning>& warnings) {
    if (chart.note_data().empty()) {
        warnings.emplace_back(ValidationSeverity::ERROR, "Chart has no notes");
    }
}

void ChartValidator::check_column_range(const Chart& chart, std::vector<ValidationWarning>& warnings) {
    const auto& events = chart.note_data().events();
    const PlayMode mode = chart.metadata().mode;
    const int max_col = max_columns(mode);

    for (const auto& event : events) {
        if (event.column >= max_col) {
            warnings.emplace_back(ValidationSeverity::WARNING,
                                  "Column index out of range for play mode (max " + std::to_string(max_col - 1) + ")",
                                  event.beat,
                                  event.column);
        }
    }
}

} // namespace openitup
