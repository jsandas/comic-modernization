#include <SDL2/SDL.h>
#include <iostream>
#include <cstring>
#include "../include/physics.h"
#include "../include/graphics.h"
#include "../include/level_loader.h"
#include "../include/doors.h"
#include "../include/cheats.h"
#include "../include/actors.h"
#include "../include/audio.h"
#include "../include/title_sequence.h"
#include "../include/ui_system.h"

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
uint8_t key_state_fire = 0;  // Fire key (Left Ctrl)
int camera_x = 0;

// Item collection state
uint8_t comic_has_door_key = 0;  // 1 if player has door key, 0 otherwise
uint8_t comic_num_lives = 0;  // Number of lives remaining (counts up from 0 to 5, then subtracts 1)
uint8_t lives_sequence_counter = 5;  // Lives count-up sequence: award 5 lives, then subtract 1
uint8_t lives_sequence_delay = 1;  // Delay counter for lives animation (1 tick between each award)
bool lives_sequence_complete = false;  // Flag to stop lives sequence after completion
uint8_t comic_hp = 0;  // Current health points (0-6)
uint8_t comic_hp_pending_increase = MAX_HP;  // HP fills gradually from 0 to MAX_HP at game start
uint8_t score_bytes[3] = {0, 0, 0};  // Score in base-100 encoding
// Note: Firepower, items, and treasures are managed by ActorSystem (authoritative state)

// Level/stage transition tracking
uint8_t current_level_number = 1;  // Current level (0=LAKE, 1=FOREST, etc.)
uint8_t current_stage_number = 0;  // Current stage (0-2 per level)
const level_t* current_level_ptr = nullptr;  // Pointer to current level data
int8_t source_door_level_number = -1;  // Set when entering via door for reciprocal positioning
int8_t source_door_stage_number = -1;

// Checkpoint position (for respawn and boundary crossing)
uint8_t comic_y_checkpoint = 12;  // Y position to respawn at
uint8_t comic_x_checkpoint = 14;  // X position to respawn at

// Game state
bool game_over_triggered = false;  // Flag to track if game-over sound has been played

// Global system pointers (for access from other modules)
ActorSystem* g_actor_system = nullptr;  // Actor system pointer (for cheat access)

