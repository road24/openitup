#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace openitup {

class NoteData;
class TimingData;

// Compute a SHA-256 content hash for chart data.
// The hash is computed from the canonical binary representation (see chart_canonical.h)
// and is stable across file formats and metadata changes.
//
// Returns: 64-character hexadecimal string (SHA-256 hash).
std::string compute_chart_hash(const NoteData& notes, const TimingData& timing);

// Helper: convert bytes to hex string
std::string bytes_to_hex(const std::vector<uint8_t>& bytes);

} // namespace openitup
