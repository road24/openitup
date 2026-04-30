#include <openitup/render/noteskin_loader.h>

#include <format>
#include <spdlog/spdlog.h>

#include <openitup/sprite/sprite_loader.h>

namespace openitup {

std::unique_ptr<Sprite> NoteSkinLoader::try_load_sprj(
    const std::filesystem::path& skin_dir,
    const std::string& filename,
    TextureCache& cache) {

    std::filesystem::path path = skin_dir / filename;

    if (!std::filesystem::exists(path)) {
        spdlog::warn("NoteSkin: missing {}", filename);
        return nullptr;
    }

    try {
        return load_sprj(path, cache);
    } catch (const std::exception& e) {
        spdlog::warn("NoteSkin: failed to load {}: {}", filename, e.what());
        return nullptr;
    }
}

std::unique_ptr<NoteSkin> NoteSkinLoader::load(
    const std::filesystem::path& skin_dir,
    TextureCache& cache) {

    // Verify directory exists
    if (!std::filesystem::exists(skin_dir) || !std::filesystem::is_directory(skin_dir)) {
        throw std::runtime_error(
            std::format("NoteSkin directory does not exist: {}", skin_dir.string()));
    }

    auto skin = std::make_unique<NoteSkin>();
    skin->directory = skin_dir;
    skin->name = skin_dir.filename().string();

    // Load sprites for each track (0-4)
    for (int track = 0; track < NUM_TRACKS; ++track) {
        // TAP arrows
        skin->tap_[track] = try_load_sprj(
            skin_dir, std::format("ARROW{:02d}_TAP.sprj", track), cache);

        // FAKETAP arrows
        skin->faketap_[track] = try_load_sprj(
            skin_dir, std::format("ARROW{:02d}_FAKETAP.sprj", track), cache);

        // LONG hold note parts
        skin->hold_head_[track] = try_load_sprj(
            skin_dir, std::format("ARROW{:02d}_LONG_HEAD.sprj", track), cache);
        skin->hold_body_[track] = try_load_sprj(
            skin_dir, std::format("ARROW{:02d}_LONG_BODY.sprj", track), cache);
        skin->hold_tail_[track] = try_load_sprj(
            skin_dir, std::format("ARROW{:02d}_LONG_TAIL.sprj", track), cache);

        // OTHER division mode notes
        skin->other_w_[track] = try_load_sprj(
            skin_dir, std::format("ARROW{:02d}_OTHER_W.sprj", track), cache);
        skin->other_g_[track] = try_load_sprj(
            skin_dir, std::format("ARROW{:02d}_OTHER_G.sprj", track), cache);

        // PRESS overlay
        skin->press_[track] = try_load_sprj(
            skin_dir, std::format("ARROW{:02d}_PRESS.sprj", track), cache);

        // JUDGE overlay
        skin->judge_[track] = try_load_sprj(
            skin_dir, std::format("ARROW{:02d}_JUDGE.sprj", track), cache);
    }

    // Load receptor sprites
    skin->receptor_[static_cast<size_t>(PlayMode::SINGLE)] =
        try_load_sprj(skin_dir, "ARROW_RECEPTOR_SINGLE.sprj", cache);
    skin->receptor_[static_cast<size_t>(PlayMode::DOUBLE)] =
        try_load_sprj(skin_dir, "ARROW_RECEPTOR_DOUBLE.sprj", cache);
    skin->receptor_[static_cast<size_t>(PlayMode::HALF)] =
        try_load_sprj(skin_dir, "ARROW_RECEPTOR_HALF.sprj", cache);

    // Load judgment tier sprites
    skin->judgment_perfect_ = try_load_sprj(skin_dir, "judge-perfect.sprj", cache);
    skin->judgment_great_ = try_load_sprj(skin_dir, "judge-great.sprj", cache);
    skin->judgment_good_ = try_load_sprj(skin_dir, "judge-good.sprj", cache);
    skin->judgment_bad_ = try_load_sprj(skin_dir, "judge-bad.sprj", cache);
    skin->judgment_miss_ = try_load_sprj(skin_dir, "judge-miss.sprj", cache);

    spdlog::info("NoteSkin '{}' loaded: {}/{} sprites",
                 skin->name, skin->loaded_count(), NoteSkin::EXPECTED_COUNT);

    return skin;
}

std::unique_ptr<NoteSkin> NoteSkinLoader::load_with_fallback(
    const std::filesystem::path& skin_dir,
    const std::filesystem::path& fallback_dir,
    TextureCache& cache) {

    // Try primary skin first
    try {
        return load(skin_dir, cache);
    } catch (const std::exception& e) {
        spdlog::warn("NoteSkin '{}' failed to load: {}, trying fallback '{}'",
                     skin_dir.filename().string(), e.what(),
                     fallback_dir.filename().string());
    }

    // Try fallback
    try {
        auto skin = load(fallback_dir, cache);

        // Fill in missing sprites from primary skin (if it partially loaded)
        // For now, just return the fallback as-is since primary failed completely

        return skin;
    } catch (const std::exception& e) {
        throw std::runtime_error(
            std::format("Both primary skin '{}' and fallback skin '{}' failed to load",
                        skin_dir.filename().string(),
                        fallback_dir.filename().string()));
    }
}

} // namespace openitup
