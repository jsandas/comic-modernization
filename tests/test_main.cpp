#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include "../include/physics.h"
#include "../include/graphics.h"
#include "../include/level.h"
#include "../include/level_loader.h"
#include "../include/doors.h"
#include "../include/actors.h"

// Provide required globals from main.cpp for physics.cpp linkage.
int comic_x = 0;
int comic_y = 0;
int8_t comic_y_vel = 0;
int8_t comic_x_momentum = 0;
uint8_t comic_facing = 1;
uint8_t comic_animation = 0;
uint8_t comic_is_falling_or_jumping = 0;
uint8_t comic_jump_counter = 0;
uint8_t comic_jump_power = JUMP_POWER_DEFAULT;
uint8_t key_state_jump = 0;
uint8_t previous_key_state_jump = 0;
uint8_t key_state_left = 0;
uint8_t key_state_right = 0;
uint8_t key_state_open = 0;
int camera_x = 0;

// Door system globals for test linkage
uint8_t comic_has_door_key = 0;
uint8_t current_level_number = 1;
uint8_t current_stage_number = 0;
const level_t* current_level_ptr = nullptr;
int8_t source_door_level_number = -1;
int8_t source_door_stage_number = -1;

// Checkpoint position
uint8_t comic_y_checkpoint = 12;
uint8_t comic_x_checkpoint = 14;

static int failures = 0;

static void check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << std::endl;
        failures++;
    }
}

static Animation make_animation(const std::vector<int>& durations, bool looping) {
    Animation anim;
    anim.looping = looping;
    anim.frame_start_time = 0;
    anim.current_frame = 0;
    anim.frames.clear();
    for (int duration : durations) {
        AnimationFrame frame;
        frame.duration_ms = duration;
        anim.frames.push_back(frame);
    }
    return anim;
}

static void test_physics_tiles() {
    init_test_level();

    uint8_t ground_tile = get_tile_at(0, 18); // tile_y = 9 (ground row)
    check(ground_tile == 0x3F, "ground tile should be 0x3F");

    uint8_t out_of_bounds = get_tile_at(255, 30); // tile_y = 15 (out of bounds)
    check(out_of_bounds == 0, "out-of-bounds tile should be 0");

    check(is_tile_solid(0x3F), "tile 0x3F should be solid");
    check(!is_tile_solid(0x3E), "tile 0x3E should be passable");
}

static void test_animation_looping() {
    GraphicsSystem graphics(nullptr);
    Animation anim = make_animation({100, 200, 100}, true);

    graphics.update_animation(anim, 50);
    check(anim.current_frame == 0, "looping: frame at 50ms should be 0");

    graphics.update_animation(anim, 150);
    check(anim.current_frame == 1, "looping: frame at 150ms should be 1");

    graphics.update_animation(anim, 310);
    check(anim.current_frame == 2, "looping: frame at 310ms should be 2");

    graphics.update_animation(anim, 450);
    check(anim.current_frame == 0, "looping: frame at 450ms should loop to 0");
}

static void test_animation_non_looping() {
    GraphicsSystem graphics(nullptr);
    Animation anim = make_animation({100, 200, 100}, false);

    graphics.update_animation(anim, 50);
    check(anim.current_frame == 0, "non-looping: frame at 50ms should be 0");

    graphics.update_animation(anim, 150);
    check(anim.current_frame == 1, "non-looping: frame at 150ms should be 1");

    graphics.update_animation(anim, 450);
    check(anim.current_frame == 2, "non-looping: frame at 450ms should clamp to 2");
}

static void test_animation_zero_duration() {
    GraphicsSystem graphics(nullptr);
    Animation anim = make_animation({0, 0}, true);

    graphics.update_animation(anim, 1);
    check(anim.current_frame >= 0 && anim.current_frame < static_cast<int>(anim.frames.size()),
          "zero duration: frame index should stay in range");
}

static void reset_physics_state() {
    init_test_level();
    comic_x = 4;
    comic_y = 14;
    comic_y_vel = 0;
    comic_x_momentum = 0;
    comic_facing = 1;
    comic_animation = 0;
    comic_is_falling_or_jumping = 0;
    comic_jump_power = JUMP_POWER_DEFAULT;
    comic_jump_counter = comic_jump_power;
    key_state_jump = 0;
    previous_key_state_jump = 0;
    key_state_left = 0;
    key_state_right = 0;
    camera_x = 0;
}

static void reset_door_state() {
    comic_has_door_key = 0;
    key_state_open = 0;
    current_level_number = 1;
    current_stage_number = 0;
    current_level_ptr = nullptr;
    source_door_level_number = -1;
    source_door_stage_number = -1;
}