// Level names (indexed by level number)
static constexpr const char* level_names[] = {
    "lake", "forest", "space", "base", "cave", "shed", "castle", "comp"
};

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
    // Parse command-line arguments
    bool debug_mode = false;
    bool skip_title = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--debug") == 0) {
            debug_mode = true;
            std::cout << "Debug mode enabled" << std::endl;
        } else if (std::strcmp(argv[i], "--skip-title") == 0) {
            skip_title = true;
        } else if (std::strcmp(argv[i], "--help") == 0) {
            std::cout << "Captain Comic - Usage:" << std::endl;
            std::cout << "  --debug       Enable debug mode and cheat keys" << std::endl;
            std::cout << "  --skip-title  Skip the title sequence" << std::endl;
            std::cout << "  --help        Show this help message" << std::endl;
            return 0;
        }
    }
    
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

    if (!initialize_audio_system()) {
        std::cerr << "Warning: Audio system initialization failed. Continuing without sound." << std::endl;
    }

    // Load persisted key bindings if KEYS.DEF is present.
    load_input_bindings_from_file();

    // Run startup notice and title sequence.
    // Both are skipped with --skip-title for faster iteration during development.
    if (!skip_title) {
        if (!run_startup_notice(renderer, g_graphics)) {
            cleanup_title_sequence();
            delete g_graphics;
            shutdown_audio_system();
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            SDL_Quit();
            return 0;
        }

        if (!run_title_sequence(renderer, g_graphics)) {
            // User quit during title sequence
            cleanup_title_sequence();
            delete g_graphics;
            shutdown_audio_system();
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            SDL_Quit();
            return 0;
        }
    }

    // Initialize cheat system
    g_cheats = new CheatSystem();
    g_cheats->initialize(debug_mode);

    ActorSystem actor_system;
    actor_system.initialize();
    if (debug_mode) {
        actor_system.comic_firepower = 3;  // Start with 3 fireball slots in debug mode for testing
    }
    
    // Make actor system accessible to cheat system
    g_actor_system = &actor_system;

    // Initialize UI system
    UISystem ui_system;
    if (!ui_system.initialize()) {
        std::cerr << "Warning: UI system initialization failed. HUD will not display properly."
                  << std::endl;
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

    // Load fireball sprites
    if (!actor_system.load_fireball_sprites(g_graphics)) {
        std::cerr << "Warning: Could not load fireball sprites. Fireballs will not render." << std::endl;
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
    initialize_level_data();

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

    // Cache for tileset to avoid per-frame lookups
    uint8_t cached_level_number = current_level_number;
    uint8_t cached_stage_number = current_stage_number;
    Tileset* cached_tileset = nullptr;
    if (current_level_number < 8) {
        cached_tileset = g_graphics->get_tileset(level_names[current_level_number]);
    }

    if (current_level_ptr) {
        actor_system.setup_enemies_for_stage(current_level_ptr, current_level_number, current_stage_number, g_graphics);
    }
    
    // Load item sprites
    if (!actor_system.load_item_sprites(g_graphics)) {
        std::cerr << "Warning: Some item sprites failed to load" << std::endl;
    }

    // Tick timing - match original game's ~9.1 Hz tick rate.
    // The PC timer interrupt (IRQ 0) fires at 18.2065 Hz but game_tick_flag is
    // only set on *odd* interrupt cycles (irq0_parity toggles 0,1,0,1,...),
    // so the actual game logic runs at half the interrupt rate: ~9.1032 Hz.
    // Based on analysis of the original DOS timer interrupt handler
    // (original disassembly is not vendored in this repository).
    constexpr double TICK_RATE = 18.2065 / 2.0; // ~9.1032 Hz - actual game tick rate
    constexpr double MS_PER_TICK = 1000.0 / TICK_RATE; // ~109.86 ms per tick
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
                // Process regular gameplay keys
                const InputBindings& bindings = get_input_bindings();
                const SDL_Keycode key = e.key.keysym.sym;

                if (key == bindings.move_left) {
                    key_state_left = 1;
                }
                if (key == bindings.move_right) {
                    key_state_right = 1;
                }
                if (key == bindings.jump) {
                    key_state_jump = 1;
                }
                if (key == bindings.fire) {
                    key_state_fire = 1;
                }
                if (key == bindings.open_door) {
                    key_state_open = 1;
                }
                
                // Process cheat keys (only active if --debug flag set)
                g_cheats->process_input(e.key.keysym.sym);
            } else if (e.type == SDL_KEYUP) {
                const InputBindings& bindings = get_input_bindings();
                const SDL_Keycode key = e.key.keysym.sym;

                if (key == bindings.move_left) {
                    key_state_left = 0;
                }
                if (key == bindings.move_right) {
                    key_state_right = 0;
                }
                if (key == bindings.jump) {
                    key_state_jump = 0;
                }
                if (key == bindings.fire) {
                    key_state_fire = 0;
                }
                if (key == bindings.open_door) {
                    key_state_open = 0;
                }
            }
        }

        // Process physics ticks at ~9.1 Hz (original game speed)
        // This decouples physics from rendering rate
        int ticks_processed = 0;
        while (tick_accumulator >= MS_PER_TICK && ticks_processed < MAX_TICKS_PER_FRAME) {
            tick_accumulator -= MS_PER_TICK;
            ticks_processed++;

            // Process jump input once per tick (edge-triggered)
            process_jump_input();

            // Process door input once per tick (edge-triggered)
            process_door_input();

            // Update jump power from item system (boots affect jump height)
            comic_jump_power = static_cast<uint8_t>(actor_system.get_jump_power());

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

            const uint8_t* tiles = current_level_ptr
                ? current_level_ptr->stages[current_stage_number].tiles
                : nullptr;
            actor_system.update(comic_x, comic_y, comic_facing, tiles, camera_x, key_state_fire);
            ui_system.update();
            
            // Lives count-up sequence: award 5 lives with 1-tick delay between each,
            // then subtract 1 (the life currently in use)
            if (!lives_sequence_complete) {
                if (lives_sequence_counter > 0) {
                    lives_sequence_delay--;
                    if (lives_sequence_delay == 0) {
                        // Award a life
                        comic_num_lives++;
                        lives_sequence_counter--;
                        
                        if (lives_sequence_counter > 0) {
                            // Still more lives to award (wait 1 tick)
                            lives_sequence_delay = 1;
                        } else {
                            // Awarded all 5, wait 3 ticks then subtract 1
                            lives_sequence_delay = 3;
                        }
                    }
                } else if (lives_sequence_delay > 0) {
                    // Waiting 3 ticks after awarding all 5 lives
                    lives_sequence_delay--;
                    if (lives_sequence_delay == 0) {
                        // Subtract 1 life (currently in use: 5→4)
                        if (comic_num_lives > 0) {
                            comic_num_lives--;
                        }
                        lives_sequence_complete = true;
                    }
                }
            }
            
            // Gradually fill HP from 0 to MAX_HP at game startup
            // Each tick, increment HP if pending increase is scheduled
            if (comic_hp_pending_increase > 0) {
                comic_hp_pending_increase--;
                if (comic_hp < MAX_HP) {
                    comic_hp++;
                }
            }
            
            // Fireball meter charging is handled by ActorSystem::update()
            // (charges at 1 unit per 2 ticks when not firing)
            
            // Game-over is handled by physics when Comic hits the bottom of playfield
            // (sound triggered there).  No additional check needed here.
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

        // Keep gameplay aligned with the same letterboxed 320x200 frame used by HUD.
        SDL_Rect gameplay_frame_rect = g_graphics->compute_letterbox_rect(renderer);

        // Render HUD background (sys003.ega) - should be behind all game elements
        SDL_Texture* hud_texture = get_hud_texture();
        if (hud_texture) {
            SDL_RenderCopy(renderer, hud_texture, nullptr, &gameplay_frame_rect);
        }

        // Calculate the letterbox scale factor to keep playfield dimensions correct
        // as the window size changes.
        const float letterbox_scale = static_cast<float>(gameplay_frame_rect.w) / EGA_WIDTH;

        // Derive per-frame render scale from letterbox scale.
        // Each game unit = 8 EGA pixels (192 EGA pixels / 24 game units),
        // so gameplay scales consistently with the HUD.
        // Round to nearest int and clamp to minimum 1 to prevent zero-sized rendering.
        const int render_scale = (letterbox_scale < 0.125f) ? 1 : static_cast<int>(8.0f * letterbox_scale + 0.5f);

        // Render gameplay only inside the HUD playfield window
        // (C code: top-left at 8,8 EGA pixels; size 192x160 EGA pixels).
        SDL_Rect playfield_viewport;
        playfield_viewport.x = gameplay_frame_rect.x + static_cast<int>(8 * letterbox_scale);
        playfield_viewport.y = gameplay_frame_rect.y + static_cast<int>(8 * letterbox_scale);
        playfield_viewport.w = static_cast<int>(192 * letterbox_scale);
        playfield_viewport.h = static_cast<int>(160 * letterbox_scale);
        SDL_RenderSetViewport(renderer, &playfield_viewport);

        bool level_changed = current_level_number != cached_level_number;
        bool stage_changed = current_stage_number != cached_stage_number;

        // Update tileset cache if level changed
        if (level_changed) {
            cached_level_number = current_level_number;
            if (current_level_number < 8) {
                cached_tileset = g_graphics->get_tileset(level_names[current_level_number]);
            }
        }

        if (level_changed || stage_changed) {
            cached_stage_number = current_stage_number;
            if (current_level_ptr) {
                actor_system.setup_enemies_for_stage(current_level_ptr, current_level_number, current_stage_number, g_graphics);
            }
        }

        Tileset* tileset = cached_tileset;

        // Render tiles with offscreen margin for seamless scrolling.
        // The C code pre-renders into a 256-byte-wide buffer; we render tiles
        // slightly outside the visible playfield and let viewport clipping
        // hide any offscreen portions.
        const int OFFSCREEN_MARGIN_UNITS = 2;
        const int min_visible_x = camera_x - OFFSCREEN_MARGIN_UNITS;
        const int max_visible_x = camera_x + PLAYFIELD_WIDTH + OFFSCREEN_MARGIN_UNITS;
        
        for (int ty = 0; ty < MAP_HEIGHT_TILES; ty++) {
            for (int tx = 0; tx < MAP_WIDTH_TILES; tx++) {
                int world_x = tx * 2; // Tile x in game units
                
                // Render tiles within visible range (with left margin for scrolling).
                // Each tile is 2 units wide, so render if tile overlaps the range.
                if (world_x + 2 > min_visible_x && world_x < max_visible_x) {
                    uint8_t tile = get_tile_at(tx * 2, ty * 2);
                    
                    int screen_x = (world_x - camera_x) * render_scale;
                    int screen_y = ty * 2 * render_scale;
                    
                    g_graphics->render_tile(screen_x, screen_y, tileset, tile, render_scale);
                }
            }
        }

        actor_system.render_enemies(g_graphics, camera_x, render_scale);
        actor_system.render_fireballs(g_graphics, camera_x, render_scale);
        actor_system.render_item(g_graphics, camera_x, render_scale);

        // Render player sprite
        if (current_animation) {
            AnimationFrame* frame = g_graphics->get_current_frame(*current_animation);
            if (frame) {
                // Center player on screen relative to camera
                // Player is 2 units wide, 4 units tall in game coords
                int screen_x = (comic_x - camera_x) * render_scale + render_scale;  // Center X
                int screen_y = comic_y * render_scale + render_scale * 2; // Center Y
                int player_width = render_scale * 2;
                int player_height = render_scale * 4;
                g_graphics->render_sprite_centered_scaled(screen_x, screen_y, frame->sprite, player_width, player_height);
            }
        }
        
        // Restore full renderer viewport before rendering debug overlay.
        SDL_RenderSetViewport(renderer, nullptr);
        
        // Render debug overlay if enabled via F3 (in full window space)
        if (g_cheats->should_show_debug_overlay()) {
            g_graphics->render_debug_overlay();
        }

        // Render HUD (score, lives, HP, fireball meter, inventory)
        // Draw in the same 320x200 letterboxed coordinate space as SYS003.
        SDL_RenderSetViewport(renderer, &gameplay_frame_rect);
        SDL_RenderSetScale(renderer, letterbox_scale, letterbox_scale);
        ui_system.render_hud(
            score_bytes,
            comic_num_lives,
            comic_hp,
            actor_system.fireball_meter,
            actor_system.comic_firepower,
            actor_system.comic_has_corkscrew != 0,
            comic_has_door_key != 0,
            actor_system.comic_has_teleport_wand != 0,
            actor_system.comic_has_lantern != 0,
            actor_system.comic_has_gems != 0,
            actor_system.comic_has_crown != 0,
            actor_system.comic_has_gold != 0,
            comic_jump_power
        );
        SDL_RenderSetScale(renderer, 1.0f, 1.0f);
        SDL_RenderSetViewport(renderer, nullptr);
        // Present
        SDL_RenderPresent(renderer);

        // Cap rendering to ~60 FPS while physics runs at ~9.1 Hz
        if (tick_accumulator >= MS_PER_TICK) {
            SDL_Delay(0);
        } else {
            SDL_Delay(16);
        }
    }

    // Cleanup
    g_actor_system = nullptr;  // Clear pointer before actor_system goes out of scope
    delete g_cheats;
    cleanup_title_sequence();
    delete g_graphics;
    shutdown_audio_system();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}