cmake_minimum_required(VERSION 3.24)
project(openitup VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

# --- Dependencies via FetchContent ---

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        main
    GIT_SHALLOW    TRUE
)

FetchContent_Declare(
    SDL3_image
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
    GIT_TAG        main
    GIT_SHALLOW    TRUE
)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.15.2
    GIT_SHALLOW    TRUE
)

FetchContent_Declare(
    cli11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG        v2.4.2
    GIT_SHALLOW    TRUE
)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.15.3
    GIT_SHALLOW    TRUE
)

set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(SDL3IMAGE_INSTALL OFF CACHE BOOL "" FORCE)
set(SDL3IMAGE_DEPS_SHARED OFF CACHE BOOL "" FORCE)
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)

# SDL3 must be made available before SDL3_image so its targets exist
FetchContent_MakeAvailable(SDL3)
FetchContent_MakeAvailable(SDL3_image)
FetchContent_MakeAvailable(nlohmann_json googletest cli11 spdlog)

# --- Engine library ---

add_library(openitup_engine STATIC
    src/openitup/asset/data_directory.cpp
    src/openitup/core/clock.cpp
    src/openitup/core/engine.cpp
    src/openitup/input/pad_input.cpp
    src/openitup/input/input_snapshot.cpp
    src/openitup/input/input_system.cpp
    src/openitup/input/keyboard_driver.cpp
    src/openitup/bga/layer.cpp
    src/openitup/bga/animation.cpp
    src/openitup/bga/bga_loader.cpp
    src/openitup/gfx/texture_cache.cpp
    src/openitup/gfx/image_loader.cpp
    src/openitup/gfx/blend_modes.cpp
    src/openitup/gfx/renderer.cpp
    src/openitup/sprite/sprite.cpp
    src/openitup/sprite/sprite_loader.cpp
    src/openitup/chart/note_type.cpp
    src/openitup/chart/play_mode.cpp
    src/openitup/chart/note_data.cpp
    src/openitup/chart/timing_data.cpp
    src/openitup/chart/chart.cpp
    src/openitup/chart/chart_builder.cpp
    src/openitup/chart/ksf_parser.cpp
    src/openitup/judge/judgment_tier.cpp
    src/openitup/judge/timing_profile.cpp
    src/openitup/judge/judgment_event.cpp
    src/openitup/judge/judge.cpp
    src/openitup/judge/gameplay_state.cpp
    src/openitup/render/note_renderer.cpp
    src/openitup/render/judgment_display.cpp
    src/openitup/scene/minimal_gameplay_scene.cpp
)

target_include_directories(openitup_engine PUBLIC
    src
    third_party
)
target_link_libraries(openitup_engine PUBLIC
    SDL3::SDL3-static
    SDL3_image::SDL3_image-static
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    openitup_audio
)

# --- Audio library ---

add_library(openitup_audio STATIC
    src/openitup/audio/vorbis_decoder.cpp
    src/openitup/audio/mp3_decoder.cpp
    src/openitup/audio/sdl3_audio_system.cpp
)

target_include_directories(openitup_audio PUBLIC
    src
    third_party
)

target_link_libraries(openitup_audio PUBLIC
    SDL3::SDL3-static
    spdlog::spdlog
)

# --- Main executable ---

add_executable(openitup src/openitup/main.cpp)
target_link_libraries(openitup PRIVATE openitup_engine CLI11::CLI11)

# --- BGA Player tool ---

add_executable(bga_player tools/bga_player.cpp)
target_link_libraries(bga_player PRIVATE openitup_engine CLI11::CLI11)

# --- Format converter tools ---

add_executable(spr2sprj tools/spr2sprj.cpp)
target_link_libraries(spr2sprj PRIVATE openitup_engine CLI11::CLI11)

add_executable(bga2bgaj tools/bga2bgaj.cpp)
target_link_libraries(bga2bgaj PRIVATE nlohmann_json::nlohmann_json CLI11::CLI11)

# --- Sample fixture generator ---

add_executable(generate_samples test/fixtures/generate_samples.cpp)
target_link_libraries(generate_samples PRIVATE SDL3::SDL3-static SDL3_image::SDL3_image-static)

# --- Tests ---

enable_testing()

add_executable(openitup_tests
    test/test_data_directory.cpp
    test/test_keyframe_interp.cpp
    test/test_texture_cache.cpp
    test/test_sprite_modes.cpp
    test/test_integration.cpp
    test/test_regression.cpp
    test/test_clock.cpp
    test/test_engine.cpp
    test/test_input.cpp
    test/test_vendor_headers.cpp
    test/test_audio_system.cpp
    test/test_audio_decoder.cpp
    test/test_chart.cpp
    test/test_timing_data.cpp
    test/test_ksf_parser.cpp
    test/test_judge.cpp
    test/test_gameplay_state.cpp
    test/test_note_renderer.cpp
)

target_link_libraries(openitup_tests PRIVATE
    openitup_engine
    openitup_audio
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(openitup_tests)