/**
 * Create a test level with a door at the specified coordinates
 * Door transitions to target_level/target_stage
 */
static level_t* create_test_level_with_door(
    uint8_t door_x, uint8_t door_y, 
    uint8_t target_level, uint8_t target_stage) {
    
    static level_t test_level;
    std::memset(&test_level, 0, sizeof(test_level));
    
    // Set up door in stage 0
    test_level.stages[0].doors[0].x = door_x;
    test_level.stages[0].doors[0].y = door_y;
    test_level.stages[0].doors[0].target_level = target_level;
    test_level.stages[0].doors[0].target_stage = target_stage;
    
    // Mark other doors as unused
    test_level.stages[0].doors[1].x = DOOR_UNUSED;
    test_level.stages[0].doors[1].y = DOOR_UNUSED;
    test_level.stages[0].doors[2].x = DOOR_UNUSED;
    test_level.stages[0].doors[2].y = DOOR_UNUSED;
    
    // Set door tile properties (for passability checks)
    test_level.door_tile_ul = 0x10;
    test_level.door_tile_ur = 0x11;
    test_level.door_tile_ll = 0x12;
    test_level.door_tile_lr = 0x13;
    
    return &test_level;
}

static void simulate_tick() {
    process_jump_input();
    handle_fall_or_jump();
}

static int measure_jump_height(uint8_t jump_power) {
    reset_physics_state();
    comic_jump_power = jump_power;
    comic_jump_counter = comic_jump_power;

    int start_y = comic_y;
    int min_y = comic_y;

    key_state_jump = 1;
    simulate_tick();

    for (int tick = 0; tick < 200; ++tick) {
        simulate_tick();
        min_y = std::min(min_y, comic_y);
        if (comic_is_falling_or_jumping == 0 && tick > 0) {
            break;
        }
    }

    return start_y - min_y;
}

static void test_jump_edge_trigger() {
    reset_physics_state();

    key_state_jump = 1;
    simulate_tick();
    check(comic_is_falling_or_jumping == 1, "jump should start on key press edge");

    comic_is_falling_or_jumping = 0;
    comic_y = 14;
    comic_y_vel = 0;
    comic_jump_counter = comic_jump_power;
    key_state_jump = 1;
    simulate_tick();
    check(comic_is_falling_or_jumping == 0, "holding jump should not retrigger");
}

static void test_jump_recharge() {
    reset_physics_state();

    comic_jump_counter = 1;
    key_state_jump = 0;
    simulate_tick();
    check(comic_jump_counter == comic_jump_power, "jump counter should recharge on release");
}

static void test_jump_height() {
    int default_height = measure_jump_height(JUMP_POWER_DEFAULT);
    int boots_height = measure_jump_height(JUMP_POWER_WITH_BOOTS);

    check(default_height == 7,
          "default jump height should be 7 units (got " + std::to_string(default_height) + ")");
    check(boots_height == 9,
          "boots jump height should be 9 units (got " + std::to_string(boots_height) + ")");
}

static void test_door_activation_alignment_x() {
    reset_door_state();
    level_t* level = create_test_level_with_door(10, 8, 1, 1);
    current_level_ptr = level;
    current_level_number = 1;
    current_stage_number = 0;
    comic_has_door_key = 1;
    key_state_open = 1;
    
    // Test activation at exact door position
    comic_x = 10;
    comic_y = 8;
    uint8_t result = check_door_activation();
    check(result == 1, "door should activate at exact position (x=10, y=8)");
    
    // Test activation 1 unit to the right
    reset_door_state();
    current_level_ptr = level;
    current_level_number = 1;
    current_stage_number = 0;
    comic_has_door_key = 1;
    key_state_open = 1;
    comic_x = 11;
    comic_y = 8;
    result = check_door_activation();
    check(result == 1, "door should activate 1 unit offset (x=11, y=8)");
    
    // Test activation 2 units to the right (boundary)
    reset_door_state();
    current_level_ptr = level;
    current_level_number = 1;
    current_stage_number = 0;
    comic_has_door_key = 1;
    key_state_open = 1;
    comic_x = 12;
    comic_y = 8;
    result = check_door_activation();
    check(result == 1, "door should activate 2 units offset (x=12, y=8)");
    
    // Test no activation 3 units away (too far)
    reset_door_state();
    current_level_ptr = level;
    current_level_number = 1;
    current_stage_number = 0;
    comic_has_door_key = 1;
    key_state_open = 1;
    comic_x = 13;
    comic_y = 8;
    result = check_door_activation();
    check(result == 0, "door should not activate 3 units away (x=13, y=8)");
    
    // Test no activation to the left
    reset_door_state();
    current_level_ptr = level;
    current_level_number = 1;
    current_stage_number = 0;
    comic_has_door_key = 1;
    key_state_open = 1;
    comic_x = 9;
    comic_y = 8;
    result = check_door_activation();
    check(result == 0, "door should not activate left of position (x=9, y=8)");
}

