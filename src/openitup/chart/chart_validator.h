#pragma once

#include <vector>
#include <string>

#include <openitup/chart/chart.h>

namespace openitup {

enum class ValidationSeverity : uint8_t {
    WARNING = 0,  // Non-fatal issues that don't prevent playback
    ERROR = 1,    // Serious issues that may cause problems
};

struct ValidationWarning {
    ValidationSeverity severity;
    std::string message;
    double beat;        // -1.0 if not beat-specific
    int column;         // -1 if not column-specific

    ValidationWarning(ValidationSeverity sev, std::string msg, double b = -1.0, int col = -1)
        : severity(sev), message(std::move(msg)), beat(b), column(col) {}
};

class ChartValidator {
public:
    // Validate a chart for common errors.
    // Returns a vector of ValidationWarning structs.
    // Empty vector means chart is valid.
    // Warnings are permissive and do not prevent chart loading.
    static std::vector<ValidationWarning> validate(const Chart& chart);

private:
    // Individual validation checks
    static void check_negative_beat_positions(const Chart& chart, std::vector<ValidationWarning>& warnings);
    static void check_overlapping_notes(const Chart& chart, std::vector<ValidationWarning>& warnings);
    static void check_orphan_hold_notes(const Chart& chart, std::vector<ValidationWarning>& warnings);
    static void check_bpm_validity(const Chart& chart, std::vector<ValidationWarning>& warnings);
    static void check_has_notes(const Chart& chart, std::vector<ValidationWarning>& warnings);
    static void check_column_range(const Chart& chart, std::vector<ValidationWarning>& warnings);
};

} // namespace openitup
