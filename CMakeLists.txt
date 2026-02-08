cmake_minimum_required(VERSION 3.16)
project(CaptainComic)

set(CMAKE_CXX_STANDARD 17)

find_package(SDL2 REQUIRED)

# Find SDL2_image using pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)

include_directories(${SDL2_INCLUDE_DIRS})
include_directories(${SDL2_IMAGE_INCLUDE_DIRS})
include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(captain_comic 
    src/main.cpp
    src/physics.cpp
    src/graphics.cpp
)

# Link libraries
target_link_libraries(captain_comic 
    ${SDL2_LIBRARIES}
    ${SDL2_IMAGE_LIBRARIES}
)

# Add include directories from pkg-config
target_include_directories(captain_comic PRIVATE ${SDL2_IMAGE_INCLUDE_DIRS})