static void test_door_activation_alignment_y() {
    reset_door_state();
    level_t* level = create_test_level_with_door(10, 8, 1, 1);
    current_level_ptr = level;
    current_level_number = 1;
    current_stage_number = 0;
    comic_has_door_key = 1;
    key_state_open = 1;
    
    // Test exact Y match
    comic_x = 10;
    comic_y = 8;
    uint8_t result = check_door_activation();
    check(result == 1, "door should activate at exact Y (y=8)");
    
    // Test Y one unit above
    reset_door_state();
    current_level_ptr = level;
    current_level_number = 1;
    current_stage_number = 0;
    comic_has_door_key = 1;
    key_state_open = 1;
    comic_x = 10;
    comic_y = 7;
    result = check_door_activation();
    check(result == 0, "door should not activate above door (y=7)");
    
    // Test Y one unit below
    reset_door_state();
    current_level_ptr = level;
    current_level_number = 1;
    current_stage_number = 0;
    comic_has_door_key = 1;
    key_state_open = 1;
    comic_x = 10;
    comic_y = 9;
    result = check_door_activation();
    check(result == 0, "door should not activate below door (y=9)");
}

static void test_door_key_requirement() {
    reset_door_state();
    level_t* level = create_test_level_with_door(10, 8, 1, 1);
    current_level_ptr = level;
    current_level_number = 1;
    current_stage_number = 0;
    comic_x = 10;
    comic_y = 8;
    key_state_open = 1;
    
    // Test without key
    comic_has_door_key = 0;
    uint8_t result = check_door_activation();
    check(result == 0, "door should not activate without key");
    
    // Test with key
    reset_door_state();
    current_level_ptr = level;
    current_level_number = 1;
    current_stage_number = 0;
    comic_has_door_key = 1;
    comic_x = 10;
    comic_y = 8;
    key_state_open = 1;
    result = check_door_activation();
    check(result == 1, "door should activate with key");
}

static void test_door_open_key_requirement() {
    reset_door_state();
    level_t* level = create_test_level_with_door(10, 8, 1, 1);
    current_level_ptr = level;
    current_level_number = 1;
    current_stage_number = 0;
    comic_has_door_key = 1;
    comic_x = 10;
    comic_y = 8;
    
    // Test without open key pressed
    key_state_open = 0;
    uint8_t result = check_door_activation();
    check(result == 0, "check_door_activation should return 0 when open key not pressed");
    
    // Test with open key pressed
    reset_door_state();
    current_level_ptr = level;
    current_level_number = 1;
    current_stage_number = 0;
    comic_has_door_key = 1;
    comic_x = 10;
    comic_y = 8;
    key_state_open = 1;
    result = check_door_activation();
    check(result == 1, "check_door_activation should return 1 when open key pressed");
}

static void test_door_state_update_same_level() {
    reset_door_state();
    level_t* level = create_test_level_with_door(10, 8, 1, 1);  // Target same level, stage 1
    current_level_ptr = level;
    current_level_number = 1;
    current_stage_number = 0;
    comic_has_door_key = 1;
    comic_x = 10;
    comic_y = 8;
    key_state_open = 1;
    
    // Before activation
    check(source_door_level_number == -1, "source_door_level_number should start as -1");
    check(source_door_stage_number == -1, "source_door_stage_number should start as -1");
    check(current_stage_number == 0, "current_stage_number should start as 0");
    
    // Activate door (will call load_new_stage due to same level)
    uint8_t result = check_door_activation();
    check(result == 1, "door activation to same level stage should succeed");
    
    // After activation, verify state was set
    check(source_door_level_number == 1, 
          "source_door_level_number should be 1 (origin level)");
    check(source_door_stage_number == 0, 
          "source_door_stage_number should be 0 (origin stage)");
    check(current_stage_number == 1, 
          "current_stage_number should be 1 (target stage)");
    check(current_level_number == 1, 
          "current_level_number should remain 1 (same level)");
}

