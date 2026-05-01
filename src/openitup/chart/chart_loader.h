#pragma once

#include <string>
#include <filesystem>
#include <vector>

#include <openitup/chart/chart.h>

namespace openitup {

// Chart loader factory.
// Detects chart format by file extension and dispatches to the appropriate parser.
// Handles both single-chart formats (KSF, OSF) and multi-chart formats (SSC, SMA).
class ChartLoader {
public:
    ChartLoader() = default;

    // Load chart(s) from a file.
    // Detects format by extension and dispatches to the correct parser.
    // Returns a vector of Chart objects (1 for single-chart formats, N for multi-chart).
    // Throws ChartLoadException on parse errors.
    // Returns empty vector if format is unsupported or file has no compatible charts.
    std::vector<Chart> load(const std::filesystem::path& chart_path) const;

    // Check if a file extension is supported.
    static bool is_supported_extension(const std::string& ext);

private:
    // Load single-chart formats (KSF, OSF)
    std::vector<Chart> load_ksf(const std::filesystem::path& chart_path) const;
    std::vector<Chart> load_osf(const std::filesystem::path& chart_path) const;

    // Load multi-chart formats (SMA, SEE)
    std::vector<Chart> load_sma(const std::filesystem::path& chart_path) const;
    std::vector<Chart> load_see(const std::filesystem::path& chart_path) const;
};

} // namespace openitup
