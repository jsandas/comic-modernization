cmake_minimum_required(VERSION 3.16)
project(CaptainComic)

set(CMAKE_CXX_STANDARD 17)

find_package(SDL2 REQUIRED)

# Find SDL2 extension libraries using pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2_IMAGE REQUIRED IMPORTED_TARGET SDL2_image)
pkg_check_modules(SDL2_TTF REQUIRED IMPORTED_TARGET SDL2_ttf)
pkg_check_modules(SDL2_MIXER SDL2_mixer)

# --- Core Game Logic Library ---
# Shared logic used by both the main game and the test suite
add_library(comic_core STATIC
    src/actors.cpp
    src/audio.cpp
    src/cheats.cpp
    src/doors.cpp
    src/graphics.cpp
    src/level_data.cpp
    src/level_loader.cpp
    src/level_tiles.cpp
    src/physics.cpp
    src/player_teleport.cpp
    src/title_sequence.cpp
    src/ui_system.cpp
)

target_include_directories(comic_core PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
)

target_include_directories(comic_core PRIVATE 
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(comic_core PUBLIC
    ${SDL2_LIBRARIES}
    PkgConfig::SDL2_IMAGE
    PkgConfig::SDL2_TTF
)

if (SDL2_MIXER_FOUND)
    target_link_libraries(comic_core PUBLIC ${SDL2_MIXER_LIBRARIES})
    target_include_directories(comic_core PUBLIC ${SDL2_MIXER_INCLUDE_DIRS})
    target_compile_definitions(comic_core PUBLIC HAVE_SDL2_MIXER=1)
else()
    message(WARNING "SDL2_mixer not found. Building without audio output support.")
endif()

# --- Main Executable ---
add_executable(captain_comic src/main.cpp)
target_link_libraries(captain_comic PRIVATE comic_core)

# --- Test Suite ---
enable_testing()

add_executable(comic_tests 
    tests/test_main.cpp
    tests/test_helpers.cpp
    tests/test_physics.cpp
    tests/test_graphics.cpp
    tests/test_actors.cpp
    tests/test_audio.cpp
    tests/test_ui.cpp
)
target_link_libraries(comic_tests PRIVATE comic_core)

# Test Registrations
add_test(NAME comic_tests_all COMMAND comic_tests)
add_test(NAME comic_tests_physics_tiles COMMAND comic_tests --filter physics_tiles)
add_test(NAME comic_tests_animation_looping COMMAND comic_tests --filter animation_looping)
add_test(NAME comic_tests_animation_non_looping COMMAND comic_tests --filter animation_non_looping)
add_test(NAME comic_tests_animation_zero_duration COMMAND comic_tests --filter animation_zero_duration)
add_test(NAME comic_tests_jump_edge_trigger COMMAND comic_tests --filter jump_edge_trigger)
add_test(NAME comic_tests_jump_recharge COMMAND comic_tests --filter jump_recharge)
add_test(NAME comic_tests_jump_height COMMAND comic_tests --filter jump_height)
add_test(NAME comic_tests_space_level_uses_lower_gravity COMMAND comic_tests --filter space_level_uses_lower_gravity)
add_test(NAME comic_tests_jump_top_clamped_to_playfield COMMAND comic_tests --filter jump_top_clamped_to_playfield)
add_test(NAME comic_tests_high_score_bytes_conversion COMMAND comic_tests --filter high_score_bytes_conversion)
add_test(NAME comic_tests_door_animation_phase_progression_and_render_state COMMAND comic_tests --filter door_animation_phase_progression_and_render_state)
add_test(NAME comic_tests_door_destination_load_deferred_until_entering_complete COMMAND comic_tests --filter door_destination_load_deferred_until_entering_complete)
