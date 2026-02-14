#include "../include/physics.h"
#include "../include/level_loader.h"
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <vector>

// External game state (defined in main.cpp)
extern int comic_x;
extern int comic_y;
extern int8_t comic_y_vel;
extern int8_t comic_x_momentum;
extern uint8_t comic_facing;
extern uint8_t comic_is_falling_or_jumping;
extern uint8_t comic_jump_counter;
extern uint8_t comic_jump_power;
extern uint8_t key_state_jump;
extern uint8_t previous_key_state_jump;
extern uint8_t key_state_left;
extern uint8_t key_state_right;
extern int camera_x;

// Level/stage tracking (defined in main.cpp)
extern uint8_t current_level_number;
extern uint8_t current_stage_number;
extern const level_t* current_level_ptr;
extern int8_t source_door_level_number;
extern int8_t source_door_stage_number;

// Checkpoint position (defined in main.cpp)
extern uint8_t comic_y_checkpoint;
extern uint8_t comic_x_checkpoint;

// Tile map data
static uint8_t current_tiles[MAP_WIDTH_TILES * MAP_HEIGHT_TILES];
static uint8_t tileset_last_passable = 0x3F; // Tiles > this are solid

// Ceiling stick flag
static bool ceiling_stick_flag = false;

void init_test_level() {
    // Initialize empty level
    std::memset(current_tiles, 0, sizeof(current_tiles));
    
    // Use tile ID 0x3F (last valid tile) for visible platforms
    // Valid tile range is 0x00-0x3F (64 tiles from tileset)
    // Update tileset_last_passable to mark 0x3F as solid for collision
    tileset_last_passable = 0x3E;
    
    // Create ground floor (row 9, bottom row)
    for (int x = 0; x < MAP_WIDTH_TILES; x++) {
        current_tiles[9 * MAP_WIDTH_TILES + x] = 0x3F; // Solid platform tile
    }
    
    // Add some walls for testing
    // Left wall
    for (int y = 5; y < 9; y++) {
        current_tiles[y * MAP_WIDTH_TILES + 10] = 0x3F;
    }
    
    // Right wall
    for (int y = 5; y < 9; y++) {
        current_tiles[y * MAP_WIDTH_TILES + 30] = 0x3F;
    }
    
    // Platform in the middle
    for (int x = 15; x < 25; x++) {
        current_tiles[7 * MAP_WIDTH_TILES + x] = 0x3F;
    }
}

uint8_t get_tile_at(uint8_t x, uint8_t y) {
    // Convert game units to tile coordinates (divide by 2)
    uint8_t tile_x = x / 2;
    uint8_t tile_y = y / 2;
    
    // Bounds check
    if (tile_x >= MAP_WIDTH_TILES || tile_y >= MAP_HEIGHT_TILES) {
        return 0; // Return passable tile if out of bounds
    }
    
    return current_tiles[tile_y * MAP_WIDTH_TILES + tile_x];
}

bool is_tile_solid(uint8_t tile_id) {
    return tile_id > tileset_last_passable;
}

void process_jump_input() {
    if (comic_is_falling_or_jumping == 0 &&
        key_state_jump && !previous_key_state_jump &&
        comic_jump_counter == comic_jump_power) {
        comic_is_falling_or_jumping = 1;
    }

    previous_key_state_jump = key_state_jump;
}

