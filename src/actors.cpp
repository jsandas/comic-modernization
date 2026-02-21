#include "actors.h"
#include "level.h"
#include "physics.h"
#include <iostream>
#include <cstring>

/**
 * ActorSystem constructor
 */
ActorSystem::ActorSystem()
    : enemies(MAX_NUM_ENEMIES),
      current_tiles(nullptr),
      current_map_width_tiles(128),
      current_map_height_tiles(10),
      tileset_last_passable(0x3E),
      spawned_this_tick(0),
      spawn_offset_cycle(PLAYFIELD_WIDTH),
      enemy_respawn_counter_cycle(20),
      g_comic_x(0),
      g_comic_y(0),
      g_comic_facing(COMIC_FACING_LEFT),
      g_camera_x(0) {
    for (auto& enemy : enemies) {
        enemy.state = ENEMY_STATE_DESPAWNED;
        enemy.spawn_timer_and_animation = 100;
        enemy.animation_data = nullptr;
    }
}

/**
 * ActorSystem destructor
 */
ActorSystem::~ActorSystem() {
    // Cleanup handled by SDL/graphics system
}

/**
 * Initialize the actor system
 */
bool ActorSystem::initialize() {
    // Actor system is initialized on per-stage basis via setup_enemies_for_stage
    return true;
}

/**
 * Reset all enemies for a new stage
 */
void ActorSystem::reset_for_stage() {
    for (auto& enemy : enemies) {
        enemy.state = ENEMY_STATE_DESPAWNED;
        enemy.spawn_timer_and_animation = enemy_respawn_counter_cycle;
    }
    spawned_this_tick = 0;
    spawn_offset_cycle = PLAYFIELD_WIDTH;
}

/**
 * Setup enemies for a stage from level data
 */
void ActorSystem::setup_enemies_for_stage(
    const level_t* level,
    int stage_number,
    GraphicsSystem* graphics_system) {
    if (!level || stage_number < 0 || stage_number >= 3) {
        return;
    }

    const stage_t& stage = level->stages[stage_number];
    tileset_last_passable = level->tileset_last_passable;

    // Initialize each enemy slot from stage data
    for (int i = 0; i < MAX_NUM_ENEMIES; i++) {
        const enemy_record_t& record = stage.enemies[i];
        enemy_t& enemy = enemies[i];

        // Check if slot is used
        if ((record.behavior & ~ENEMY_BEHAVIOR_FAST) >= ENEMY_BEHAVIOR_UNUSED) {
            enemy.state = ENEMY_STATE_DESPAWNED;
            enemy.spawn_timer_and_animation = 100;
            enemy.animation_data = nullptr;
            continue;
        }

        // Load animation data from sprite descriptor
        const shp_t& sprite_desc = level->shp[record.shp_index];
        enemy.num_animation_frames = sprite_desc.num_distinct_frames;
        enemy.behavior = record.behavior;

        // Load sprite animation from graphics system
        std::string sprite_name = sprite_desc.filename;
        // Remove null padding if present
        size_t null_pos = sprite_name.find('\0');
        if (null_pos != std::string::npos) {
            sprite_name = sprite_name.substr(0, null_pos);
        }

        enemy.animation_data = graphics_system->load_enemy_sprite(sprite_name);
        if (!enemy.animation_data) {
            std::cerr << "Failed to load sprite: " << sprite_name << std::endl;
            enemy.state = ENEMY_STATE_DESPAWNED;
            enemy.animation_data = nullptr;
            continue;
        }

        // Set initial state
        enemy.state = ENEMY_STATE_DESPAWNED;
        enemy.spawn_timer_and_animation = 0;
        enemy.x_vel = 0;
        enemy.y_vel = 0;
        enemy.facing = COMIC_FACING_LEFT;
        enemy.restraint = ENEMY_RESTRAINT_MOVE_THIS_TICK;
    }

    reset_for_stage();
}