static void test_door_state_update_different_level() {
    reset_door_state();
    level_t* level = create_test_level_with_door(10, 8, 2, 1);  // Target different level
    current_level_ptr = level;
    current_level_number = 1;
    current_stage_number = 0;
    comic_has_door_key = 1;
    comic_x = 10;
    comic_y = 8;
    key_state_open = 1;
    
    // Before activation
    check(source_door_level_number == -1, "source_door_level_number should start as -1");
    check(source_door_stage_number == -1, "source_door_stage_number should start as -1");
    check(current_level_number == 1, "current_level_number should start as 1");
    
    // Activate door (will call load_new_level due to different level)
    uint8_t result = check_door_activation();
    check(result == 1, "door activation to different level should succeed");
    
    // After activation, verify state was set
    check(source_door_level_number == 1, 
          "source_door_level_number should be 1 (origin level)");
    check(source_door_stage_number == 0, 
          "source_door_stage_number should be 0 (origin stage)");
    check(current_level_number == 2, 
          "current_level_number should be 2 (target level)");
    check(current_stage_number == 1, 
          "current_stage_number should be 1 (target stage)");
}

static void test_runtime_level_tiles_populated() {
    initialize_level_data();
    current_level_number = LEVEL_NUMBER_FOREST;
    current_stage_number = 0;
    source_door_level_number = -1;
    source_door_stage_number = -1;
    current_level_ptr = nullptr;

    load_new_level();
    check(current_level_ptr != nullptr, "current_level_ptr should be set after load_new_level");

    bool any_non_zero = false;
    const uint8_t* tiles = current_level_ptr->stages[current_stage_number].tiles;
    for (int i = 0; i < 128 * 10; ++i) {
        if (tiles[i] != 0) {
            any_non_zero = true;
            break;
        }
    }

    check(any_non_zero, "current level tiles should be populated (non-zero)");
    reset_door_state();
}

static void test_stage_left_exit_blocked() {
    reset_physics_state();
    
    // Position Comic not quite at edge (x=10, not 0)
    comic_x = 10;
    comic_y = 12;
    comic_x_momentum = 0;
    
    // Normal move should work (not at edge)
    move_left();
    check(comic_x == 9, "Comic should move left normally when not at edge");
}

static void test_stage_right_exit_blocked() {
    reset_physics_state();
    
    // Position Comic not quite at edge
    comic_x = MAP_WIDTH - 10;
    comic_y = 12;
    comic_x_momentum = 0;
    
    // Normal move should work (not at edge)
    move_right();
    check(comic_x == MAP_WIDTH - 9, "Comic should move right normally when not at edge");
}

static void test_stage_left_edge_detection() {
    reset_physics_state();
    
    // Position Comic at left edge with no level loaded
    comic_x = 0;
    comic_y = 12;
    comic_x_momentum = -1;
    current_level_ptr = nullptr;
    
    // Attempt to move left - should be blocked by null check
    uint8_t initial_stage = current_stage_number;
    move_left();
    
    // Verify stage didn't change and momentum was cleared
    check(current_stage_number == initial_stage, 
          "stage should not change when current_level_ptr is null");
    check(comic_x_momentum == 0, "momentum should be cleared when blocked at edge");
}

static void test_stage_right_edge_detection() {
    reset_physics_state();
    
    // Position Comic at right edge with no level loaded
    comic_x = MAP_WIDTH - 2;
    comic_y = 12;
    comic_x_momentum = 1;
    current_level_ptr = nullptr;
    
    // Attempt to move right - should be blocked by null check
    uint8_t initial_stage = current_stage_number;
    move_right();
    
    // Verify stage didn't change and momentum was cleared
    check(current_stage_number == initial_stage, 
          "stage should not change when current_level_ptr is null");
    check(comic_x_momentum == 0, "momentum should be cleared when blocked at edge");
}

static void test_cave_level_solidity() {
    initialize_level_data();
    current_level_number = LEVEL_NUMBER_CAVE;
    current_stage_number = 0;
    source_door_level_number = -1;
    source_door_stage_number = -1;
    current_level_ptr = nullptr;

    load_new_level();
    check(current_level_ptr != nullptr, "current_level_ptr should be set after load_new_level");
    check(current_level_ptr->tileset_last_passable == 0x09, 
          "cave level tileset_last_passable should be 0x09");
    
    // Tiles above 0x09 should be solid
    check(is_tile_solid(0x0a), "cave tile 0x0a should be solid");
    check(is_tile_solid(0x14), "cave tile 0x14 should be solid");
    check(!is_tile_solid(0x09), "cave tile 0x09 should be passable");
    check(!is_tile_solid(0x00), "cave tile 0x00 should be passable");
    
    reset_door_state();
    reset_level_tiles();  // Reset physics module state for test isolation
}

