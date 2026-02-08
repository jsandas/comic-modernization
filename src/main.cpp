#include <SDL2/SDL.h>
#include <iostream>

// Physics constants
#define COMIC_GRAVITY 5
#define TERMINAL_VELOCITY 23
#define JUMP_ACCELERATION 7

// Game state
int comic_x = 100;
int comic_y = 100;
int8_t comic_y_vel = 0;
int8_t comic_x_momentum = 0;
uint8_t comic_facing = 1; // 1 right, 0 left
uint8_t comic_animation = 0;
uint8_t comic_is_falling_or_jumping = 0;
uint8_t comic_jump_counter = 0;
uint8_t comic_jump_power = 4;
uint8_t key_state_jump = 0;
uint8_t key_state_left = 0;
uint8_t key_state_right = 0;

void handle_fall_or_jump() {
    if (comic_is_falling_or_jumping) {
        // Decrement jump counter
        if (comic_jump_counter > 0) {
            comic_jump_counter--;
            if (key_state_jump) {
                comic_y_vel -= JUMP_ACCELERATION;
            }
        }

        // Integrate velocity
        int delta_y = comic_y_vel >> 3;
        comic_y += delta_y;

        // Apply gravity
        comic_y_vel += COMIC_GRAVITY;
        if (comic_y_vel > TERMINAL_VELOCITY) {
            comic_y_vel = TERMINAL_VELOCITY;
        }

        // Ground collision (simple)
        if (comic_y >= 400) {
            comic_y = 400;
            comic_y_vel = 0;
            comic_is_falling_or_jumping = 0;
            comic_jump_counter = comic_jump_power;
        }
    } else {
        // Recharge jump counter
        comic_jump_counter = comic_jump_power;

        // Start jump if pressed
        if (key_state_jump) {
            comic_is_falling_or_jumping = 1;
            comic_jump_counter--;
            comic_y_vel -= JUMP_ACCELERATION;
        }
    }
}

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

        // Simple horizontal movement
        if (key_state_left) comic_x -= 1;
        if (key_state_right) comic_x += 1;

        // Clear screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Render player as a rectangle
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_Rect playerRect = {comic_x, comic_y, 16, 16};
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