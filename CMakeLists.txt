cmake_minimum_required(VERSION 3.10)
project(ComicC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find SDL2
find_package(SDL2 REQUIRED)

# Find SDL2_image (optional, try pkg-config if cmake find fails)
find_package(SDL2_image)
if(NOT SDL2_image_FOUND)
    # Try using pkg-config
    find_package(PkgConfig)
    if(PkgConfig_FOUND)
        pkg_check_modules(SDL2_image SDL2_image)
    endif()
endif()

# Add custom target to copy reference assets
add_custom_target(build_assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/reference/assets
        ${CMAKE_BINARY_DIR}/data
    COMMENT "Copying reference assets to ${CMAKE_BINARY_DIR}/data"
)

# Add source subdirectory
add_subdirectory(src)
