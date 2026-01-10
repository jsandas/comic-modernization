set(SOURCES
    main.cpp
    assets/AssetManager.cpp
    assets/MapLoader.cpp
    graphics/Graphics.cpp
    graphics/UI.cpp
    audio/Audio.cpp
    input/Input.cpp
    game/GameState.cpp
)

set(HEADERS
    assets/AssetManager.h
    graphics/Graphics.h
    graphics/UI.h
    audio/Audio.h
    input/Input.h
    game/GameState.h
    game/Constants.h
)

add_executable(comic-c ${SOURCES} ${HEADERS})

# Link SDL2 and SDL2_image
if(TARGET SDL2::SDL2)
    target_link_libraries(comic-c SDL2::SDL2)
else()
    # Fallback: link and include via SDL2 variables from find_package or pkg-config
    if(SDL2_INCLUDE_DIRS)
        # Normalize list
        if(NOT SDL2_INCLUDE_DIRS STREQUAL "${SDL2_INCLUDE_DIRS}")
        endif()
        target_include_directories(comic-c PRIVATE ${SDL2_INCLUDE_DIRS})
    endif()
    if(SDL2_LIBRARIES)
        target_link_libraries(comic-c ${SDL2_LIBRARIES})
    endif()
endif()


# Link SDL2_image if found
if(TARGET SDL2_image::SDL2_image)
    target_link_libraries(comic-c SDL2_image::SDL2_image)
    target_compile_definitions(comic-c PRIVATE HAVE_SDL2_IMAGE=1)
else()
    target_link_libraries(comic-c ${SDL2_image_LIBRARIES})
    if(SDL2_image_INCLUDE_DIRS)
        target_include_directories(comic-c PRIVATE ${SDL2_image_INCLUDE_DIRS})
    endif()
endif()

# Add dependency on asset building
add_dependencies(comic-c build_assets)

# Set runtime output directory to build root
set_target_properties(comic-c PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Include directories
target_include_directories(comic-c PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Collision unit tests
add_executable(collision-tests tests/collision_tests.cpp game/GameState.cpp assets/MapLoader.cpp input/Input.cpp)
# Put test in build root so it can find data files if needed
set_target_properties(collision-tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
target_include_directories(collision-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
# Link libraries similar to the main executable (SDL2) so MapLoader and Input link
if(TARGET SDL2::SDL2)
    target_link_libraries(collision-tests SDL2::SDL2)
else()
    if(SDL2_INCLUDE_DIRS)
        target_include_directories(collision-tests PRIVATE ${SDL2_INCLUDE_DIRS})
    endif()
    if(SDL2_LIBRARIES)
        target_link_libraries(collision-tests ${SDL2_LIBRARIES})
    endif()
endif()

# If pkg-config provided explicit SDL2 include dirs, ensure both targets get them
if(SDL2_INCLUDE_DIRS)
    target_include_directories(comic-c PRIVATE ${SDL2_INCLUDE_DIRS})
    target_include_directories(collision-tests PRIVATE ${SDL2_INCLUDE_DIRS})
endif()

# If pkg-config provided explicit SDL2_image include dirs, ensure both targets get them
if(SDL2_image_INCLUDE_DIRS)
    target_include_directories(comic-c PRIVATE ${SDL2_image_INCLUDE_DIRS})
    target_include_directories(collision-tests PRIVATE ${SDL2_image_INCLUDE_DIRS})
endif()

# On macOS with Homebrew, ensure the common brew include paths are present if they exist
if(APPLE)
    execute_process(COMMAND brew --prefix OUTPUT_VARIABLE BREW_PREFIX OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(HOMEBREW_INCLUDE ${BREW_PREFIX}/include)
    set(HOMEBREW_INCLUDE_SDL2 ${BREW_PREFIX}/include/SDL2)
    if(EXISTS ${HOMEBREW_INCLUDE} AND NOT "${HOMEBREW_INCLUDE}" IN_LIST CMAKE_SYSTEM_INCLUDEDIR)
        target_include_directories(comic-c PRIVATE ${HOMEBREW_INCLUDE} ${HOMEBREW_INCLUDE_SDL2})
        target_include_directories(collision-tests PRIVATE ${HOMEBREW_INCLUDE} ${HOMEBREW_INCLUDE_SDL2})
    endif()
endif()



if(TARGET SDL2_image::SDL2_image)
    target_link_libraries(collision-tests SDL2_image::SDL2_image)
else()
    target_link_libraries(collision-tests ${SDL2_image_LIBRARIES})
    if(SDL2_image_INCLUDE_DIRS)
        target_include_directories(collision-tests PRIVATE ${SDL2_image_INCLUDE_DIRS})
    endif()
endif()

# Enable CTest and register the collision-tests as a test so it can be run via `ctest` or a CI runner
enable_testing()
add_test(NAME collision-tests COMMAND collision-tests)
# Add a convenience target 'check' to build and run tests: `cmake --build build --target check`
add_custom_target(check
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS collision-tests
)
