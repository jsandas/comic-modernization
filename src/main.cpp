#include <SDL2/SDL.h>
#ifdef HAVE_SDL2_IMAGE
#include <SDL2/SDL_image.h>
#endif
#include <iostream>
#include <chrono>
#include "assets/AssetManager.h"
#include "graphics/Graphics.h"
#include "graphics/UI.h"
#include "audio/Audio.h"
#include "input/Input.h"
#include "game/GameState.h"
#include "game/Constants.h"

int main(int argc, char* argv[]) {
    (void)argc; (void)argv; // argc/argv are unused in this implementation; mark explicitly to avoid -Werror unused-parameter

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) < 0) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Initialize SDL_image
#ifdef HAVE_SDL2_IMAGE
    int img_flags = IMG_INIT_PNG;
    if (!(IMG_Init(img_flags) & img_flags)) {
        std::cerr << "Failed to initialize SDL_image: " << IMG_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }
#else
    std::cerr << "Warning: SDL2_image not available - PNG loading disabled" << std::endl;
#endif

    // Initialize graphics
    Graphics graphics;
    if (!graphics.initialize()) {
        std::cerr << "Failed to initialize graphics" << std::endl;
#ifdef HAVE_SDL2_IMAGE
        IMG_Quit();
#endif
        SDL_Quit();
        return 1;
    }

    // Initialize Audio
    Audio audio;
    if (!audio.initialize()) {
        std::cerr << "Failed to initialize audio (continuing without sound)" << std::endl;
    }

    // Initialize input
    Input input;
    input.initialize();

    // Initialize asset manager
    AssetManager assets;
    assets.setRenderer(graphics.getRenderer());
    if (!assets.initialize()) {
        std::cerr << "Failed to initialize asset manager" << std::endl;
        audio.shutdown();
        graphics.shutdown();
#ifdef HAVE_SDL2_IMAGE
        IMG_Quit();
#endif
        SDL_Quit();
        return 1;
    }

    // Set asset manager for graphics
    graphics.setAssetManager(&assets);

    // Initialize UI system
    UI ui;
    if (!ui.initialize(&graphics, &assets)) {
        std::cerr << "Failed to initialize UI system" << std::endl;
        audio.shutdown();
        graphics.shutdown();
#ifdef HAVE_SDL2_IMAGE
        IMG_Quit();
#endif
        SDL_Quit();
        return 1;
    }

    // Initialize game state
    GameState game_state;

    // Load initial level from data directory
    std::cerr << "Loading initial level " << game_state.current_level_number << " from: " << assets.getDataPath() << std::endl;
    if (!game_state.loadLevel(game_state.current_level_number, assets.getDataPath())) {
        std::cerr << "Warning: failed to load initial level; starting with empty map" << std::endl;
    } else {
        std::cerr << "Initial level loaded." << std::endl;
    }

    // Game loop
    bool running = true;
    bool paused = false;
    std::chrono::high_resolution_clock::time_point last_frame = std::chrono::high_resolution_clock::now();

    const float tick_interval = 1.0f / GameConstants::TICK_RATE;
    float time_accumulator = 0.0f;

    std::cout << "Starting game loop at " << GameConstants::TICK_RATE << " Hz" << std::endl;

    SDL_Event event;
    while (running) {
        // Handle events
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    running = false;
                    break;
                case SDL_KEYDOWN:
                    input.handleEvent(event);
                    if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                        running = false;
                    }
                    if (event.key.keysym.scancode == SDL_SCANCODE_P) {
                        paused = !paused;
                    }
                    break;
                case SDL_KEYUP:
                    input.handleEvent(event);
                    break;
                default:
                    break;
            }
        }

        // Calculate delta time
        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(now - last_frame).count();
        last_frame = now;

        // Accumulate time for fixed timestep
        time_accumulator += dt;

        // Process game ticks
        while (time_accumulator >= tick_interval && !paused) {
            // Update input state at the start of each tick
            input.update();

            // Game logic tick
            game_state.update(input);

            time_accumulator -= tick_interval;
        }

        graphics.clear();

        // Render the tilemap based on current level and stage
        if (game_state.current_map) {
            // Get the tileset name for the current level
            std::string tileset_name;
            switch (game_state.current_level_number) {
                case GameConstants::LEVEL_NUMBER_LAKE:    tileset_name = "lake.tt2"; break;
                case GameConstants::LEVEL_NUMBER_FOREST:  tileset_name = "forest.tt2"; break;
                case GameConstants::LEVEL_NUMBER_SPACE:   tileset_name = "space.tt2"; break;
                case GameConstants::LEVEL_NUMBER_COMP:    tileset_name = "comp.tt2"; break;
                case GameConstants::LEVEL_NUMBER_CAVE:    tileset_name = "cave.tt2"; break;
                case GameConstants::LEVEL_NUMBER_SHED:    tileset_name = "shed.tt2"; break;
                case GameConstants::LEVEL_NUMBER_CASTLE:  tileset_name = "castle.tt2"; break;
                case GameConstants::LEVEL_NUMBER_BASE:    tileset_name = "base.tt2"; break;
                default:                                  tileset_name = "forest.tt2"; break;
            }
            
            graphics.renderTileMap(*game_state.current_map, game_state.camera_x, tileset_name);
        }

        // Render player sprite
        // For now, just render a simple placeholder at the player position
        std::string player_sprite = "sprite-comic_standing_" + std::string(game_state.comic_facing == 1 ? "right" : "left");
        graphics.drawSpriteByName(game_state.comic_x - game_state.camera_x, game_state.comic_y, player_sprite);

        // Render enemies (placeholder - would iterate over active enemies)
        for (const auto& enemy : game_state.enemies) {
            if (enemy.spawn_timer > 0) {
                // Skip rendering if spawn timer is active (materializing effect)
                continue;
            }
            // Enemy rendering would go here
        }

        // Render fireballs
        for (const auto& fireball : game_state.fireballs) {
            graphics.drawSpriteByName(fireball.x - game_state.camera_x, fireball.y, "sprite-fireball_0");
        }

        // Render UI overlay (HP, fireball meter, lives, etc.)
        ui.render(game_state);

        graphics.present();

        // Cap frame rate if needed
        SDL_Delay(1);  // 1ms sleep to prevent CPU spinning
    }

    std::cout << "Shutting down..." << std::endl;

    // Cleanup
    audio.shutdown();
    graphics.shutdown();
#ifdef HAVE_SDL2_IMAGE
    IMG_Quit();
#endif
    SDL_Quit();

    std::cout << "Goodbye!" << std::endl;
    return 0;
}