void handle_fall_or_jump() {
    if (comic_is_falling_or_jumping) {
        // STEP 1: Decrement jump counter
        if (comic_jump_counter > 0) {
            comic_jump_counter--;
        }
        
        // STEP 2: Check if counter expired, set to 1 as sentinel
        if (comic_jump_counter == 0) {
            comic_jump_counter = 1;
            ceiling_stick_flag = false;
        }
        // STEP 3: Apply upward acceleration if counter > 0 and jump key held
        else if (key_state_jump) {
            comic_y_vel -= JUMP_ACCELERATION;
        } else {
            ceiling_stick_flag = false;
        }
        
        // STEP 4: Integrate velocity (divide by 8)
        int delta_y = comic_y_vel >> 3;
        comic_y += delta_y;
        
        // Apply ceiling stick (push down 1 unit if against ceiling)
        if (ceiling_stick_flag) {
            comic_y++;
            ceiling_stick_flag = false;
        }
        
        // Bounds check: death if too far down
        if (comic_y >= PLAYFIELD_HEIGHT - 3) {
            // Reset position for now (would be death in full game)
            comic_y = 1;
            comic_y_vel = 0;
            comic_is_falling_or_jumping = 0;
        }
        
        // STEP 5: Apply gravity
        comic_y_vel += COMIC_GRAVITY;
        if (comic_y_vel > TERMINAL_VELOCITY) {
            comic_y_vel = TERMINAL_VELOCITY;
        }
        
        // STEP 6: Handle mid-air momentum (simplified for now)
        if (key_state_left) {
            comic_x_momentum--;
            if (comic_x_momentum < -5) {
                comic_x_momentum = -5;
            }
        }
        
        if (key_state_right) {
            comic_x_momentum++;
            if (comic_x_momentum > 5) {
                comic_x_momentum = 5;
            }
        }
        
        // Apply horizontal movement with drag
        if (comic_x_momentum < 0) {
            comic_x_momentum++; // Drag toward zero
            move_left();
        }
        
        if (comic_x_momentum > 0) {
            comic_x_momentum--; // Drag toward zero
            move_right();
        }
        
        // STEP 7: Check ceiling collision (upward)
        if (comic_y_vel < 0) {
            uint8_t head_tile = get_tile_at(comic_x, comic_y);
            bool head_solid = is_tile_solid(head_tile);
            
            // Also check tile to the right if between tiles
            if (!head_solid && (comic_x & 1)) {
                head_tile = get_tile_at(comic_x + 1, comic_y);
                head_solid = is_tile_solid(head_tile);
            }
            
            if (head_solid) {
                // Hit ceiling: stick and reset velocity
                ceiling_stick_flag = true;
                comic_y_vel = 0;
            }
        }
        
        // STEP 8: Check ground collision (downward)
        if (comic_y_vel > 0) {
            // Check 1 unit below Comic's feet (comic_y + 5)
            uint8_t foot_y = comic_y + 5;
            uint8_t foot_tile = get_tile_at(comic_x, foot_y);
            bool foot_solid = is_tile_solid(foot_tile);
            
            // Also check tile to the right if between tiles
            if (!foot_solid && (comic_x & 1)) {
                foot_tile = get_tile_at(comic_x + 1, foot_y);
                foot_solid = is_tile_solid(foot_tile);
            }
            
            if (foot_solid) {
                // Landing: align to tile boundary and stop falling
                uint8_t tile_row = foot_y / 2;
                comic_y = tile_row * 2 - 4;
                comic_is_falling_or_jumping = 0;
                comic_y_vel = 0;
                comic_x_momentum = 0;
                return;
            }
        }
    } else {
        // Recharge jump counter when on ground and not pressing jump
        // (jump initiation now happens in main loop with edge detection)
        if (!key_state_jump) {
            comic_jump_counter = comic_jump_power;
        }
        
        // Check if we should start falling (no ground beneath)
        uint8_t foot_y = comic_y + 5;
        uint8_t foot_tile = get_tile_at(comic_x, foot_y);
        bool foot_solid = is_tile_solid(foot_tile);
        
        if (!foot_solid && (comic_x & 1)) {
            foot_tile = get_tile_at(comic_x + 1, foot_y);
            foot_solid = is_tile_solid(foot_tile);
        }
        
        if (!foot_solid) {
            // No ground, start falling
            comic_is_falling_or_jumping = 1;
        }
    }
}