/**
 * Main update function - called once per game tick
 */
void ActorSystem::update(
    uint8_t comic_x, uint8_t comic_y,
    uint8_t comic_facing,
    const uint8_t* current_tiles,
    int camera_x) {
    // Store global state for use by behavior functions
    g_comic_x = comic_x;
    g_comic_y = comic_y;
    g_comic_facing = comic_facing;
    g_camera_x = camera_x;
    current_tiles = current_tiles;

    spawned_this_tick = 0;

    for (int i = 0; i < MAX_NUM_ENEMIES; i++) {
        enemy_t& enemy = enemies[i];

        // Handle despawned state
        if (enemy.state == ENEMY_STATE_DESPAWNED) {
            if (enemy.spawn_timer_and_animation > 0) {
                enemy.spawn_timer_and_animation--;
            }
            if (enemy.spawn_timer_and_animation == 0) {
                maybe_spawn_enemy(i);
            }
            continue;
        }

        // Handle death animation states (white spark or red spark)
        if (enemy.state >= ENEMY_STATE_WHITE_SPARK && enemy.state != ENEMY_STATE_SPAWNED) {
            // Still animating spark effect
            if ((enemy.state == ENEMY_STATE_WHITE_SPARK + 5) ||
                (enemy.state == ENEMY_STATE_RED_SPARK + 5)) {
                // Animation finished; despawn
                enemy.state = ENEMY_STATE_DESPAWNED;
                enemy.spawn_timer_and_animation = enemy_respawn_counter_cycle;
                
                // Cycle respawn: 20→40→60→80→100→20
                enemy_respawn_counter_cycle += 20;
                if (enemy_respawn_counter_cycle > 100) {
                    enemy_respawn_counter_cycle = 20;
                }
            } else {
                // Advance animation frame
                enemy.state++;
            }
            continue;
        }

        // Handle spawned state
        if (enemy.state == ENEMY_STATE_SPAWNED) {
            update_enemy_animation(&enemy);
            handle_single_enemy(i);
            check_enemy_despawn(&enemy);
            check_enemy_player_collision(&enemy);
        }
    }
}

/**
 * Update animation frame
 */
void ActorSystem::update_enemy_animation(enemy_t* enemy) {
    if (!enemy || enemy->num_animation_frames == 0) {
        return;
    }

    enemy->spawn_timer_and_animation++;
    if (enemy->spawn_timer_and_animation >= enemy->num_animation_frames) {
        enemy->spawn_timer_and_animation = 0;  // Loop animation
    }
}

/**
 * Try to spawn an enemy from despawned state
 */
