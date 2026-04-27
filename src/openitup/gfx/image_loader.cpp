#include <openitup/gfx/image_loader.h>
#include <SDL3_image/SDL_image.h>
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace openitup {

SDL_Surface* load_image(const std::filesystem::path& path) {
    SDL_Surface* surface = IMG_Load(path.string().c_str());
    if (!surface) {
        spdlog::error("IMG_Load failed for '{}': {}", path.string(), SDL_GetError());
        throw std::runtime_error(
            "Failed to load image '" + path.string() + "': " + SDL_GetError());
    }
    return surface;
}

} // namespace openitup