void move_left() {
    // Check if at left edge of stage
    if (comic_x == 0) {
        // Guard against NULL level pointer
        if (current_level_ptr == nullptr) {
            comic_x_momentum = 0;
            return;
        }
        
        // Validate stage number is within bounds (0-2)
        if (current_stage_number >= 3) {
            comic_x_momentum = 0;
            return;
        }
        
        const stage_t* stage = &current_level_ptr->stages[current_stage_number];
        
        // Check if there's a left exit
        if (stage->exit_l == 0xFF) {  // EXIT_UNUSED
            // No exit here, stop moving
            comic_x_momentum = 0;
            return;
        }
        
        // Stage transition to the left
        // TODO: play_sound(SOUND_STAGE_EDGE_TRANSITION, 4) when audio is implemented
        std::cout << "Stage transition: left to stage " << static_cast<int>(stage->exit_l) << std::endl;
        
        current_stage_number = stage->exit_l;
        comic_y_vel = 0;
        
        // Update checkpoint for spawn position on new stage
        comic_y_checkpoint = comic_y;
        comic_x_checkpoint = MAP_WIDTH - 2;  // Far right of new stage (254)
        
        // Position at far right edge of new stage
        comic_x = MAP_WIDTH - 2;
        
        // Mark as boundary transition (not door)
        source_door_level_number = -1;
        
        // Load the new stage
        load_new_stage();
        return;
    }
    
    int new_x = comic_x - 1;
    uint8_t check_y = comic_y + 3; // Check at knees
    
    // Check if we'd hit a wall
    uint8_t tile_id = get_tile_at(new_x, check_y);
    if (is_tile_solid(tile_id)) {
        comic_x_momentum = 0;
        return;
    }
    
    // Can move left
    comic_x = new_x;
    comic_facing = 0; // Left
    
    // Move camera left if appropriate
    int relative_x = comic_x - camera_x;
    if (camera_x > 0 && relative_x < (PLAYFIELD_WIDTH / 2 - 2)) {
        camera_x--;
    }
}

void move_right() {
    // Check if at right edge of stage
    if (comic_x >= MAP_WIDTH - 2) {
        // Guard against NULL level pointer
        if (current_level_ptr == nullptr) {
            comic_x_momentum = 0;
            return;
        }
        
        // Validate stage number is within bounds (0-2)
        if (current_stage_number >= 3) {
            comic_x_momentum = 0;
            return;
        }
        
        const stage_t* stage = &current_level_ptr->stages[current_stage_number];
        
        // Check if there's a right exit
        if (stage->exit_r == 0xFF) {  // EXIT_UNUSED
            // No exit here, stop moving
            comic_x_momentum = 0;
            return;
        }
        
        // Stage transition to the right
        // TODO: play_sound(SOUND_STAGE_EDGE_TRANSITION, 4) when audio is implemented
        std::cout << "Stage transition: right to stage " << static_cast<int>(stage->exit_r) << std::endl;
        
        current_stage_number = stage->exit_r;
        comic_y_vel = 0;
        
        // Update checkpoint for spawn position on new stage
        comic_y_checkpoint = comic_y;
        comic_x_checkpoint = 0;  // Far left of new stage
        
        // Position at far left edge of new stage
        comic_x = 0;
        
        // Mark as boundary transition (not door)
        source_door_level_number = -1;
        
        // Load the new stage
        load_new_stage();
        return;
    }
    
    int new_x = comic_x + 1;
    uint8_t check_y = comic_y + 3; // Check at knees
    uint8_t check_tile_x = new_x + 1; // Look at tile 1 unit to the right
    
    // Check if we'd hit a wall
    uint8_t tile_id = get_tile_at(check_tile_x, check_y);
    if (is_tile_solid(tile_id)) {
        comic_x_momentum = 0;
        return;
    }
    
    // Can move right
    comic_x = new_x;
    comic_facing = 1; // Right
    
    // Move camera right if appropriate
    int max_camera_x = MAP_WIDTH - PLAYFIELD_WIDTH;
    int relative_x = comic_x - camera_x;
    if (camera_x < max_camera_x && relative_x > (PLAYFIELD_WIDTH / 2)) {
        camera_x++;
    }
}
bool load_level_from_file(const std::string& level_name, int stage_number) {
    // Get the level data (which has been pre-loaded with tiles)
    level_t* level = get_level_data(level_name);
    if (!level) {
        std::cerr << "Failed to load level: " << level_name << std::endl;
        return false;
    }
    
    // Validate stage number
    if (stage_number < 0 || stage_number >= 3) {
        std::cerr << "Invalid stage number: " << stage_number << " (must be 0-2)" << std::endl;
        return false;
    }
    
    // Copy tile data from the stage structure to the current map
    const stage_t& stage = level->stages[stage_number];
    std::memcpy(current_tiles, stage.tiles, sizeof(current_tiles));
    
    // Set last_passable from the level's metadata
    // For now, use default value 0x3F (tiles > 0x3F are solid)
    tileset_last_passable = 0x3F;
    
    return true;
}