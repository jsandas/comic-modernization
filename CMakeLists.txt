cmake_minimum_required(VERSION 3.16)
project(CaptainComic)

set(CMAKE_CXX_STANDARD 17)

find_package(SDL2 REQUIRED)

# Find SDL2_image using pkg-config with IMPORTED_TARGET
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2_IMAGE REQUIRED IMPORTED_TARGET SDL2_image)

include_directories(${SDL2_INCLUDE_DIRS})
include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(captain_comic 
    src/main.cpp
    src/physics.cpp
    src/graphics.cpp
    src/level_data.cpp
    src/file_loader.cpp
    src/level_loader.cpp
    src/level_tiles.cpp
)

# Link libraries
target_link_libraries(captain_comic 
    ${SDL2_LIBRARIES}
    PkgConfig::SDL2_IMAGE
)

enable_testing()

add_executable(comic_tests
    tests/test_main.cpp
    src/physics.cpp
    src/graphics.cpp
    src/level_data.cpp
    src/file_loader.cpp
    src/level_loader.cpp
    src/level_tiles.cpp
)

target_link_libraries(comic_tests
    ${SDL2_LIBRARIES}
    PkgConfig::SDL2_IMAGE
)

add_test(NAME comic_tests_all COMMAND comic_tests)
add_test(NAME comic_tests_physics_tiles COMMAND comic_tests --filter physics_tiles)
add_test(NAME comic_tests_animation_looping COMMAND comic_tests --filter animation_looping)
add_test(NAME comic_tests_animation_non_looping COMMAND comic_tests --filter animation_non_looping)
add_test(NAME comic_tests_animation_zero_duration COMMAND comic_tests --filter animation_zero_duration)
add_test(NAME comic_tests_jump_edge_trigger COMMAND comic_tests --filter jump_edge_trigger)
add_test(NAME comic_tests_jump_recharge COMMAND comic_tests --filter jump_recharge)
add_test(NAME comic_tests_jump_height COMMAND comic_tests --filter jump_height)