bool ActorSystem::maybe_spawn_enemy(int enemy_index) {
    if (enemy_index < 0 || enemy_index >= MAX_NUM_ENEMIES) {
        return false;
    }

    // Only spawn one enemy per tick
    if (spawned_this_tick) {
        return false;
    }

    enemy_t& enemy = enemies[enemy_index];

    // Check if slot is used
    if ((enemy.behavior & ~ENEMY_BEHAVIOR_FAST) >= ENEMY_BEHAVIOR_UNUSED) {
        enemy.state = ENEMY_STATE_DESPAWNED;
        enemy.spawn_timer_and_animation = 100;
        return false;
    }

    // Determine spawn position
    spawn_offset_cycle += 2;
    if (spawn_offset_cycle >= PLAYFIELD_WIDTH + 7) {
        spawn_offset_cycle = PLAYFIELD_WIDTH;
    }

    uint8_t spawn_x;
    if (g_comic_facing == COMIC_FACING_RIGHT) {
        // Spawn to the right
        spawn_x = static_cast<uint8_t>(g_camera_x + spawn_offset_cycle);
    } else {
        // Spawn to the left
        spawn_x = static_cast<uint8_t>(g_camera_x - (spawn_offset_cycle - PLAYFIELD_WIDTH + 2));
    }

    // Determine spawn Y: search for passable tile at Comic's foot level or higher
    uint8_t spawn_y = g_comic_y;
    for (int y_search = 0; y_search < 2; y_search++) {
        if (!is_tile_solid(get_tile_at(spawn_x, spawn_y))) {
            break;  // Found passable tile
        }
        spawn_y = (uint8_t)(spawn_y - 1);
    }

    // Initialize spawned enemy
    spawned_this_tick = 1;
    enemy.x = spawn_x;
    enemy.y = spawn_y;
    enemy.state = ENEMY_STATE_SPAWNED;
    enemy.spawn_timer_and_animation = 0;  // Start animation at frame 0

    // Initialize velocities based on behavior type
    uint8_t behavior_type = enemy.behavior & ~ENEMY_BEHAVIOR_FAST;
    switch (behavior_type) {
        case ENEMY_BEHAVIOR_BOUNCE:
        case ENEMY_BEHAVIOR_SHY:
            enemy.x_vel = -1;  // Start moving left
            enemy.y_vel = -1;  // Start moving up
            enemy.facing = COMIC_FACING_LEFT;
            break;

        case ENEMY_BEHAVIOR_LEAP:
        case ENEMY_BEHAVIOR_ROLL:
        case ENEMY_BEHAVIOR_SEEK:
        default:
            enemy.x_vel = 0;
            enemy.y_vel = 0;
            enemy.facing = COMIC_FACING_LEFT;
            break;
    }

    // Set restraint based on speed flag
    if (enemy.behavior & ENEMY_BEHAVIOR_FAST) {
        enemy.restraint = ENEMY_RESTRAINT_MOVE_EVERY_TICK;
    } else {
        enemy.restraint = ENEMY_RESTRAINT_MOVE_THIS_TICK;
    }

    return true;
}

/**
 * Handle a single spawned enemy's AI update
 */
void ActorSystem::handle_single_enemy(int enemy_index) {
    if (enemy_index < 0 || enemy_index >= MAX_NUM_ENEMIES) {
        return;
    }

    enemy_t& enemy = enemies[enemy_index];
    uint8_t behavior_type = enemy.behavior & ~ENEMY_BEHAVIOR_FAST;

    switch (behavior_type) {
        case ENEMY_BEHAVIOR_BOUNCE:
            enemy_behavior_bounce(&enemy);
            break;
        case ENEMY_BEHAVIOR_LEAP:
            enemy_behavior_leap(&enemy);
            break;
        case ENEMY_BEHAVIOR_ROLL:
            enemy_behavior_roll(&enemy);
            break;
        case ENEMY_BEHAVIOR_SEEK:
            enemy_behavior_seek(&enemy);
            break;
        case ENEMY_BEHAVIOR_SHY:
            enemy_behavior_shy(&enemy);
            break;
        default:
            break;
    }
}

/**
 * Check if enemy should despawn due to distance
 */
void ActorSystem::check_enemy_despawn(enemy_t* enemy) {
    if (!enemy) return;

    int16_t x_diff = static_cast<int16_t>(static_cast<int>(enemy->x) - static_cast<int>(g_comic_x));
    if (x_diff < -ENEMY_DESPAWN_RADIUS || x_diff > ENEMY_DESPAWN_RADIUS) {
        enemy->state = ENEMY_STATE_DESPAWNED;
        enemy->spawn_timer_and_animation = enemy_respawn_counter_cycle;
    }
}

/**
 * Check collision between enemy and player
 */
