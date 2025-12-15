#include <SDL2/SDL.h>
#ifdef HAVE_SDL2_IMAGE
#include <SDL2/SDL_image.h>
#endif
#include <iostream>
#include <chrono>
#include "assets/AssetManager.h"
#include "graphics/Graphics.h"
#include "audio/Audio.h"
#include "input/Input.h"
#include "game/GameState.h"
#include "game/Constants.h"

int main(int argc, char* argv[]) {
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

    // Initialize game state
    GameState game_state;

    // Game loop
    bool running = true;
    bool paused = false;
    std::chrono::high_resolution_clock::time_point last_tick = std::chrono::high_resolution_clock::now();
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

            // Game logic tick would go here
            // For now, just a placeholder

            time_accumulator -= tick_interval;
        }

        // Rendering
        graphics.clear();

        // Placeholder: render game content
        // This would render the current map, player, enemies, etc.

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