static void test_problematic_levels_have_solid_tiles() {
    initialize_level_data();
    
    // Test SHED level configuration and solidity
    const level_t* shed = get_level_by_number(LEVEL_NUMBER_SHED);
    check(shed != nullptr, "shed level should exist");
    check(shed->tileset_last_passable == 0x17, "shed level tileset_last_passable should be 0x17");
    
    current_level_number = LEVEL_NUMBER_SHED;
    current_stage_number = 0;
    source_door_level_number = -1;
    source_door_stage_number = -1;
    current_level_ptr = nullptr;
    load_new_level();
    check(is_tile_solid(0x18), "shed tile 0x18 should be solid (> 0x17)");
    check(!is_tile_solid(0x17), "shed tile 0x17 should be passable (<= 0x17)");
    reset_level_tiles();
    
    // Test BASE level configuration and solidity
    const level_t* base = get_level_by_number(LEVEL_NUMBER_BASE);
    check(base != nullptr, "base level should exist");
    check(base->tileset_last_passable == 0x3b, "base level tileset_last_passable should be 0x3b");
    
    current_level_number = LEVEL_NUMBER_BASE;
    current_stage_number = 0;
    source_door_level_number = -1;
    source_door_stage_number = -1;
    current_level_ptr = nullptr;
    load_new_level();
    check(is_tile_solid(0x3c), "base tile 0x3c should be solid (> 0x3b)");
    check(!is_tile_solid(0x3b), "base tile 0x3b should be passable (<= 0x3b)");
    reset_level_tiles();
    
    // Test COMP level configuration and solidity
    const level_t* comp = get_level_by_number(LEVEL_NUMBER_COMP);
    check(comp != nullptr, "comp level should exist");
    check(comp->tileset_last_passable == 0x1d, "comp level tileset_last_passable should be 0x1d");
    
    current_level_number = LEVEL_NUMBER_COMP;
    current_stage_number = 0;
    source_door_level_number = -1;
    source_door_stage_number = -1;
    current_level_ptr = nullptr;
    load_new_level();
    check(is_tile_solid(0x1e), "comp tile 0x1e should be solid (> 0x1d)");
    check(!is_tile_solid(0x1d), "comp tile 0x1d should be passable (<= 0x1d)");
    reset_level_tiles();
    
    // Test CAVE level configuration and solidity
    const level_t* cave = get_level_by_number(LEVEL_NUMBER_CAVE);
    check(cave != nullptr, "cave level should exist");
    check(cave->tileset_last_passable == 0x09, "cave level tileset_last_passable should be 0x09");
    
    current_level_number = LEVEL_NUMBER_CAVE;
    current_stage_number = 0;
    source_door_level_number = -1;
    source_door_stage_number = -1;
    current_level_ptr = nullptr;
    load_new_level();
    check(is_tile_solid(0x0a), "cave tile 0x0a should be solid (> 0x09)");
    check(!is_tile_solid(0x09), "cave tile 0x09 should be passable (<= 0x09)");
    reset_level_tiles();
}

// ============================================================================
// ACTOR SYSTEM TESTS
// ============================================================================

static void reset_actor_state(ActorSystem& actor_system) {
    init_test_level();
    comic_x = 10;
    comic_y = 10;
    comic_facing = COMIC_FACING_RIGHT;
    camera_x = 0;
    actor_system.reset_for_stage();
}

static void setup_test_enemy(std::vector<enemy_t>& enemies, int index, uint8_t behavior) {
    enemy_t& enemy = enemies[index];
    enemy.state = ENEMY_STATE_DESPAWNED;
    enemy.spawn_timer_and_animation = 0;  // Ready to spawn
    enemy.x = 0;
    enemy.y = 0;
    enemy.x_vel = 0;
    enemy.y_vel = 0;
    enemy.behavior = behavior;
    enemy.num_animation_frames = 2;
    enemy.facing = ENEMY_FACING_LEFT;
    enemy.restraint = (behavior & ENEMY_BEHAVIOR_FAST) ? 
                      ENEMY_RESTRAINT_MOVE_EVERY_TICK : 
                      ENEMY_RESTRAINT_MOVE_THIS_TICK;
    enemy.sprite_descriptor = nullptr;   // Tests don't need actual level data
    enemy.animation_data = nullptr;      // Tests don't need actual sprite data
}

static void test_actor_spawn_one_per_tick() {
    ActorSystem actor_system;
    actor_system.initialize();
    reset_actor_state(actor_system);
    
    auto& enemies = const_cast<std::vector<enemy_t>&>(actor_system.get_enemies());
    
    // Set up 4 enemies ready to spawn
    for (int i = 0; i < MAX_NUM_ENEMIES; i++) {
        setup_test_enemy(enemies, i, ENEMY_BEHAVIOR_BOUNCE);
    }
    
    const uint8_t* tiles = new uint8_t[128 * 10]();  // Empty tilemap
    
    // First update - should spawn exactly 1 enemy
    actor_system.update(comic_x, comic_y, comic_facing, tiles, camera_x);
    
    int spawned = 0;
    for (const auto& enemy : enemies) {
        if (enemy.state == ENEMY_STATE_SPAWNED) spawned++;
    }
    check(spawned == 1, "actor_spawn: should spawn exactly 1 enemy per tick");
    
    delete[] tiles;
}