void ActorSystem::check_enemy_player_collision(enemy_t* enemy) {
    if (!enemy) return;

    int16_t x_diff = static_cast<int16_t>(static_cast<int>(enemy->x) - static_cast<int>(g_comic_x));
    int16_t y_diff = static_cast<int16_t>(static_cast<int>(enemy->y) - static_cast<int>(g_comic_y));

    // Collision box: horizontal abs(enemy.x - comic.x) < 2, vertical 0 <= (enemy.y - comic.y) < 4
    if (x_diff >= -1 && x_diff <= 1 && y_diff >= 0 && y_diff < 4) {
        // Collision! Start red spark death animation
        enemy->state = ENEMY_STATE_RED_SPARK;
        // TODO: Handle damage to Comic (shield loss, HP loss, or death)
    }
}

/**
 * Check if a tile is solid
 */
bool ActorSystem::is_tile_solid(uint8_t tile_id) const {
    return tile_id > tileset_last_passable;
}

/**
 * Get tile at coordinates
 */
uint8_t ActorSystem::get_tile_at(uint8_t x, uint8_t y) const {
    if (!current_tiles) {
        return 0;  // Passable if no tile data available
    }

    // Each tile is 2 game units
    uint8_t tile_x = x / 2;
    uint8_t tile_y = y / 2;

    if (tile_x >= current_map_width_tiles || tile_y >= current_map_height_tiles) {
        return 0;  // Out of bounds = passable
    }

    uint16_t index = tile_y * current_map_width_tiles + tile_x;
    return current_tiles[index];
}

/**
 * Check horizontal collision for enemy at (x, y)
 * Uses 2 tiles ahead for look-ahead
 */
bool ActorSystem::check_horizontal_enemy_map_collision(uint8_t x, uint8_t y) const {
    // Check 2 tiles (4 game units) ahead
    return is_tile_solid(get_tile_at(x + 1, y)) ||
           is_tile_solid(get_tile_at(x + 1, y - 1));
}

/**
 * Check vertical collision for enemy at (x, y)
 * Uses 2 tiles below for look-ahead
 */
bool ActorSystem::check_vertical_enemy_map_collision(uint8_t x, uint8_t y) const {
    // Check 2 tiles (4 game units) below
    return is_tile_solid(get_tile_at(x, y + 1)) ||
           is_tile_solid(get_tile_at(x - 1, y + 1));
}

// ============================================================================
// AI BEHAVIORS
// ============================================================================

/**
 * BOUNCE behavior: Diagonal bouncing with independent x/y velocities
 * Used by: Fire Ball, Brave Bird
 */
void ActorSystem::enemy_behavior_bounce(enemy_t* enemy) {
    if (!enemy) return;

    // Handle restraint (movement throttle)
    if (enemy->restraint == ENEMY_RESTRAINT_SKIP_THIS_TICK) {
        enemy->restraint = ENEMY_RESTRAINT_MOVE_THIS_TICK;
        return;  // Skip this tick
    }

    if (enemy->restraint == ENEMY_RESTRAINT_MOVE_THIS_TICK) {
        enemy->restraint = ENEMY_RESTRAINT_SKIP_THIS_TICK;  // Skip next tick
    }

    // Horizontal movement
    uint8_t next_x;
    int16_t camera_rel_x;

    if (enemy->x_vel > 0) {
        // Moving right
        enemy->facing = COMIC_FACING_RIGHT;
        next_x = static_cast<uint8_t>(enemy->x + 2);
        if (check_horizontal_enemy_map_collision(next_x, enemy->y)) {
            enemy->x_vel = -1;  // Bounce left
        } else {
            enemy->x = static_cast<uint8_t>(enemy->x + 1);
            camera_rel_x = static_cast<int16_t>(enemy->x) - static_cast<int16_t>(g_camera_x);
            if (camera_rel_x >= PLAYFIELD_WIDTH - 2) {
                enemy->x_vel = -1;  // Hit right edge, bounce
            }
        }
    } else {
        // Moving left
        enemy->facing = COMIC_FACING_LEFT;
        if (enemy->x == 0) {
            enemy->x_vel = 1;  // Hit left edge, bounce
        } else {
            next_x = static_cast<uint8_t>(enemy->x - 1);
            if (check_horizontal_enemy_map_collision(next_x, enemy->y)) {
                enemy->x_vel = 1;  // Bounce right
            } else {
                enemy->x = next_x;
                camera_rel_x = static_cast<int16_t>(enemy->x) - static_cast<int16_t>(g_camera_x);
                if (camera_rel_x <= 0) {
                    enemy->x_vel = 1;  // Hit left edge, bounce
                }
            }
        }
    }

    // Vertical movement
    uint8_t next_y;

    if (enemy->y_vel > 0) {
        // Moving down
        if (enemy->y >= PLAYFIELD_HEIGHT - 2) {
            enemy->y_vel = -1;  // Hit bottom, bounce up
        } else {
            next_y = static_cast<uint8_t>(enemy->y + 2);
            if (check_vertical_enemy_map_collision(enemy->x, next_y)) {
                enemy->y_vel = -1;  // Hit solid tile, bounce
            } else {
                enemy->y = static_cast<uint8_t>(enemy->y + 1);
                if (enemy->y >= PLAYFIELD_HEIGHT - 2) {
                    enemy->y_vel = -1;
                }
            }
        }
    } else {
        // Moving up
        if (enemy->y == 0) {
            enemy->y_vel = 1;  // Hit top, bounce down
        } else {
            next_y = static_cast<uint8_t>(enemy->y - 1);
            if (check_vertical_enemy_map_collision(enemy->x, next_y)) {
                enemy->y_vel = 1;  // Hit solid tile, bounce
            } else {
                enemy->y = next_y;
                if (enemy->y == 0) {
                    enemy->y_vel = 1;
                }
            }
        }
    }
}

