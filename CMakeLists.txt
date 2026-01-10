cmake_minimum_required(VERSION 3.10)
project(ComicC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find SDL2. Try cmake package first, fall back to pkg-config on macOS/Homebrew if needed
find_package(SDL2)
if(NOT SDL2_FOUND)
    # Try pkg-config via CMake's PkgConfig module first
    find_package(PkgConfig)
    if(PkgConfig_FOUND)
        pkg_check_modules(SDL2_PKG sdl2)
        if(SDL2_PKG_FOUND)
            set(SDL2_LIBRARIES ${SDL2_PKG_LIBRARIES})
            set(SDL2_INCLUDE_DIRS ${SDL2_PKG_INCLUDE_DIRS})
            if(NOT TARGET SDL2::SDL2)
                add_library(SDL2::SDL2 UNKNOWN IMPORTED)
                set_target_properties(SDL2::SDL2 PROPERTIES
                    INTERFACE_INCLUDE_DIRECTORIES "${SDL2_INCLUDE_DIRS}"
                    INTERFACE_LINK_LIBRARIES "${SDL2_LIBRARIES}")
            endif()
            set(SDL2_FOUND TRUE)
        endif()
    endif()

    # As a more direct fallback, call pkg-config binary to gather flags and include dirs
    if(NOT SDL2_FOUND OR NOT SDL2_INCLUDE_DIRS)
        execute_process(COMMAND pkg-config --cflags sdl2
                        OUTPUT_VARIABLE PKG_SDL2_CFLAGS
                        RESULT_VARIABLE PKG_SDL2_C_RV
                        OUTPUT_STRIP_TRAILING_WHITESPACE)
        if(PKG_SDL2_C_RV EQUAL 0)
            # Extract -I flags
            string(REGEX MATCHALL "-I[^ ]+" PKG_SDL2_INCLUDES "${PKG_SDL2_CFLAGS}")
            if(PKG_SDL2_INCLUDES)
                foreach(_inc ${PKG_SDL2_INCLUDES})
                    string(SUBSTRING ${_inc} 2 -1 _incpath)
                    include_directories(${_incpath})
                    list(APPEND SDL2_INCLUDE_DIRS ${_incpath})
                endforeach()
            endif()
        endif()

        execute_process(COMMAND pkg-config --libs sdl2
                        OUTPUT_VARIABLE PKG_SDL2_LIBS
                        RESULT_VARIABLE PKG_SDL2_L_RV
                        OUTPUT_STRIP_TRAILING_WHITESPACE)
        if(PKG_SDL2_L_RV EQUAL 0)
            set(SDL2_LIBRARIES "${PKG_SDL2_LIBS}")
            set(SDL2_FOUND TRUE)
        endif()
    endif()
endif()

# Find SDL2_image (optional, try pkg-config if cmake find fails)
find_package(SDL2_image)
if(NOT SDL2_image_FOUND)
    find_package(PkgConfig)
    if(PkgConfig_FOUND)
        pkg_check_modules(SDL2_image_PKG SDL2_image)
        if(SDL2_image_PKG_FOUND)
            set(SDL2_image_LIBRARIES ${SDL2_image_PKG_LIBRARIES})
            set(SDL2_image_INCLUDE_DIRS ${SDL2_image_PKG_INCLUDE_DIRS})
            if(NOT TARGET SDL2_image::SDL2_image)
                add_library(SDL2_image::SDL2_image UNKNOWN IMPORTED)
                set_target_properties(SDL2_image::SDL2_image PROPERTIES
                    INTERFACE_INCLUDE_DIRECTORIES "${SDL2_image_INCLUDE_DIRS}"
                    INTERFACE_LINK_LIBRARIES "${SDL2_image_LIBRARIES}")
            endif()
            set(SDL2_image_FOUND TRUE)
        endif()
    endif()

    if(NOT SDL2_image_FOUND)
        execute_process(COMMAND pkg-config --cflags SDL2_image
                        OUTPUT_VARIABLE PKG_SDL2_IMAGE_CFLAGS
                        RESULT_VARIABLE PKG_SDL2_IMAGE_C_RV
                        OUTPUT_STRIP_TRAILING_WHITESPACE)
        if(PKG_SDL2_IMAGE_C_RV EQUAL 0)
            string(REGEX MATCHALL "-I[^ ]+" PKG_SDL2_IMAGE_INCLUDES "${PKG_SDL2_IMAGE_CFLAGS}")
            if(PKG_SDL2_IMAGE_INCLUDES)
                foreach(_inc ${PKG_SDL2_IMAGE_INCLUDES})
                    string(SUBSTRING ${_inc} 2 -1 _incpath)
                    list(APPEND SDL2_image_INCLUDE_DIRS ${_incpath})
                endforeach()
            endif()
        endif()

        execute_process(COMMAND pkg-config --libs SDL2_image
                        OUTPUT_VARIABLE PKG_SDL2_IMAGE_LIBS
                        RESULT_VARIABLE PKG_SDL2_IMAGE_L_RV
                        OUTPUT_STRIP_TRAILING_WHITESPACE)
        if(PKG_SDL2_IMAGE_L_RV EQUAL 0)
            set(SDL2_image_LIBRARIES "${PKG_SDL2_IMAGE_LIBS}")
            set(SDL2_image_FOUND TRUE)
        endif()
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