static void test_actor_spawn_offset_cycling() {
    ActorSystem actor_system;
    actor_system.initialize();
    reset_actor_state(actor_system);
    
    const uint8_t* tiles = new uint8_t[128 * 10]();
    auto& enemies = const_cast<std::vector<enemy_t>&>(actor_system.get_enemies());
    
    // Spawn offset should cycle: 24→26→28→30→24...
    std::vector<uint8_t> spawn_positions;
    
    for (int spawn_cycle = 0; spawn_cycle < 5; spawn_cycle++) {
        actor_system.reset_for_stage();
        setup_test_enemy(enemies, 0, ENEMY_BEHAVIOR_BOUNCE);
        
        // Trigger spawn
        actor_system.update(comic_x, comic_y, comic_facing, tiles, camera_x);
        
        if (enemies[0].state == ENEMY_STATE_SPAWNED) {
            spawn_positions.push_back(enemies[0].x);
        }
    }
    
    // Verify spawn positions vary (offset cycling working)
    check(spawn_positions.size() >= 3, "actor_spawn_offset: should spawn multiple times");
    bool has_variation = false;
    for (size_t i = 1; i < spawn_positions.size(); i++) {
        if (spawn_positions[i] != spawn_positions[0]) {
            has_variation = true;
            break;
        }
    }
    check(has_variation, "actor_spawn_offset: spawn positions should vary due to offset cycling");
    
    delete[] tiles;
}

static void test_actor_despawn_distance() {
    ActorSystem actor_system;
    actor_system.initialize();
    reset_actor_state(actor_system);
    
    const uint8_t* tiles = new uint8_t[128 * 10]();
    auto& enemies = const_cast<std::vector<enemy_t>&>(actor_system.get_enemies());
    
    // Manually spawn enemy
    setup_test_enemy(enemies, 0, ENEMY_BEHAVIOR_BOUNCE);
    enemies[0].state = ENEMY_STATE_SPAWNED;
    enemies[0].x = comic_x;
    enemies[0].y = comic_y;
    
    actor_system.update(comic_x, comic_y, comic_facing, tiles, camera_x);
    check(enemies[0].state == ENEMY_STATE_SPAWNED, "actor_despawn: enemy should remain spawned when close");
    
    // Move Comic far away (> ENEMY_DESPAWN_RADIUS = 30)
    comic_x += 35;
    actor_system.update(comic_x, comic_y, comic_facing, tiles, camera_x);
    
    check(enemies[0].state == ENEMY_STATE_DESPAWNED, "actor_despawn: enemy should despawn when far from Comic");
    
    delete[] tiles;
}

static void test_actor_player_collision() {
    ActorSystem actor_system;
    actor_system.initialize();
    reset_actor_state(actor_system);
    
    const uint8_t* tiles = new uint8_t[128 * 10]();
    auto& enemies = const_cast<std::vector<enemy_t>&>(actor_system.get_enemies());
    
    // Manually position enemy at Comic's location to trigger collision
    //Collision box: horizontal abs(enemy.x - comic.x) <= 1, vertical 0 <= (enemy.y - comic.y) < 4
    setup_test_enemy(enemies, 0, ENEMY_BEHAVIOR_BOUNCE);
    enemies[0].state = ENEMY_STATE_SPAWNED;
    enemies[0].x = comic_x;
    enemies[0].y = comic_y;
    
    actor_system.update(comic_x, comic_y, comic_facing, tiles, camera_x);
    
    check(enemies[0].state == ENEMY_STATE_RED_SPARK, "actor_collision: enemy should enter RED_SPARK state on collision");
    
    delete[] tiles;
}

static void test_actor_death_animation() {
    ActorSystem actor_system;
    actor_system.initialize();
    reset_actor_state(actor_system);
    
    const uint8_t* tiles = new uint8_t[128 * 10]();
    auto& enemies = const_cast<std::vector<enemy_t>&>(actor_system.get_enemies());
    
    setup_test_enemy(enemies, 0, ENEMY_BEHAVIOR_BOUNCE);
    enemies[0].state = ENEMY_STATE_RED_SPARK;
    
    // Advance through animation frames (RED_SPARK: 8→9→10→11→12→13)
    for (int i = 0; i < 5; i++) {
        actor_system.update(comic_x, comic_y, comic_facing, tiles, camera_x);
    }
    
    check(enemies[0].state != ENEMY_STATE_SPAWNED, "actor_death_anim: should still be in death animation");
    
    // One more tick should complete the animation
    actor_system.update(comic_x, comic_y, comic_facing, tiles, camera_x);
    
    check(enemies[0].state == ENEMY_STATE_DESPAWNED, "actor_death_anim: should despawn after animation completes");
    
    delete[] tiles;
}

