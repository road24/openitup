#pragma once

#include <map>
#include <optional>
#include <string>

#include <nlohmann/json.hpp>
#include <openitup/judge/timing_profile.h>

namespace openitup {

// Load a timing profile from a JSON file.
// Returns std::nullopt if the file cannot be loaded or is invalid.
std::optional<TimingProfile> load_timing_profile(const std::string& file_path);

// Load a timing profile from a JSON object.
// Returns std::nullopt if the JSON is invalid or fails validation.
std::optional<TimingProfile> load_timing_profile(const nlohmann::json& json);

// Get a map of built-in timing profiles by name.
// Keys: "exceed", "nx", "fiesta"
// Values: TimingProfile objects with historically-accurate timing windows.
std::map<std::string, TimingProfile> built_in_profiles();

} // namespace openitup
