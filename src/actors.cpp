#include "actors.h"
#include "level.h"
#include "physics.h"
#include <iostream>

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
      enemy_respawn_counter_cycle(RESPAWN_TIMER_MIN),
      g_comic_x(0),
      g_comic_y(0),
      g_comic_facing(COMIC_FACING_LEFT),
      g_camera_x(0) {
    for (auto& enemy : enemies) {
        enemy.state = ENEMY_STATE_DESPAWNED;
        enemy.spawn_timer_and_animation = 100;
        enemy.sprite_descriptor = nullptr;
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
    // NOTE: spawn_offset_cycle is intentionally NOT reset here.
    // In the original game (jsandas/comic-c: src/actors.c maybe_spawn_enemy),
    // it is a static local variable initialized once at program start and never
    // cleared on stage load, so it naturally persists across stage transitions.
    // Persisting it here matches that behavior and adds variety to spawn positions
    // across stage changes.
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

        // Validate sprite descriptor index to avoid out-of-bounds access
        if (record.shp_index >= 4) {
            std::cerr << "Invalid sprite index " << static_cast<int>(record.shp_index) 
                      << " for enemy slot " << i << " (max is 3)" << std::endl;
            enemy.state = ENEMY_STATE_DESPAWNED;
            enemy.spawn_timer_and_animation = 100;
            enemy.sprite_descriptor = nullptr;
            enemy.animation_data = nullptr;
            continue;
        }
        // Load animation data from sprite descriptor
        const shp_t& sprite_desc = level->shp[record.shp_index];
        enemy.sprite_descriptor = &sprite_desc;  // Store reference to source metadata
        enemy.num_animation_frames = sprite_desc.num_distinct_frames;  // Cache for performance
        enemy.behavior = record.behavior;

        // Load sprite animation from graphics system
        enemy.animation_data = graphics_system->load_enemy_sprite(sprite_desc);
        if (!enemy.animation_data) {
            std::string sprite_name = sprite_desc.filename;
            size_t null_pos = sprite_name.find('\0');
            if (null_pos != std::string::npos) {
                sprite_name = sprite_name.substr(0, null_pos);
            }
            while (!sprite_name.empty() && sprite_name.back() == ' ') {
                sprite_name.pop_back();
            }
            std::cerr << "Failed to load sprite animation data for "
                      << (sprite_name.empty() ? "<unknown>" : sprite_name) << std::endl;
            enemy.state = ENEMY_STATE_DESPAWNED;
            enemy.sprite_descriptor = nullptr;
            enemy.animation_data = nullptr;
            continue;
        }

        enemy.num_animation_frames = static_cast<uint8_t>(enemy.animation_data->frame_sequence.size());
        if (enemy.num_animation_frames == 0) {
            std::cerr << "Invalid animation sequence for enemy sprite" << std::endl;
            enemy.state = ENEMY_STATE_DESPAWNED;
            enemy.sprite_descriptor = nullptr;
            enemy.animation_data = nullptr;
            continue;
        }

        // Set initial velocities and facing
        // (state and spawn_timer_and_animation will be set by reset_for_stage())
        enemy.x_vel = 0;
        enemy.y_vel = 0;
        enemy.facing = ENEMY_FACING_LEFT;
        enemy.restraint = ENEMY_RESTRAINT_MOVE_THIS_TICK;
    }

    reset_for_stage();
}