/**
 * LEAP behavior: Jumping arc with gravity
 * Used by: Bug-eyes, Blind Toad, Beach Ball
 */
void ActorSystem::enemy_behavior_leap(enemy_t* enemy) {
    if (!enemy) return;

    uint8_t next_x, next_y;
    int16_t camera_rel_x;
    bool collision = false;

    // Handle restraint
    if (enemy->restraint == ENEMY_RESTRAINT_SKIP_THIS_TICK) {
        enemy->restraint = ENEMY_RESTRAINT_MOVE_THIS_TICK;
        return;
    }

    if (enemy->restraint == ENEMY_RESTRAINT_MOVE_THIS_TICK) {
        enemy->restraint = ENEMY_RESTRAINT_SKIP_THIS_TICK;
    }

    // Vertical velocity handling and gravity
    uint8_t proposed_y = enemy->y;

    if (enemy->y_vel < 0) {
        // Moving up - apply upward movement with fractional velocity
        int8_t vel_div_8 = enemy->y_vel >> 3;  // y_vel / 8
        uint8_t target_y = static_cast<uint8_t>(proposed_y + vel_div_8);
        
        if (target_y >= 254) {  // Underflow check
            proposed_y = 0;
        } else {
            collision = check_vertical_enemy_map_collision(enemy->x, target_y);
            if (!collision) {
                proposed_y = target_y;
            }
        }
    } else if (enemy->y_vel > 0) {
        // Moving down
        int8_t vel_div_8 = enemy->y_vel >> 3;
        proposed_y = static_cast<uint8_t>(proposed_y + vel_div_8);
    }

    // Apply gravity
    enemy->y_vel += 2;  // Gravity
    if (enemy->y_vel > TERMINAL_VELOCITY) {
        enemy->y_vel = TERMINAL_VELOCITY;
    }

    // Horizontal movement toward Comic's x position
    if (enemy->x_vel > 0) {
        // Moving right
        next_x = static_cast<uint8_t>(enemy->x + 2);
        collision = check_horizontal_enemy_map_collision(next_x, proposed_y);
        if (collision) {
            enemy->x_vel = -1;  // Bounce left
        } else {
            enemy->x = static_cast<uint8_t>(enemy->x + 1);
            camera_rel_x = static_cast<int16_t>(enemy->x) - static_cast<int16_t>(g_camera_x);
            if (camera_rel_x >= PLAYFIELD_WIDTH - 2) {
                enemy->x_vel = -1;  // Hit right edge
            }
        }
    } else if (enemy->x_vel < 0) {
        // Moving left
        next_x = static_cast<uint8_t>(enemy->x - 1);
        collision = check_horizontal_enemy_map_collision(next_x, proposed_y);
        if (collision) {
            enemy->x_vel = 1;  // Bounce right
        } else {
            enemy->x = next_x;
            camera_rel_x = static_cast<int16_t>(enemy->x) - static_cast<int16_t>(g_camera_x);
            if (camera_rel_x <= 0) {
                enemy->x_vel = 1;  // Hit left edge
            }
        }
    } else {
        // x_vel == 0: determine direction toward Comic
        if (enemy->x < g_comic_x) {
            enemy->x_vel = 1;
        } else if (enemy->x > g_comic_x) {
            enemy->x_vel = -1;
        }
    }

    // Vertical movement and ground detection
    if (enemy->y_vel <= 0) {
        // Moving up or stationary - check for ground
        collision = check_vertical_enemy_map_collision(enemy->x, static_cast<uint8_t>(enemy->y + 2));
        if (!collision) {
            // No ground - start falling
            enemy->y_vel = 1;
        } else {
            // Ground exists - initiate jump
            enemy->x_vel = (enemy->x < g_comic_x) ? 1 : (enemy->x > g_comic_x) ? -1 : 0;
            enemy->y_vel = -7;  // Jump velocity
        }
    }

    // Apply vertical position (calculated at top with velocity/8 for smooth movement)
    enemy->y = proposed_y;

    // Check if enemy fell off bottom of playfield
    if (enemy->y >= PLAYFIELD_HEIGHT - 2) {
        enemy->state = ENEMY_STATE_WHITE_SPARK + 5;
        enemy->y = PLAYFIELD_HEIGHT - 2;
        return;
    }
}

