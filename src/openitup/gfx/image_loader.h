#pragma once

#include <filesystem>
#include <SDL3/SDL.h>

namespace openitup {

// Thin wrapper isolating the image-loading dependency (SDL3_image).
// If SDL3_image needs to be replaced, only image_loader.cpp changes.
SDL_Surface* load_image(const std::filesystem::path& path);

} // namespace openitup
