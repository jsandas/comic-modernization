#include <SDL2/SDL.h>
#include <iostream>
#include "../include/physics.h"
#include "../include/graphics.h"
#include "../include/level_loader.h"
#include "../include/doors.h"

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
uint8_t previous_key_state_jump = 0;
uint8_t key_state_left = 0;
uint8_t key_state_right = 0;
uint8_t key_state_open = 0;  // Open key for doors
uint8_t previous_key_state_open = 0;  // Track previous state for edge-triggered activation
int camera_x = 0;

// Item collection state
uint8_t comic_has_door_key = 0;  // 1 if player has door key, 0 otherwise

// Level/stage transition tracking
uint8_t current_level_number = 1;  // Current level (0=LAKE, 1=FOREST, etc.)
uint8_t current_stage_number = 0;  // Current stage (0-2 per level)
const level_t* current_level_ptr = nullptr;  // Pointer to current level data
int8_t source_door_level_number = -1;  // Set when entering via door for reciprocal positioning
int8_t source_door_stage_number = -1;

// Checkpoint position (for respawn and boundary crossing)
uint8_t comic_y_checkpoint = 12;  // Y position to respawn at
uint8_t comic_x_checkpoint = 14;  // X position to respawn at

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

void process_door_input() {
    // Edge-triggered door activation: only trigger on rising edge of open key
    // This prevents the door from immediately re-triggering when entering a new stage
    // (since load_new_stage positions Comic at the reciprocal door location)
    if (comic_is_falling_or_jumping == 0 &&
        key_state_open && !previous_key_state_open) {
        check_door_activation();
    }
    
    previous_key_state_open = key_state_open;
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

    // Initialize all level data (tile data is compiled-in as hex arrays)
    if (!initialize_level_data()) {
        std::cerr << "Warning: Level data initialization failed." << std::endl;
    }

    // Load the first playable level (FOREST = level 1, stage 0)
    // Level numbers: 0=LAKE, 1=FOREST, 2=SPACE, 3=BASE, 4=CAVE, 5=SHED, 6=CASTLE, 7=COMP
    current_level_number = LEVEL_NUMBER_FOREST;  // Forest is the first playable level
    current_stage_number = 0;
    source_door_level_number = -1;  // Not entering via door
    
    // Set initial spawn position
    comic_x = 14;
    comic_y = 12;
    comic_y_vel = 0;
    
    // Load the level and stage
    load_new_level();
    
    if (!current_level_ptr) {
        std::cerr << "Failed to load game level. Falling back to test level." << std::endl;
        init_test_level();  // Fall back to test level if loading fails
    }

    // Tick timing - match original game's ~18.2 Hz tick rate
    constexpr double TICK_RATE = 18.2065; // PC timer interrupt rate (1193182/65536 Hz)
    constexpr double MS_PER_TICK = 1000.0 / TICK_RATE; // ~54.93 ms per tick
    constexpr int MAX_TICKS_PER_FRAME = 5;
    constexpr double MAX_ACCUMULATED_MS = MS_PER_TICK * MAX_TICKS_PER_FRAME;
    uint32_t last_tick_time = SDL_GetTicks();
    double tick_accumulator = 0.0;

    while (!quit) {
        uint32_t current_time = SDL_GetTicks();
        uint32_t delta_time = current_time - last_tick_time;
        last_tick_time = current_time;
        tick_accumulator += delta_time;
        if (tick_accumulator > MAX_ACCUMULATED_MS) {
            tick_accumulator = MAX_ACCUMULATED_MS;
        }

        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            } else if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_LEFT: key_state_left = 1; break;
                    case SDLK_RIGHT: key_state_right = 1; break;
                    case SDLK_SPACE: key_state_jump = 1; break;
                    case SDLK_o: key_state_open = 1; break;  // 'O' key to open doors
                    case SDLK_k: comic_has_door_key = 1; break;  // 'K' key for debugging (grant door key)
                }
            } else if (e.type == SDL_KEYUP) {
                switch (e.key.keysym.sym) {
                    case SDLK_LEFT: key_state_left = 0; break;
                    case SDLK_RIGHT: key_state_right = 0; break;
                    case SDLK_SPACE: key_state_jump = 0; break;
                    case SDLK_o: key_state_open = 0; break;  // 'O' key to open doors
                }
            }
        }

        // Process physics ticks at ~18.2 Hz (original game speed)
        // This decouples physics from rendering rate
        int ticks_processed = 0;
        while (tick_accumulator >= MS_PER_TICK && ticks_processed < MAX_TICKS_PER_FRAME) {
            tick_accumulator -= MS_PER_TICK;
            ticks_processed++;

            // Process jump input once per tick (edge-triggered)
            process_jump_input();

            // Process door input once per tick (edge-triggered)
            process_door_input();

            // Update physics (once per tick)
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
        }

        // Update animation based on state (updates every frame for smooth animation)
        current_time = SDL_GetTicks();
        Animation* previous_animation = current_animation;
        if (comic_is_falling_or_jumping) {
            current_animation = comic_facing ? &comic_jump_right : &comic_jump_left;
        } else {
            if (key_state_left || key_state_right) {
                current_animation = comic_facing ? &comic_run_right : &comic_run_left;
            } else {
                current_animation = comic_facing ? &comic_idle_right : &comic_idle_left;
            }
        }

        // Reset animation state when switching to a new animation
        if (current_animation && current_animation != previous_animation) {
            current_animation->current_frame = 0;
            current_animation->frame_start_time = current_time;
        }

        if (current_animation) {
            g_graphics->update_animation(*current_animation, current_time);
        }

        // Clear screen with black background
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Get current tileset from loaded level
        const char* level_names[] = {"lake", "forest", "space", "base", "cave", "shed", "castle", "comp"};
        std::string current_level_name = (current_level_number < 8) ? level_names[current_level_number] : "forest";
        Tileset* tileset = g_graphics->get_tileset(current_level_name);

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

        // Cap rendering to ~60 FPS while physics runs at 18.2 Hz
        if (tick_accumulator >= MS_PER_TICK) {
            SDL_Delay(0);
        } else {
            SDL_Delay(16);
        }
    }

    // Cleanup
    delete g_graphics;
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}