/**
 * ROLL behavior: Ground-following movement
 * Used by: Glow Globe
 */
void ActorSystem::enemy_behavior_roll(enemy_t* enemy) {
    if (!enemy) return;

    uint8_t next_x;

    // Vertical movement: falling or on ground
    if (enemy->y_vel > 0) {
        // Falling: check if near bottom and despawn
        if (enemy->y + 1 >= PLAYFIELD_HEIGHT - 3) {
            enemy->state = ENEMY_STATE_WHITE_SPARK + 5;
            enemy->y = PLAYFIELD_HEIGHT - 2;
            return;
        }
        // Move down one unit (maintains horizontal momentum from before falling)
        enemy->y = static_cast<uint8_t>(enemy->y + 1);
    } else {
        // On ground: update direction toward Comic
        if (enemy->x < g_comic_x) {
            enemy->x_vel = 1;
        } else if (enemy->x > g_comic_x) {
            enemy->x_vel = -1;
        } else {
            enemy->x_vel = 0;
        }
    }

    // Handle restraint (applies to both falling and rolling)
    if (enemy->restraint == ENEMY_RESTRAINT_SKIP_THIS_TICK) {
        enemy->restraint = ENEMY_RESTRAINT_MOVE_THIS_TICK;
        return;
    }

    if (enemy->restraint == ENEMY_RESTRAINT_MOVE_THIS_TICK) {
        enemy->restraint = ENEMY_RESTRAINT_SKIP_THIS_TICK;
    }

    // Horizontal movement
    if (enemy->x_vel == 0) {
        enemy->restraint = ENEMY_RESTRAINT_MOVE_THIS_TICK;
        return;
    }

    if (enemy->x_vel > 0) {
        // Moving right
        next_x = static_cast<uint8_t>(enemy->x + 2);
        if (!check_horizontal_enemy_map_collision(next_x, enemy->y)) {
            enemy->x = static_cast<uint8_t>(enemy->x + 1);
        }
    } else {
        // Moving left
        next_x = static_cast<uint8_t>(enemy->x - 1);
        if (!check_horizontal_enemy_map_collision(next_x, enemy->y)) {
            enemy->x = next_x;
        }
    }

    // Check for ground below
    if (!check_vertical_enemy_map_collision(enemy->x, static_cast<uint8_t>(enemy->y + 3))) {
        // No ground - start falling
        enemy->y_vel = 1;
        return;
    }

    // On ground
    enemy->y_vel = 0;
}

