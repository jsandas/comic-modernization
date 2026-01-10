cmake_minimum_required(VERSION 3.10)
project(ComicC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find SDL2. Try cmake package first, fall back to pkg-config on macOS/Homebrew if needed
find_package(SDL2)
if(NOT SDL2_FOUND)
    find_package(PkgConfig)
    if(PkgConfig_FOUND)
        pkg_check_modules(SDL2_PKG sdl2)
        if(SDL2_PKG_FOUND)
            message(STATUS "Found SDL2 via pkg-config: ${SDL2_PKG_INCLUDE_DIRS}")
            set(SDL2_LIBRARIES ${SDL2_PKG_LIBRARIES})
            set(SDL2_INCLUDE_DIRS ${SDL2_PKG_INCLUDE_DIRS})
            # Provide an imported target so the rest of the CMakeLists can link against SDL2::SDL2
            if(NOT TARGET SDL2::SDL2)
                add_library(SDL2::SDL2 UNKNOWN IMPORTED)
                set_target_properties(SDL2::SDL2 PROPERTIES
                    INTERFACE_INCLUDE_DIRECTORIES "${SDL2_INCLUDE_DIRS}"
                    INTERFACE_LINK_LIBRARIES "${SDL2_LIBRARIES}")
            endif()
            set(SDL2_FOUND TRUE)
        endif()
    endif()
endif()

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