void ActorSystem::render_enemies(GraphicsSystem* graphics_system, int camera_x, int render_scale) const {
    if (!graphics_system) {
        return;
    }

    const int scale_factor = (render_scale * 2) / TILE_SIZE;
    if (scale_factor <= 0) {
        return;
    }

    for (const auto& enemy : enemies) {
        if (enemy.state != ENEMY_STATE_SPAWNED) {
            continue;
        }

        // Cull enemies outside the visible viewport; 2-unit margin covers sprite width
        if (static_cast<int>(enemy.x) < camera_x - 2 ||
            static_cast<int>(enemy.x) >= camera_x + PLAYFIELD_WIDTH + 2) {
            continue;
        }

        if (!enemy.animation_data || enemy.animation_data->frames_left.empty()) {
            continue;
        }

        const auto& sequence = enemy.animation_data->frame_sequence;
        if (sequence.empty()) {
            continue;
        }

        uint8_t sequence_index = enemy.spawn_timer_and_animation % sequence.size();
        uint8_t frame_index = sequence[sequence_index];

        const TextureInfo* frame_info = nullptr;
        bool flip_h = false;

        if (enemy.sprite_descriptor && enemy.sprite_descriptor->horizontal == ENEMY_HORIZONTAL_SEPARATE) {
            const auto& right_frames = enemy.animation_data->frames_right;
            if (enemy.facing == ENEMY_FACING_RIGHT && !right_frames.empty()) {
                frame_info = &right_frames[frame_index % right_frames.size()];
            } else {
                frame_info = &enemy.animation_data->frames_left[frame_index % enemy.animation_data->frames_left.size()];
            }
        } else {
            frame_info = &enemy.animation_data->frames_left[frame_index % enemy.animation_data->frames_left.size()];
            flip_h = (enemy.facing == ENEMY_FACING_RIGHT);
        }

        if (!frame_info || !frame_info->texture) {
            continue;
        }

        int enemy_screen_x = (static_cast<int>(enemy.x) - camera_x) * render_scale + render_scale;
        int enemy_screen_y = static_cast<int>(enemy.y) * render_scale + render_scale;

        int render_width = frame_info->width * scale_factor;
        int render_height = frame_info->height * scale_factor;

        Sprite sprite;
        sprite.texture = *frame_info;
        sprite.width = frame_info->width;
        sprite.height = frame_info->height;

        graphics_system->render_sprite_centered_scaled(
            enemy_screen_x,
            enemy_screen_y,
            sprite,
            render_width,
            render_height,
            flip_h
        );
    }
}

/**
 * Main update function - called once per game tick
 */
