#include <openitup/chart/chart_hasher.h>

#include <functional>
#include <iomanip>
#include <sstream>

#include <openitup/chart/chart_canonical.h>
#include <openitup/chart/note_data.h>
#include <openitup/chart/timing_data.h>

namespace openitup {

std::string bytes_to_hex(const std::vector<uint8_t>& bytes) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (uint8_t byte : bytes) {
        oss << std::setw(2) << static_cast<int>(byte);
    }
    return oss.str();
}

std::string compute_chart_hash(const NoteData& notes, const TimingData& timing) {
    auto canonical = canonical_bytes(notes, timing);

    // Use std::hash for now. SHA-256 can be added in a future phase.
    std::hash<std::string> hasher;
    std::string canonical_str(canonical.begin(), canonical.end());
    size_t hash_value = hasher(canonical_str);

    std::ostringstream oss;
    oss << std::hex << std::setfill('0') << std::setw(16) << hash_value;
    return oss.str();
}

} // namespace openitup