/**
 * SEEK behavior: Pathfinding toward player
 * Used by: Killer Bee
 */
void ActorSystem::enemy_behavior_seek(enemy_t* enemy) {
    if (!enemy) return;

    uint8_t next_x, next_y;
    unsigned char collision;

    // Handle restraint
    if (enemy->restraint == ENEMY_RESTRAINT_SKIP_THIS_TICK) {
        enemy->restraint = ENEMY_RESTRAINT_MOVE_THIS_TICK;
        return;
    }

    if (enemy->restraint == ENEMY_RESTRAINT_MOVE_THIS_TICK) {
        enemy->restraint = ENEMY_RESTRAINT_SKIP_THIS_TICK;
    }

    // Horizontal movement toward Comic (prioritized)
    if (enemy->x != g_comic_x) {
        if (enemy->x < g_comic_x) {
            // Move right
            next_x = static_cast<uint8_t>(enemy->x + 1);
            collision = check_horizontal_enemy_map_collision(static_cast<uint8_t>(next_x + 1), enemy->y);

            if (!collision) {
                enemy->x = next_x;
                enemy->x_vel = 1;
            } else {
                enemy->x_vel = -1;  // Blocked, try left next time
            }
        } else {
            // Move left
            next_x = static_cast<uint8_t>(enemy->x - 1);
            collision = check_horizontal_enemy_map_collision(next_x, enemy->y);

            if (!collision) {
                enemy->x = next_x;
                enemy->x_vel = -1;
            } else {
                enemy->x_vel = 1;  // Blocked, try right next time
            }
        }

        enemy->facing = (enemy->x_vel < 0) ? COMIC_FACING_LEFT : COMIC_FACING_RIGHT;
        return;  // Continue to next tick after x movement
    }

    // Vertical movement when x is aligned
    if (enemy->y != g_comic_y) {
        if (enemy->y < g_comic_y) {
            // Move down
            next_y = static_cast<uint8_t>(enemy->y + 1);
            collision = check_vertical_enemy_map_collision(enemy->x, static_cast<uint8_t>(next_y + 1));

            if (!collision) {
                enemy->y = next_y;
                enemy->y_vel = 1;
            } else {
                enemy->y_vel = -1;
            }
        } else {
            // Move up
            next_y = static_cast<uint8_t>(enemy->y - 1);
            collision = check_vertical_enemy_map_collision(enemy->x, next_y);

            if (!collision) {
                enemy->y = next_y;
                enemy->y_vel = -1;
            } else {
                enemy->y_vel = 1;
            }
        }
    }

    enemy->facing = (enemy->x_vel < 0) ? COMIC_FACING_LEFT : COMIC_FACING_RIGHT;
}

/**
 * SHY behavior: Flees when Comic is facing toward, approaches otherwise
 * Used by: Shy Bird, Spinner
 */