void ActorSystem::update(
    uint8_t comic_x, uint8_t comic_y,
    uint8_t comic_facing,
    const uint8_t* tiles,
    int camera_x) {
    // Store global state for use by behavior functions
    g_comic_x = comic_x;
    g_comic_y = comic_y;
    g_comic_facing = comic_facing;
    g_camera_x = camera_x;
    current_tiles = tiles;

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
        if (enemy.state >= ENEMY_STATE_WHITE_SPARK) {
            // Still animating spark effect
            if ((enemy.state == ENEMY_STATE_WHITE_SPARK + DEATH_ANIMATION_LAST_FRAME) ||
                (enemy.state == ENEMY_STATE_RED_SPARK + DEATH_ANIMATION_LAST_FRAME)) {
                // Animation finished; despawn
                enemy.state = ENEMY_STATE_DESPAWNED;
                enemy.spawn_timer_and_animation = enemy_respawn_counter_cycle;
                
                // Cycle respawn timer: 20→40→60→80→100→20
                enemy_respawn_counter_cycle += RESPAWN_TIMER_STEP;
                if (enemy_respawn_counter_cycle > RESPAWN_TIMER_MAX) {
                    enemy_respawn_counter_cycle = RESPAWN_TIMER_MIN;
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

    // Use int for intermediate calculation to avoid overflow
    int spawn_x_temp;
    if (g_comic_facing == COMIC_FACING_RIGHT) {
        // Spawn to the right
        spawn_x_temp = g_camera_x + spawn_offset_cycle;
    } else {
        // Spawn to the left
        spawn_x_temp = g_camera_x - (spawn_offset_cycle - PLAYFIELD_WIDTH + 2);
    }

    // Clamp to valid uint8_t range [0, 255]
    if (spawn_x_temp < 0) {
        spawn_x_temp = 0;
    } else if (spawn_x_temp > 255) {
        spawn_x_temp = 255;
    }
    uint8_t spawn_x = static_cast<uint8_t>(spawn_x_temp);

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
            enemy.facing = ENEMY_FACING_LEFT;
            break;

        case ENEMY_BEHAVIOR_LEAP:
        case ENEMY_BEHAVIOR_ROLL:
        case ENEMY_BEHAVIOR_SEEK:
        default:
            enemy.x_vel = 0;
            enemy.y_vel = 0;
            enemy.facing = ENEMY_FACING_LEFT;
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

    // Collision box: horizontal abs(enemy.x - comic.x) <= 1, vertical 0 <= (enemy.y - comic.y) < 4
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
 * Matches assembly check_horizontal_enemy_map_collision:
 * Checks tile at (x, y). If y is odd (enemy spans two tiles vertically),
 * also checks (x, y+1).
 */
bool ActorSystem::check_horizontal_enemy_map_collision(uint8_t x, uint8_t y) const {
    if (is_tile_solid(get_tile_at(x, y))) {
        return true;
    }
    // If y is odd, enemy spans two vertical tiles - check the one below
    if (y & 1) {
        return is_tile_solid(get_tile_at(x, static_cast<uint8_t>(y + 1)));
    }
    return false;
}

/**
 * Check vertical collision for enemy at (x, y)
 * Matches assembly check_vertical_enemy_map_collision:
 * Checks tile at (x, y). If x is odd (enemy spans two tiles horizontally),
 * also checks (x+1, y).
 */
bool ActorSystem::check_vertical_enemy_map_collision(uint8_t x, uint8_t y) const {
    if (is_tile_solid(get_tile_at(x, y))) {
        return true;
    }
    // If x is odd, enemy spans two horizontal tiles - check the one to the right
    if (x & 1) {
        return is_tile_solid(get_tile_at(static_cast<uint8_t>(x + 1), y));
    }
    return false;
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
        enemy->facing = ENEMY_FACING_RIGHT;
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
        enemy->facing = ENEMY_FACING_LEFT;
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
 * LEAP behavior: Jump toward Comic with gravity
 * Used by: Bug-eyes, Blind Toad, Beach Ball
 *
 * Mirrors R5sw1991.asm:enemy_behavior_leap exactly.
 *
 * Assembly flow:
 *   y_vel < 0  (.moving_up):   delta = y_vel>>3 (arithmetic; e.g. -7>>3 = -1)
 *                               proposed_y += delta  (moves up 1 unit for y_vel=-7)
 *                               undo on ceiling collision or top-of-field underflow
 *   y_vel > 0  (.moving_down): delta = y_vel>>3 (e.g. 8>>3 = 1, move down 1)
 *                               despawn at PLAYFIELD_HEIGHT-2
 *                               undo on solid at proposed_y+1 (.start_falling)
 *   y_vel == 0: check y+2 for solid ground
 *               solid   → .begin_leap: set x_vel toward Comic, y_vel=-7, return
 *               no solid → .start_falling → .apply_gravity (fall through, NO early return)
 *   .apply_gravity: y_vel += 2, clamp to TERMINAL_VELOCITY  (runs for all paths except .begin_leap)
 *   Restraint only skips horizontal movement; gravity and .check_for_ground always run
 *   .check_for_ground: if y_vel>0, check y+3; on solid: snap y=(y+1)&0xfe, y_vel=0
 */
void ActorSystem::enemy_behavior_leap(enemy_t* enemy) {
    if (!enemy) return;

    // proposed_y tracks vertical changes before committing (mirrors ax.lo in assembly)
    uint8_t proposed_y = enemy->y;

    if (enemy->y_vel < 0) {
        // === .moving_up ===
        // Assembly: sar y_vel 3x (arithmetic) → y_vel/8, still negative (e.g. -7>>3 = -1)
        //           neg → positive upward delta; sub al, delta → proposed_y += delta (negative = up)
        // Simplified: proposed_y += (int8_t)(y_vel >> ENEMY_VELOCITY_SHIFT)
        // For y_vel=-7: delta=-1, proposed_y decreases by 1 (moves up 1 unit)
        int8_t delta = static_cast<int8_t>(enemy->y_vel >> ENEMY_VELOCITY_SHIFT);
        int16_t new_y = static_cast<int16_t>(proposed_y) + static_cast<int16_t>(delta);

        if (new_y < 0) {
            // Unsigned underflow → hit top of playfield (.undo_position_change)
            proposed_y = enemy->y;  // restore original (no change)
        } else {
            uint8_t target_y = static_cast<uint8_t>(new_y);
            if (!check_vertical_enemy_map_collision(enemy->x, target_y)) {
                proposed_y = target_y;  // accept upward movement
            }
            // else: ceiling collision → .undo_position_change (proposed_y stays = enemy->y)
        }
        // fall through to .apply_gravity

    } else if (enemy->y_vel > 0) {
        // === .moving_down ===
        // Assembly: sar y_vel 3x → y_vel/8; proposed_y += that; e.g. y_vel=8 → move down 1
        int8_t vel_over_8 = static_cast<int8_t>(enemy->y_vel >> ENEMY_VELOCITY_SHIFT);
        uint8_t new_y = static_cast<uint8_t>(proposed_y + vel_over_8);

        // Despawn at or below the bottom of the playfield
        if (new_y >= PLAYFIELD_HEIGHT - 2) {
            enemy->state = ENEMY_STATE_WHITE_SPARK + DEATH_ANIMATION_LAST_FRAME;
            enemy->y = PLAYFIELD_HEIGHT - 2;
            return;
        }

        // .keep_moving_down: look-ahead check at new_y+1
        // Assembly: inc al; check_vertical; dec al; jnc .apply_gravity (accept move)
        //            else: .start_falling → .undo_position_change (restore original pos)
        if (check_vertical_enemy_map_collision(enemy->x, static_cast<uint8_t>(new_y + 1))) {
            proposed_y = enemy->y;  // collision: restore original (.undo_position_change)
        } else {
            proposed_y = new_y;     // no collision: accept downward movement
        }
        // fall through to .apply_gravity

    } else {
        // === y_vel == 0 ===
        // Assembly: ax = [si+enemy.y]; al += 2; check_vertical;
        //           if solid → .begin_leap; else → .start_falling
        if (check_vertical_enemy_map_collision(enemy->x, static_cast<uint8_t>(enemy->y + 2))) {
            // .begin_leap: solid ground — initiate jump toward Comic
            // Assembly: cmp comic_x(ah), enemy.x(dh); jae (.unsigned >=) → x_vel=+1 else -1
            // jae means: if comic_x >= enemy.x → enemy is at/left-of Comic → move right (+1)
            enemy->x_vel = (g_comic_x >= enemy->x) ? 1 : -1;
            enemy->y_vel = ENEMY_JUMP_VELOCITY;  // -7
            // Assembly jumps directly to .done: stores position (unchanged), no gravity this tick
            return;
        }
        // .start_falling → .undo_position_change → .apply_gravity
        // proposed_y stays = enemy->y; y_vel stays = 0 (gravity below will set it to 2)
        // CRITICAL: do NOT return here — must fall through to gravity + horizontal + ground check
    }

    // === .apply_gravity ===
    // Runs for all paths except .begin_leap (which returned above).
    // Assembly: dl += 2; clamp to TERMINAL_VELOCITY; store y_vel
    {
        int16_t new_vel = static_cast<int16_t>(enemy->y_vel) + ENEMY_GRAVITY;  // +2
        enemy->y_vel = static_cast<int8_t>(new_vel > TERMINAL_VELOCITY ? TERMINAL_VELOCITY : new_vel);
    }

    // === Restraint — only gates horizontal movement; gravity and ground-check always run ===
    bool skip_horizontal = false;
    if (enemy->restraint == ENEMY_RESTRAINT_SKIP_THIS_TICK) {
        // .skip_this_tick: set restraint to MOVE, then fall to .check_for_ground (skip horizontal)
        enemy->restraint = ENEMY_RESTRAINT_MOVE_THIS_TICK;
        skip_horizontal = true;
    } else if (enemy->restraint == ENEMY_RESTRAINT_MOVE_THIS_TICK) {
        // Slow enemy: transition MOVE → SKIP
        enemy->restraint = ENEMY_RESTRAINT_SKIP_THIS_TICK;
    }
    // ENEMY_RESTRAINT_MOVE_EVERY_TICK: no state change

    // === Horizontal movement (skipped when restraint was SKIP_THIS_TICK) ===
    if (!skip_horizontal) {
        int16_t camera_rel_x;
        if (enemy->x_vel > 0) {
            // Moving right: check tile at x+2, advance x by 1, bounce at right playfield edge
            uint8_t next_x = static_cast<uint8_t>(enemy->x + 2);
            if (check_horizontal_enemy_map_collision(next_x, proposed_y)) {
                enemy->x_vel = -1;  // wall → bounce left
            } else {
                enemy->x = static_cast<uint8_t>(enemy->x + 1);
                camera_rel_x = static_cast<int16_t>(enemy->x) - static_cast<int16_t>(g_camera_x);
                if (camera_rel_x >= PLAYFIELD_WIDTH - 2) {
                    enemy->x_vel = -1;  // right playfield edge
                }
            }
        } else if (enemy->x_vel < 0) {
            // Moving left: check tile at x-1, advance x by -1, bounce at left playfield edge
            if (enemy->x == 0) {
                enemy->x_vel = 1;  // already at left edge
            } else {
                uint8_t next_x = static_cast<uint8_t>(enemy->x - 1);
                if (check_horizontal_enemy_map_collision(next_x, proposed_y)) {
                    enemy->x_vel = 1;  // wall → bounce right
                } else {
                    enemy->x = next_x;
                    camera_rel_x = static_cast<int16_t>(enemy->x) - static_cast<int16_t>(g_camera_x);
                    if (camera_rel_x <= 0) {
                        enemy->x_vel = 1;  // left playfield edge
                    }
                }
            }
        }
    }

    // === Commit vertical position (.done in assembly: "mov [si+enemy.y], dx") ===
    enemy->y = proposed_y;

    // === .check_for_ground: landing detection ===
    // Assembly: dx = ax; if y_vel <= 0: goto .done (still rising/hovering)
    //           al += 3; check_vertical; if solid: .landed → inc dl; and dl, 0xfe; y_vel=0
    if (enemy->y_vel > 0) {
        if (check_vertical_enemy_map_collision(enemy->x, static_cast<uint8_t>(enemy->y + 3))) {
            // .landed: snap to even tile boundary, stop vertical movement
            enemy->y = static_cast<uint8_t>((enemy->y + 1) & 0xFE);
            enemy->y_vel = 0;
        }
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
            enemy->state = ENEMY_STATE_WHITE_SPARK + DEATH_ANIMATION_LAST_FRAME;
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
        if (enemy->x == 0) {
            enemy->x_vel = 1;  // Hit left edge, reverse direction
        } else {
            next_x = static_cast<uint8_t>(enemy->x - 1);
            if (!check_horizontal_enemy_map_collision(next_x, enemy->y)) {
                enemy->x = next_x;
            }
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
    bool collision;

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
            if (enemy->x == 0) {
                enemy->x_vel = 1;  // Hit left edge, reverse direction
            } else {
                next_x = static_cast<uint8_t>(enemy->x - 1);
                collision = check_horizontal_enemy_map_collision(next_x, enemy->y);

                if (!collision) {
                    enemy->x = next_x;
                    enemy->x_vel = -1;
                } else {
                    enemy->x_vel = 1;  // Blocked, try right next time
                }
            }
        }

        enemy->facing = (enemy->x_vel < 0) ? ENEMY_FACING_LEFT : ENEMY_FACING_RIGHT;
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

    enemy->facing = (enemy->x_vel < 0) ? ENEMY_FACING_LEFT : ENEMY_FACING_RIGHT;
}

/**
 * SHY behavior: Flees when Comic is facing toward, approaches otherwise
 * Used by: Shy Bird, Spinner
 */
void ActorSystem::enemy_behavior_shy(enemy_t* enemy) {
    if (!enemy) return;

    uint8_t next_x, next_y;
    int8_t comic_facing_enemy;
    bool collision;
    int16_t camera_rel_x;

    // Handle restraint (movement throttle)
    if (enemy->restraint == ENEMY_RESTRAINT_SKIP_THIS_TICK) {
        enemy->restraint = ENEMY_RESTRAINT_MOVE_THIS_TICK;
        return;  // Skip this tick
    }

    if (enemy->restraint == ENEMY_RESTRAINT_MOVE_THIS_TICK) {
        enemy->restraint = ENEMY_RESTRAINT_SKIP_THIS_TICK;  // Skip next tick
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
        enemy->facing = ENEMY_FACING_RIGHT;
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
        enemy->facing = ENEMY_FACING_LEFT;
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
        // Comic is facing this enemy - always move up (flee upward unconditionally)
        enemy->y_vel = -1;
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
    } else if (enemy->y_vel < 0) {
        // Moving up
        if (enemy->y == 0) {
            enemy->y_vel = 1;  // Bounce down from top
        } else {
            next_y = static_cast<uint8_t>(enemy->y - 1);
            collision = check_vertical_enemy_map_collision(enemy->x, next_y);
            if (collision) {
                // Hit a solid tile above - bounce back down
                enemy->y_vel = 1;
            } else {
                enemy->y = next_y;
                // Hit the top of the playfield after moving - bounce back down
                if (enemy->y == 0) {
                    enemy->y_vel = 1;
                }
            }
        }
    }
}
