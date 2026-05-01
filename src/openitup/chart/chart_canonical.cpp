#include <openitup/chart/chart_canonical.h>

#include <openitup/chart/note_data.h>
#include <openitup/chart/timing_data.h>

#include <cstring>
#include <algorithm>

namespace openitup {

namespace {

// Write a value to the buffer in little-endian byte order.
template<typename T>
void write_le(std::vector<uint8_t>& buf, T value) {
    uint8_t bytes[sizeof(T)];
    std::memcpy(bytes, &value, sizeof(T));

    // On little-endian systems, this is a no-op.
    // On big-endian systems, we'd need to reverse the bytes.
    // For simplicity, assume IEEE 754 doubles and little-endian integers.
    // (Cross-platform code would check endianness at compile time.)

    for (size_t i = 0; i < sizeof(T); ++i) {
        buf.push_back(bytes[i]);
    }
}

} // anonymous namespace

std::vector<uint8_t> canonical_bytes(const NoteData& notes, const TimingData& timing) {
    std::vector<uint8_t> result;

    // Reserve approximate size to reduce reallocations:
    // 4 bytes (note count) + notes.size() * 10 + 4 bytes (timing count) + timing.size() * 17
    result.reserve(8 + notes.size() * 10 + timing.size() * 17);

    // --- Serialize notes ---

    // Copy and sort events to ensure canonical order (beat, column, type).
    // NoteData may contain unsorted events if constructed directly without ChartBuilder.
    auto note_events = notes.events();
    std::sort(note_events.begin(), note_events.end());

    uint32_t note_count = static_cast<uint32_t>(note_events.size());
    write_le(result, note_count);

    for (const auto& note : note_events) {
        write_le(result, note.beat);                           // 8 bytes
        write_le(result, note.column);                         // 1 byte
        write_le(result, static_cast<uint8_t>(note.type));     // 1 byte
    }

    // --- Serialize timing events ---

    // TimingData also keeps events sorted, so we can iterate directly.
    const auto& timing_events = timing.events();

    uint32_t timing_count = static_cast<uint32_t>(timing_events.size());
    write_le(result, timing_count);

    for (const auto& event : timing_events) {
        write_le(result, event.beat);                          // 8 bytes
        write_le(result, static_cast<uint8_t>(event.type));    // 1 byte

        // Value depends on event type:
        // For BPM_CHANGE: bpm
        // For STOP: stop_duration
        double value = (event.type == TimingEventType::BPM_CHANGE)
                       ? event.bpm
                       : event.stop_duration;
        write_le(result, value);                               // 8 bytes
    }

    return result;
}

} // namespace openitup