void ActorSystem::enemy_behavior_shy(enemy_t* enemy) {
    if (!enemy) return;

    uint8_t next_x, next_y;
    int8_t comic_facing_enemy;
    unsigned char collision;
    int16_t camera_rel_x;

    // Check restraint
    if (enemy->restraint > 0) {
        enemy->restraint--;
        return;  // Skip movement this tick
    }

    // Determine if Comic is facing this enemy
    if (g_comic_facing == COMIC_FACING_RIGHT && enemy->x > g_comic_x) {
        comic_facing_enemy = 1;  // Comic facing enemy on right
    } else if (g_comic_facing == COMIC_FACING_LEFT && enemy->x < g_comic_x) {
        comic_facing_enemy = 1;  // Comic facing enemy on left
    } else {
        comic_facing_enemy = 0;  // Comic facing away
    }

    // Horizontal movement
    if (enemy->x_vel > 0) {
        // Moving right
        enemy->facing = COMIC_FACING_RIGHT;
        next_x = static_cast<uint8_t>(enemy->x + 2);
        collision = check_horizontal_enemy_map_collision(next_x, enemy->y);
        if (collision) {
            enemy->x_vel = -1;
        } else {
            enemy->x = static_cast<uint8_t>(enemy->x + 1);
            camera_rel_x = static_cast<int16_t>(enemy->x) - static_cast<int16_t>(g_camera_x);
            if (camera_rel_x >= PLAYFIELD_WIDTH - 2) {
                enemy->x_vel = -1;  // Hit right edge
            }
        }
    } else {
        // Moving left
        enemy->facing = COMIC_FACING_LEFT;
        if (enemy->x == 0) {
            enemy->x_vel = 1;
        } else {
            next_x = static_cast<uint8_t>(enemy->x - 1);
            collision = check_horizontal_enemy_map_collision(next_x, enemy->y);
            if (collision) {
                enemy->x_vel = 1;
            } else {
                enemy->x = next_x;
                camera_rel_x = static_cast<int16_t>(enemy->x) - static_cast<int16_t>(g_camera_x);
                if (camera_rel_x <= 0) {
                    enemy->x_vel = 1;  // Hit left edge
                }
            }
        }
    }

    // Vertical movement depends on whether Comic is facing this enemy
    if (comic_facing_enemy) {
        // Comic is facing this enemy - move up (flee)
        if (enemy->y_vel <= 0) {
            enemy->y_vel = -1;  // Keep moving up
        }
    } else {
        // Comic is facing away - move toward Comic's y (approach)
        if (enemy->y < g_comic_y) {
            enemy->y_vel = 1;  // Move down
        } else if (enemy->y > g_comic_y) {
            enemy->y_vel = -1;  // Move up
        } else {
            enemy->y_vel = 0;  // Aligned
        }
    }

    // Apply vertical movement
    if (enemy->y_vel > 0) {
        // Moving down
        next_y = static_cast<uint8_t>(enemy->y + 2);
        collision = check_vertical_enemy_map_collision(enemy->x, next_y);
        if (collision) {
            enemy->y_vel = -1;  // Bounce up
        } else {
            enemy->y = static_cast<uint8_t>(enemy->y + 1);
            if (enemy->y >= PLAYFIELD_HEIGHT - 2) {
                enemy->y_vel = -1;
            }
        }
    } else {
        // Moving up or stationary
        if (enemy->y == 0) {
            enemy->y_vel = 1;  // Bounce down
        } else {
            next_y = static_cast<uint8_t>(enemy->y - 1);
            collision = check_vertical_enemy_map_collision(enemy->x, next_y);
            if (!collision) {
                enemy->y = next_y;
            }
        }
    }

    // Normalize restraint state
    if (enemy->restraint == ENEMY_RESTRAINT_SKIP_THIS_TICK) {
        enemy->restraint = ENEMY_RESTRAINT_MOVE_THIS_TICK;
    } else if (enemy->restraint == ENEMY_RESTRAINT_MOVE_THIS_TICK) {
        enemy->restraint = ENEMY_RESTRAINT_SKIP_THIS_TICK;
    } else {
        if (enemy->restraint > ENEMY_RESTRAINT_MOVE_EVERY_TICK) {
            enemy->restraint = ENEMY_RESTRAINT_MOVE_THIS_TICK;
        }
    }
}
