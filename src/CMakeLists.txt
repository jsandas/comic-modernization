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

# Enemy AI unit tests
add_executable(enemy-ai-tests tests/enemy_ai_tests.cpp game/GameState.cpp assets/MapLoader.cpp input/Input.cpp)
set_target_properties(enemy-ai-tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
target_include_directories(enemy-ai-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
if(TARGET SDL2::SDL2)
    target_link_libraries(enemy-ai-tests SDL2::SDL2)
else()
    if(SDL2_INCLUDE_DIRS)
        target_include_directories(enemy-ai-tests PRIVATE ${SDL2_INCLUDE_DIRS})
    endif()
    if(SDL2_LIBRARIES)
        target_link_libraries(enemy-ai-tests ${SDL2_LIBRARIES})
    endif()
endif()

enable_testing()
add_test(NAME collision-tests COMMAND collision-tests)
add_test(NAME enemy-ai-tests COMMAND enemy-ai-tests)
# Enemy-player tests (guarded to avoid configure-time failures on checkouts missing the test source)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/enemy_player_tests.cpp")
    add_executable(enemy-player-tests tests/enemy_player_tests.cpp game/GameState.cpp assets/MapLoader.cpp input/Input.cpp)
    set_target_properties(enemy-player-tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    target_include_directories(enemy-player-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    if(TARGET SDL2::SDL2)
        target_link_libraries(enemy-player-tests SDL2::SDL2)
    else()
        if(SDL2_INCLUDE_DIRS)
            target_include_directories(enemy-player-tests PRIVATE ${SDL2_INCLUDE_DIRS})
        endif()
        if(SDL2_LIBRARIES)
            target_link_libraries(enemy-player-tests ${SDL2_LIBRARIES})
        endif()
    endif()

    add_test(NAME enemy-player-tests COMMAND enemy-player-tests)
else()
    message(STATUS "Skipping enemy-player-tests: ${CMAKE_CURRENT_SOURCE_DIR}/tests/enemy_player_tests.cpp not found in checkout")
endif()
add_executable(player-damage-knockback-tests tests/player_damage_knockback_tests.cpp game/GameState.cpp assets/MapLoader.cpp input/Input.cpp)
set_target_properties(player-damage-knockback-tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
target_include_directories(player-damage-knockback-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
if(TARGET SDL2::SDL2)
    target_link_libraries(player-damage-knockback-tests SDL2::SDL2)
else()
    if(SDL2_INCLUDE_DIRS)
        target_include_directories(player-damage-knockback-tests PRIVATE ${SDL2_INCLUDE_DIRS})
    endif()
    if(SDL2_LIBRARIES)
        target_link_libraries(player-damage-knockback-tests ${SDL2_LIBRARIES})
    endif()
endif()

add_test(NAME player-damage-knockback-tests COMMAND player-damage-knockback-tests)
add_executable(player-respawn-tests tests/player_respawn_tests.cpp game/GameState.cpp assets/MapLoader.cpp input/Input.cpp)
set_target_properties(player-respawn-tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
target_include_directories(player-respawn-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
if(TARGET SDL2::SDL2)
    target_link_libraries(player-respawn-tests SDL2::SDL2)
else()
    if(SDL2_INCLUDE_DIRS)
        target_include_directories(player-respawn-tests PRIVATE ${SDL2_INCLUDE_DIRS})
    endif()
    if(SDL2_LIBRARIES)
        target_link_libraries(player-respawn-tests ${SDL2_LIBRARIES})
    endif()
endif()

add_test(NAME player-respawn-tests COMMAND player-respawn-tests)
# Add a convenience target 'check' to build and run tests: `cmake --build build --target check`
set(TEST_TARGETS collision-tests enemy-ai-tests player-damage-knockback-tests player-respawn-tests)
if(TARGET enemy-player-tests)
    list(APPEND TEST_TARGETS enemy-player-tests)
endif()
add_custom_target(check
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS ${TEST_TARGETS}
)

# If pkg-config provided explicit SDL2 include dirs, ensure all relevant targets get them
if(SDL2_INCLUDE_DIRS)
    target_include_directories(comic-c PRIVATE ${SDL2_INCLUDE_DIRS})
    target_include_directories(collision-tests PRIVATE ${SDL2_INCLUDE_DIRS})
    target_include_directories(enemy-ai-tests PRIVATE ${SDL2_INCLUDE_DIRS})
    if(TARGET enemy-player-tests)
        target_include_directories(enemy-player-tests PRIVATE ${SDL2_INCLUDE_DIRS})
    endif()
    target_include_directories(player-damage-knockback-tests PRIVATE ${SDL2_INCLUDE_DIRS})
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
        target_include_directories(enemy-ai-tests PRIVATE ${HOMEBREW_INCLUDE} ${HOMEBREW_INCLUDE_SDL2})
    endif()
endif()



if(TARGET SDL2_image::SDL2_image)
    target_link_libraries(collision-tests SDL2_image::SDL2_image)
    target_link_libraries(enemy-ai-tests SDL2_image::SDL2_image)
    if(TARGET enemy-player-tests)
        target_link_libraries(enemy-player-tests SDL2_image::SDL2_image)
    endif()
    target_link_libraries(player-damage-knockback-tests SDL2_image::SDL2_image)
    target_link_libraries(player-respawn-tests SDL2_image::SDL2_image)
else()
    target_link_libraries(collision-tests ${SDL2_image_LIBRARIES})
    target_link_libraries(enemy-ai-tests ${SDL2_image_LIBRARIES})
    if(TARGET enemy-player-tests)
        target_link_libraries(enemy-player-tests ${SDL2_image_LIBRARIES})
    endif()
    target_link_libraries(player-damage-knockback-tests ${SDL2_image_LIBRARIES})
    target_link_libraries(player-respawn-tests ${SDL2_image_LIBRARIES})
    if(SDL2_image_INCLUDE_DIRS)
        target_include_directories(collision-tests PRIVATE ${SDL2_image_INCLUDE_DIRS})
        target_include_directories(enemy-ai-tests PRIVATE ${SDL2_image_INCLUDE_DIRS})
        if(TARGET enemy-player-tests)
            target_include_directories(enemy-player-tests PRIVATE ${SDL2_image_INCLUDE_DIRS})
        endif()
        target_include_directories(player-damage-knockback-tests PRIVATE ${SDL2_image_INCLUDE_DIRS})
        target_include_directories(player-respawn-tests PRIVATE ${SDL2_image_INCLUDE_DIRS})
    endif()
endif()

