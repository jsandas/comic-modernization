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
set(ASSET_EXTRACTION_DIR ${CMAKE_SOURCE_DIR}/assets/extraction)

if(EXISTS ${CMAKE_SOURCE_DIR}/original/COMIC.EXE)
    add_custom_target(build_assets
        COMMAND make -C ${ASSET_EXTRACTION_DIR}
        COMMENT "Building game assets..."
    )
else()
    add_custom_target(build_assets
        COMMAND ${CMAKE_COMMAND} -E echo "Skipping build_assets: original game files not present in repo"
        COMMENT "Skipping game assets build (original files missing)"
    )
endif()

# Copy assets to build directory (only if extracted assets exist)
if(EXISTS ${CMAKE_SOURCE_DIR}/assets/extracted)
    add_custom_command(
        TARGET build_assets POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets/extracted
            ${CMAKE_BINARY_DIR}/data
        COMMENT "Copying assets to ${CMAKE_BINARY_DIR}/data"
    )
else()
    add_custom_command(
        TARGET build_assets POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "No extracted assets found; skipping copy to ${CMAKE_BINARY_DIR}/data"
        COMMENT "Skipping asset copy (no extracted assets present)"
    )
endif()

# Enable testing and add source and test subdirectories
enable_testing()
add_subdirectory(src)
add_subdirectory(src/tests)
