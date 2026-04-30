#include <openitup/render/noteskin.h>

namespace openitup {

const Sprite* NoteSkin::tap(int track) const {
    if (track < 0 || track >= NUM_TRACKS) return nullptr;
    return tap_[track].get();
}

const Sprite* NoteSkin::faketap(int track) const {
    if (track < 0 || track >= NUM_TRACKS) return nullptr;
    return faketap_[track].get();
}

const Sprite* NoteSkin::hold(int track, HoldPart part) const {
    if (track < 0 || track >= NUM_TRACKS) return nullptr;
    switch (part) {
        case HoldPart::HEAD: return hold_head_[track].get();
        case HoldPart::BODY: return hold_body_[track].get();
        case HoldPart::TAIL: return hold_tail_[track].get();
    }
    return nullptr;
}

const Sprite* NoteSkin::other_w(int track) const {
    if (track < 0 || track >= NUM_TRACKS) return nullptr;
    return other_w_[track].get();
}

const Sprite* NoteSkin::other_g(int track) const {
    if (track < 0 || track >= NUM_TRACKS) return nullptr;
    return other_g_[track].get();
}

const Sprite* NoteSkin::press(int track) const {
    if (track < 0 || track >= NUM_TRACKS) return nullptr;
    return press_[track].get();
}

const Sprite* NoteSkin::judge(int track) const {
    if (track < 0 || track >= NUM_TRACKS) return nullptr;
    return judge_[track].get();
}

const Sprite* NoteSkin::receptor(PlayMode mode) const {
    int index = static_cast<int>(mode);
    if (index < 0 || index >= 3) return nullptr;
    return receptor_[index].get();
}

const Sprite* NoteSkin::judgment_perfect() const {
    return judgment_perfect_.get();
}

const Sprite* NoteSkin::judgment_great() const {
    return judgment_great_.get();
}

const Sprite* NoteSkin::judgment_good() const {
    return judgment_good_.get();
}

const Sprite* NoteSkin::judgment_bad() const {
    return judgment_bad_.get();
}

const Sprite* NoteSkin::judgment_miss() const {
    return judgment_miss_.get();
}

bool NoteSkin::is_complete() const {
    return loaded_count() == EXPECTED_COUNT;
}

int NoteSkin::loaded_count() const {
    int count = 0;

    // Count all per-track sprites
    for (const auto& ptr : tap_) if (ptr) ++count;
    for (const auto& ptr : faketap_) if (ptr) ++count;
    for (const auto& ptr : hold_head_) if (ptr) ++count;
    for (const auto& ptr : hold_body_) if (ptr) ++count;
    for (const auto& ptr : hold_tail_) if (ptr) ++count;
    for (const auto& ptr : other_w_) if (ptr) ++count;
    for (const auto& ptr : other_g_) if (ptr) ++count;
    for (const auto& ptr : press_) if (ptr) ++count;
    for (const auto& ptr : judge_) if (ptr) ++count;

    // Count receptor sprites
    for (const auto& ptr : receptor_) if (ptr) ++count;

    return count;
}

} // namespace openitup
