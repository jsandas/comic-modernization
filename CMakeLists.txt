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
)

target_link_libraries(comic_tests
    ${SDL2_LIBRARIES}
    PkgConfig::SDL2_IMAGE
)

add_test(NAME comic_tests COMMAND comic_tests)

