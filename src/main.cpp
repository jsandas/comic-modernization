#include <SDL2/SDL.h>
#include <iostream>
#include "../include/physics.h"

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

    bool quit = false;
    SDL_Event e;

    // Initialize test level
    init_test_level();

    while (!quit) {
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

        // Clear screen
        SDL_SetRenderDrawColor(renderer, 0, 64, 128, 255); // Blue background
        SDL_RenderClear(renderer);

        // Scale factor for visibility (original is 2 pixels per game unit)
        const int SCALE = 16;

        // Render tiles (simplified - just show solid tiles)
        SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255); // Gray tiles
        for (int ty = 0; ty < MAP_HEIGHT_TILES; ty++) {
            for (int tx = 0; tx < MAP_WIDTH_TILES; tx++) {
                // Only render tiles visible on camera
                int world_x = tx * 2; // Tile x in game units
                if (world_x >= camera_x && world_x < camera_x + PLAYFIELD_WIDTH) {
                    uint8_t tile = get_tile_at(tx * 2, ty * 2);
                    if (is_tile_solid(tile)) {
                        int screen_x = (world_x - camera_x) * SCALE;
                        int screen_y = ty * 2 * SCALE;
                        SDL_Rect tileRect = {screen_x, screen_y, SCALE * 2, SCALE * 2};
                        SDL_RenderFillRect(renderer, &tileRect);
                    }
                }
            }
        }

        // Render player as a rectangle (camera-relative)
        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); // Yellow player
        int screen_x = (comic_x - camera_x) * SCALE;
        int screen_y = comic_y * SCALE;
        SDL_Rect playerRect = {screen_x, screen_y, SCALE * 2, SCALE * 4}; // 2x4 game units
        SDL_RenderFillRect(renderer, &playerRect);

        // Present
        SDL_RenderPresent(renderer);

        // Cap to 60 FPS
        SDL_Delay(16);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}