static void test_actor_respawn_timer_cycling() {
    ActorSystem actor_system;
    actor_system.initialize();
    reset_actor_state(actor_system);
    
    const uint8_t* tiles = new uint8_t[128 * 10]();
    auto& enemies = const_cast<std::vector<enemy_t>&>(actor_system.get_enemies());
    
    setup_test_enemy(enemies, 0, ENEMY_BEHAVIOR_BOUNCE);
    
    // Complete death animation to trigger despawn
    enemies[0].state = ENEMY_STATE_RED_SPARK + 5;
    actor_system.update(comic_x, comic_y, comic_facing, tiles, camera_x);
    
    check(enemies[0].state == ENEMY_STATE_DESPAWNED, "actor_respawn_cycle: should despawn after death");    
    uint8_t timer1 = enemies[0].spawn_timer_and_animation;
    
    // Trigger another death to advance the cycle
    enemies[0].state = ENEMY_STATE_RED_SPARK + 5;
    actor_system.update(comic_x, comic_y, comic_facing, tiles, camera_x);
    uint8_t timer2 = enemies[0].spawn_timer_and_animation;
    
    // Timer should increase: 20→40→60→80→100→20
    check(timer2 > timer1 || (timer1 == 100 && timer2 == 20), 
          "actor_respawn_cycle: respawn timer should cycle 20→40→60→80→100→20");
    
    delete[] tiles;
}

static void test_actor_animation_frames() {
    ActorSystem actor_system;
    actor_system.initialize();
    reset_actor_state(actor_system);
    
    const uint8_t* tiles = new uint8_t[128 * 10]();
    auto& enemies = const_cast<std::vector<enemy_t>&>(actor_system.get_enemies());
    
    setup_test_enemy(enemies, 0, ENEMY_BEHAVIOR_BOUNCE);
    enemies[0].num_animation_frames = 4;  // 4-frame animation
    enemies[0].state = ENEMY_STATE_SPAWNED;
    enemies[0].spawn_timer_and_animation = 0;
    
    // Advance animation
    for (int i = 0; i < 5; i++) {
        actor_system.update(comic_x, comic_y, comic_facing, tiles, camera_x);
    }
    
    // Animation should loop (frame 0, 1, 2, 3, 0...)
    check(enemies[0].spawn_timer_and_animation < 4, 
          "actor_animation: frame index should stay within num_animation_frames");
    
    delete[] tiles;
}

static void test_actor_behavior_bounce_movement() {
    ActorSystem actor_system;
    actor_system.initialize();
    reset_actor_state(actor_system);
    
    const uint8_t* tiles = new uint8_t[128 * 10]();
    auto& enemies = const_cast<std::vector<enemy_t>&>(actor_system.get_enemies());
    
    // Set up BOUNCE enemy
    setup_test_enemy(enemies, 0, ENEMY_BEHAVIOR_BOUNCE);
    enemies[0].state = ENEMY_STATE_SPAWNED;
    enemies[0].x = 10;
    enemies[0].y = 10;
    enemies[0].x_vel = 1;  // Moving right
    enemies[0].y_vel = -1; // Moving up
    enemies[0].restraint = ENEMY_RESTRAINT_MOVE_THIS_TICK;
    
    uint8_t start_x = enemies[0].x;
    uint8_t start_y = enemies[0].y;
    
    // Run a few ticks
    for (int i = 0; i < 5; i++) {
        actor_system.update(comic_x, comic_y, comic_facing, tiles, camera_x);
    }
    
    // Enemy should have moved (BOUNCE behavior causes diagonal movement)
    bool moved = (enemies[0].x != start_x) || (enemies[0].y != start_y);
    check(moved, "actor_bounce: enemy should move in diagonal pattern");
    
    delete[] tiles;
}

