set(SOURCES
    main.cpp
    assets/AssetManager.cpp
    graphics/Graphics.cpp
    audio/Audio.cpp
    input/Input.cpp
    game/GameState.cpp
)

set(HEADERS
    assets/AssetManager.h
    graphics/Graphics.h
    audio/Audio.h
    input/Input.h
    game/GameState.h
    game/Constants.h
)

add_executable(comic-c ${SOURCES} ${HEADERS})

# Link SDL2 and SDL2_image
target_link_libraries(comic-c SDL2::SDL2)

# Link SDL2_image if found
if(SDL2_image_FOUND)
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
