#include <openitup/chart/chart_loader.h>

#include <algorithm>
#include <spdlog/spdlog.h>

#include <openitup/chart/chart_builder.h>
#include <openitup/chart/ksf_parser.h>
#include <openitup/chart/sma_parser.h>
#include <openitup/chart/see_parser.h>
#include <openitup/chart/osf_parser.h>

namespace openitup {

namespace {

// Convert extension to lowercase
std::string normalize_extension(const std::string& ext) {
    std::string normalized = ext;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return normalized;
}

} // anonymous namespace

bool ChartLoader::is_supported_extension(const std::string& ext) {
    std::string norm = normalize_extension(ext);
    return norm == ".ksf" || norm == ".sma" || norm == ".see" || norm == ".osf";
    // SSC, STX, NX not yet implemented but listed in story
    // || norm == ".ssc" || norm == ".stx" || norm == ".nx"
}

std::vector<Chart> ChartLoader::load_ksf(const std::filesystem::path& chart_path) const {
    KsfParser parser;
    Chart chart = parser.parse(chart_path);
    return {chart};
}

std::vector<Chart> ChartLoader::load_osf(const std::filesystem::path& chart_path) const {
    OsfParser parser;
    Chart chart = parser.parse(chart_path);
    return {chart};
}

std::vector<Chart> ChartLoader::load_sma(const std::filesystem::path& chart_path) const {
    SmaParser parser;
    return parser.parse(chart_path);
}

std::vector<Chart> ChartLoader::load_see(const std::filesystem::path& chart_path) const {
    SeeParser parser;
    return parser.parse(chart_path);
}

std::vector<Chart> ChartLoader::load(const std::filesystem::path& chart_path) const {
    if (!std::filesystem::exists(chart_path)) {
        throw ChartLoadException("Chart file does not exist: " + chart_path.string());
    }

    std::string ext = chart_path.extension().string();
    std::string norm_ext = normalize_extension(ext);

    if (norm_ext == ".ksf") {
        return load_ksf(chart_path);
    } else if (norm_ext == ".sma") {
        return load_sma(chart_path);
    } else if (norm_ext == ".see") {
        return load_see(chart_path);
    } else if (norm_ext == ".osf") {
        return load_osf(chart_path);
    } else {
        spdlog::warn("ChartLoader: unsupported file extension '{}' for file: {}",
                     ext, chart_path.string());
        return {};
    }
}

} // namespace openitup
