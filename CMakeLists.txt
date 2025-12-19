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

# Add custom target to build assets
add_custom_target(build_assets
    COMMAND make -C ${CMAKE_SOURCE_DIR}/assets/extraction
    COMMENT "Building game assets..."
)

# Copy assets to build directory
add_custom_command(
    TARGET build_assets POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets/extracted
        ${CMAKE_BINARY_DIR}/data
    COMMENT "Copying assets to ${CMAKE_BINARY_DIR}/data"
)

# Enable testing and add source and test subdirectories
enable_testing()
add_subdirectory(src)
add_subdirectory(src/tests)