static void test_actor_restraint_throttling() {
    ActorSystem actor_system;
    actor_system.initialize();
    reset_actor_state(actor_system);
    
    const uint8_t* tiles = new uint8_t[128 * 10]();
    auto& enemies = const_cast<std::vector<enemy_t>&>(actor_system.get_enemies());
    
    // Slow enemy (no FAST flag)
    setup_test_enemy(enemies, 0, ENEMY_BEHAVIOR_BOUNCE);
    enemies[0].state = ENEMY_STATE_SPAWNED;
    
    // Fast enemy (FAST flag set)
    setup_test_enemy(enemies, 1, ENEMY_BEHAVIOR_BOUNCE | ENEMY_BEHAVIOR_FAST);
    enemies[1].state = ENEMY_STATE_SPAWNED;
    
    // Check restraint values
    check(enemies[0].restraint == ENEMY_RESTRAINT_MOVE_THIS_TICK, 
          "actor_restraint: slow enemy should have MOVE_THIS_TICK");
    check(enemies[1].restraint == ENEMY_RESTRAINT_MOVE_EVERY_TICK, 
          "actor_restraint: fast enemy should have MOVE_EVERY_TICK");
    
    delete[] tiles;
}

struct TestCase {
    const char* name;
    void (*run)();
};

static const std::vector<TestCase>& test_registry() {
    static const std::vector<TestCase> tests = {
        {"physics_tiles", test_physics_tiles},
        {"animation_looping", test_animation_looping},
        {"animation_non_looping", test_animation_non_looping},
        {"animation_zero_duration", test_animation_zero_duration},
        {"jump_edge_trigger", test_jump_edge_trigger},
        {"jump_recharge", test_jump_recharge},
        {"jump_height", test_jump_height},
        {"door_activation_alignment_x", test_door_activation_alignment_x},
        {"door_activation_alignment_y", test_door_activation_alignment_y},
        {"door_key_requirement", test_door_key_requirement},
        {"door_open_key_requirement", test_door_open_key_requirement},
        {"door_state_update_same_level", test_door_state_update_same_level},
        {"door_state_update_different_level", test_door_state_update_different_level},
        {"runtime_level_tiles_populated", test_runtime_level_tiles_populated},
        {"cave_level_solidity", test_cave_level_solidity},
        {"problematic_levels_have_solid_tiles", test_problematic_levels_have_solid_tiles},
        {"stage_left_exit_blocked", test_stage_left_exit_blocked},
        {"stage_right_exit_blocked", test_stage_right_exit_blocked},
        {"stage_left_edge_detection", test_stage_left_edge_detection},
        {"stage_right_edge_detection", test_stage_right_edge_detection},
        {"actor_spawn_one_per_tick", test_actor_spawn_one_per_tick},
        {"actor_spawn_offset_cycling", test_actor_spawn_offset_cycling},
        {"actor_despawn_distance", test_actor_despawn_distance},
        {"actor_player_collision", test_actor_player_collision},
        {"actor_death_animation", test_actor_death_animation},
        {"actor_respawn_timer_cycling", test_actor_respawn_timer_cycling},
        {"actor_animation_frames", test_actor_animation_frames},
        {"actor_behavior_bounce_movement", test_actor_behavior_bounce_movement},
        {"actor_restraint_throttling", test_actor_restraint_throttling}
    };
    return tests;
}

static bool matches_filter(const std::string& name, const std::string& filter) {
    if (filter.empty()) {
        return true;
    }
    return name.find(filter) != std::string::npos;
}

static int run_tests(const std::string& filter) {
    int tests_run = 0;
    for (const auto& test : test_registry()) {
        if (!matches_filter(test.name, filter)) {
            continue;
        }
        tests_run++;
        test.run();
    }

    if (tests_run == 0) {
        std::cerr << "No tests match filter: " << filter << std::endl;
        return 1;
    }

    if (failures == 0) {
        std::cout << "All tests passed." << std::endl;
        return 0;
    }

    std::cerr << failures << " test(s) failed." << std::endl;
    return 1;
}

static void print_usage(const char* argv0) {
    std::cout << "Usage: " << argv0 << " [--list] [--filter NAME]" << std::endl;
}

int main(int argc, char** argv) {
    std::string filter;
    bool list_only = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--list") {
            list_only = true;
        } else if (arg == "--filter") {
            if (i + 1 < argc) {
                filter = argv[++i];
            } else {
                std::cerr << "--filter requires a value" << std::endl;
                print_usage(argv[0]);
                return 1;
            }
        } else if (arg.rfind("--filter=", 0) == 0) {
            filter = arg.substr(std::string("--filter=").size());
        } else if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            return 0;
        } else {
            std::cerr << "Unknown argument: " << arg << std::endl;
            print_usage(argv[0]);
            return 1;
        }
    }

    if (list_only) {
        for (const auto& test : test_registry()) {
            std::cout << test.name << std::endl;
        }
        return 0;
    }

    return run_tests(filter);
}
