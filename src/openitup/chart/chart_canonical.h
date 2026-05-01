#pragma once

#include <vector>
#include <cstdint>

namespace openitup {

class NoteData;
class TimingData;

// Serialize chart content (notes + timing) to a deterministic binary format.
// This format is used for SHA-256 hashing to produce stable chart identities
// across file formats and parsers.
//
// Format (little-endian):
//   - 4 bytes: number of notes (uint32)
//   - For each note (sorted by beat, then column):
//     - 8 bytes: beat (double, IEEE 754)
//     - 1 byte: column (uint8)
//     - 1 byte: note type (uint8)
//   - 4 bytes: number of timing events (uint32)
//   - For each timing event (sorted by beat):
//     - 8 bytes: beat (double)
//     - 1 byte: event type (uint8)
//     - 8 bytes: value (double) — bpm for BPM_CHANGE, stop_duration for STOP
//
// Notes and timing events MUST be sorted before serialization to ensure
// deterministic output. The function will sort them if needed.
std::vector<uint8_t> canonical_bytes(const NoteData& notes, const TimingData& timing);

} // namespace openitup
