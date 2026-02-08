#include <SDL2/SDL.h>
#include <iostream>
#include "../include/physics.h"
#include "../include/graphics.h"

// Game state
int comic_x = 20;
int comic_y = 2;
int8_t comic_y_vel = 0;
int8_t comic_x_momentum = 0;
uint8_t comic_facing = 1; // 1 right, 0 left
uint8_t comic_animation = 0;
uint8_t comic_is_falling_or_jumping = 1;
uint8_t comic_jump_counter = 0;
uint8_t comic_jump_power = JUMP_POWER_DEFAULT;
uint8_t key_state_jump = 0;
uint8_t key_state_left = 0;
uint8_t key_state_right = 0;
int camera_x = 0;

// Rendering scale: 16 pixels per game unit
const int RENDER_SCALE = 16;

// Player animation state
Animation comic_idle_right;
Animation comic_idle_left;
Animation comic_run_right;
Animation comic_run_left;
Animation comic_jump_right;
Animation comic_jump_left;
Animation* current_animation = nullptr;

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Captain Comic", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_SHOWN);
    if (window == nullptr) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == nullptr) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Initialize graphics system
    g_graphics = new GraphicsSystem(renderer);
    if (!g_graphics->initialize()) {
        std::cerr << "Graphics system initialization failed!" << std::endl;
        delete g_graphics;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Load tileset for the current level (Forest is the first playable level)
    // Note: LEVEL_NUMBER_LAKE = 0, but LEVEL_NUMBER_FOREST = 1 is the actual starting level
    std::string current_level = "forest";
    if (!g_graphics->load_tileset(current_level)) {
        std::cerr << "Failed to load tileset for level: " << current_level << std::endl;
        delete g_graphics;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Pre-load player sprites and create animations
    const char* sprite_names[] = {
        "comic_standing", "comic_running_1", "comic_running_2", "comic_running_3", "comic_jumping"
    };
    const char* directions[] = {"right", "left"};
    
    for (const char* sprite : sprite_names) {
        for (const char* dir : directions) {
            if (!g_graphics->load_sprite(sprite, dir)) {
                std::cerr << "Failed to load sprite: " << sprite << " (" << dir << ")" << std::endl;
                delete g_graphics;
                SDL_DestroyRenderer(renderer);
                SDL_DestroyWindow(window);
                SDL_Quit();
                return 1;
            }
        }
    }

    // Create animations
    comic_idle_right = g_graphics->create_animation({"comic_standing"}, "right", 100, true);
    comic_idle_left = g_graphics->create_animation({"comic_standing"}, "left", 100, true);
    comic_run_right = g_graphics->create_animation(
        {"comic_running_1", "comic_running_2", "comic_running_3"}, "right", 100, true);
    comic_run_left = g_graphics->create_animation(
        {"comic_running_1", "comic_running_2", "comic_running_3"}, "left", 100, true);
    comic_jump_right = g_graphics->create_animation({"comic_jumping"}, "right", 100, true);
    comic_jump_left = g_graphics->create_animation({"comic_jumping"}, "left", 100, true);

    current_animation = &comic_idle_right;

    bool quit = false;
    SDL_Event e;

    // Initialize test level
    init_test_level();

    uint32_t last_time = SDL_GetTicks();

    while (!quit) {
        uint32_t current_time = SDL_GetTicks();
        last_time = current_time;

        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            } else if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_LEFT: key_state_left = 1; break;
                    case SDLK_RIGHT: key_state_right = 1; break;
                    case SDLK_SPACE: key_state_jump = 1; break;
                }
            } else if (e.type == SDL_KEYUP) {
                switch (e.key.keysym.sym) {
                    case SDLK_LEFT: key_state_left = 0; break;
                    case SDLK_RIGHT: key_state_right = 0; break;
                    case SDLK_SPACE: key_state_jump = 0; break;
                }
            }
        }

        // Update physics
        handle_fall_or_jump();

        // Ground movement (only when not in air)
        if (!comic_is_falling_or_jumping) {
            if (key_state_left) {
                move_left();
            }
            if (key_state_right) {
                move_right();
            }
        }

        // Update animation based on state
        if (comic_is_falling_or_jumping) {
            current_animation = comic_facing ? &comic_jump_right : &comic_jump_left;
        } else {
            if (key_state_left || key_state_right) {
                current_animation = comic_facing ? &comic_run_right : &comic_run_left;
            } else {
                current_animation = comic_facing ? &comic_idle_right : &comic_idle_left;
            }
        }

        if (current_animation) {
            g_graphics->update_animation(*current_animation, current_time);
        }

        // Clear screen with black background
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Get current tileset
        Tileset* tileset = g_graphics->get_tileset(current_level);

        // Render tiles
        for (int ty = 0; ty < MAP_HEIGHT_TILES; ty++) {
            for (int tx = 0; tx < MAP_WIDTH_TILES; tx++) {
                int world_x = tx * 2; // Tile x in game units
                
                // Only render tiles visible on camera
                if (world_x >= camera_x && world_x < camera_x + PLAYFIELD_WIDTH) {
                    uint8_t tile = get_tile_at(tx * 2, ty * 2);
                    
                    int screen_x = (world_x - camera_x) * RENDER_SCALE;
                    int screen_y = ty * 2 * RENDER_SCALE;
                    
                    g_graphics->render_tile(screen_x, screen_y, tileset, tile, RENDER_SCALE);
                }
            }
        }

        // Render player sprite
        if (current_animation) {
            AnimationFrame* frame = g_graphics->get_current_frame(*current_animation);
            if (frame) {
                // Center player on screen relative to camera
                // Player is 2 units wide, 4 units tall in game coords
                int screen_x = (comic_x - camera_x) * RENDER_SCALE + RENDER_SCALE;  // Center X
                int screen_y = comic_y * RENDER_SCALE + RENDER_SCALE * 2; // Center Y
                int player_width = RENDER_SCALE * 2;
                int player_height = RENDER_SCALE * 4;
                g_graphics->render_sprite_centered_scaled(screen_x, screen_y, frame->sprite, player_width, player_height);
            }
        }

        // Present
        SDL_RenderPresent(renderer);

        // Cap to 60 FPS
        SDL_Delay(16);
    }

    // Cleanup
    delete g_graphics;
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}