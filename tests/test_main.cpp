#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <fstream>
#include <filesystem>
#include "../include/physics.h"
#include "../include/graphics.h"
#include "../include/level.h"
#include "../include/level_loader.h"
#include "../include/doors.h"
#include "../include/actors.h"
#include "../include/audio.h"
#include "../include/title_sequence.h"
#include "../include/ui_system.h"
#include "../include/player_teleport.h"

#if defined(HAVE_SDL2_MIXER)
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

// Simple helper for tests: initialize SDL audio subsystem and set dummy driver.
static bool init_sdl_audio() {
    // Must be set before SDL audio init so CI/headless environments
    // reliably select the dummy backend.
    SDL_SetHint(SDL_HINT_AUDIODRIVER, "dummy");

    if ((SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO) == 0) {
        if (SDL_Init(SDL_INIT_AUDIO) < 0) {
            std::cerr << "SDL_Init AUDIO failed: " << SDL_GetError() << std::endl;
            return false;
        }
    }
    return true;
}

static void quit_sdl_audio() {
    if (SDL_WasInit(SDL_INIT_AUDIO)) {
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
    }
}

// Wait until the SFX channel (channel 0) is no longer playing or a timeout elapses.
// This avoids relying on fixed delays which can slow tests and flake on CI.
static void wait_for_sfx_channel_idle(uint32_t timeout_ms = 1000) {
    uint32_t start = SDL_GetTicks();
    while (Mix_Playing(0) && (SDL_GetTicks() - start) < timeout_ms) {
        SDL_Delay(5);
    }
}
#endif

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

// Testing hook from doors.cpp
extern bool g_skip_load_on_door;

// Checkpoint position
uint8_t comic_y_checkpoint = 12;
uint8_t comic_x_checkpoint = 14;

// Global system pointers (for cheat system compatibility)
ActorSystem* g_actor_system = nullptr;

// Game-over flag used by physics module (shared with main.cpp)
bool game_over_triggered = false;
uint8_t comic_num_lives = 0;

// Player shield/HP state used by actor collision logic
uint8_t comic_hp = MAX_HP;
uint8_t comic_hp_pending_increase = 0;

// Score bytes (used by award_points()) - base-100 encoding
uint8_t score_bytes[3] = {0, 0, 0};

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

static void test_enemy_animation_sequence() {
    std::vector<uint8_t> loop_sequence = build_enemy_animation_sequence(3, ENEMY_ANIMATION_LOOP);
    check(loop_sequence == std::vector<uint8_t>({0, 1, 2}),
        "enemy loop sequence should be 0,1,2 for 3 frames");

    std::vector<uint8_t> alternate_sequence = build_enemy_animation_sequence(3, ENEMY_ANIMATION_ALTERNATE);
    check(alternate_sequence == std::vector<uint8_t>({0, 1, 2, 1}),
        "enemy alternate sequence should be 0,1,2,1 for 3 frames");

    std::vector<uint8_t> alternate_sequence_four = build_enemy_animation_sequence(4, ENEMY_ANIMATION_ALTERNATE);
    check(alternate_sequence_four == std::vector<uint8_t>({0, 1, 2, 3, 2, 1}),
        "enemy alternate sequence should be 0,1,2,3,2,1 for 4 frames");

    std::vector<uint8_t> empty_sequence = build_enemy_animation_sequence(0, ENEMY_ANIMATION_LOOP);
    check(empty_sequence.empty(), "enemy sequence should be empty for 0 frames");
}

static void test_asset_path_resolution() {
    namespace fs = std::filesystem;
    // create a temporary directory tree mimicking the asset layout
    fs::path base = fs::temp_directory_path() / "comic_assets_test";
    fs::remove_all(base);
    fs::create_directories(base / "assets" / "sprites");
    fs::create_directories(base / "assets" / "sounds");
    fs::create_directories(base / "assets" / "maps");
    fs::create_directories(base / "assets" / "tiles");
    fs::create_directories(base / "assets" / "shp");
    fs::create_directories(base / "assets" / "graphics");

    // touch one file of each type
    std::ofstream((base / "assets" / "sprites" / "sprite-test.png").string()).close();
    std::ofstream((base / "assets" / "sounds" / "sound-test.wav").string()).close();
    std::ofstream((base / "assets" / "maps" / "foo.pt.png").string()).close();
    std::ofstream((base / "assets" / "tiles" / "foo.tt2-00.png").string()).close();
    std::ofstream((base / "assets" / "shp" / "foo.shp").string()).close();
    std::ofstream((base / "assets" / "graphics" / "sys000.ega").string()).close();

    // remember cwd so we can restore later
    fs::path orig_cwd = fs::current_path();
    fs::current_path(base);

    GraphicsSystem g(nullptr);
    std::string p;
    p = g.get_asset_path("sprite-test.png");
    check(p.find("assets/sprites/sprite-test.png") != std::string::npos,
          "get_asset_path should find sprite in sprites subdir");
    p = g.get_asset_path("sound-test.wav");
    check(p.find("assets/sounds/sound-test.wav") != std::string::npos,
          "get_asset_path should find sound in sounds subdir");
    p = g.get_asset_path("foo.pt.png");
    check(p.find("assets/maps/foo.pt.png") != std::string::npos,
          "get_asset_path should find map in maps subdir");
    p = g.get_asset_path("foo.tt2-00.png");
    check(p.find("assets/tiles/foo.tt2-00.png") != std::string::npos,
          "get_asset_path should find tileset in tiles subdir");
    p = g.get_asset_path("foo.shp");
    check(p.find("assets/shp/foo.shp") != std::string::npos,
          "get_asset_path should find shp in shp subdir");
    p = g.get_asset_path("sys000.ega");
    check(p.find("assets/graphics/sys000.ega") != std::string::npos,
          "get_asset_path should find raw graphics in graphics subdir");

    // cleanup
    fs::current_path(orig_cwd);
    fs::remove_all(base);
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
    // default testing behaviour: avoid loading when a door is activated
    g_skip_load_on_door = true;
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

static void test_space_level_uses_lower_gravity() {
    reset_physics_state();
    comic_x = 4;
    comic_y = 6;
    comic_is_falling_or_jumping = 1;
    comic_jump_counter = 1;
    comic_y_vel = 0;
    key_state_jump = 0;
    key_state_left = 0;
    key_state_right = 0;

    current_level_number = LEVEL_NUMBER_FOREST;
    handle_fall_or_jump();
    check(comic_y_vel == COMIC_GRAVITY,
        "normal levels should apply COMIC_GRAVITY");

    reset_physics_state();
    comic_x = 4;
    comic_y = 6;
    comic_is_falling_or_jumping = 1;
    comic_jump_counter = 1;
    comic_y_vel = 0;
    key_state_jump = 0;
    key_state_left = 0;
    key_state_right = 0;

    current_level_number = LEVEL_NUMBER_SPACE;
    handle_fall_or_jump();
    check(comic_y_vel == COMIC_GRAVITY_SPACE,
        "space level should apply COMIC_GRAVITY_SPACE");
}

static void test_jump_top_clamped_to_playfield() {
    reset_physics_state();
    comic_x = 4;
    comic_y = 0;
    comic_is_falling_or_jumping = 1;
    comic_jump_counter = 2;
    comic_y_vel = -16;
    key_state_jump = 1;
    key_state_left = 0;
    key_state_right = 0;
    current_level_number = LEVEL_NUMBER_FOREST;

    handle_fall_or_jump();

    check(comic_y == 0,
        "jump top clamp: player y should stay at top boundary (0)");
    check(!is_player_dying(),
        "jump top clamp: top clamp should not trigger death state");
}

static void advance_death_sequence_until_complete(int max_ticks = 128) {
    for (int i = 0; i < max_ticks && is_player_dying(); ++i) {
        update_player_death_sequence();
    }
}

static void test_player_death_sequence_respawn() {
    reset_physics_state();
    game_over_triggered = false;
    comic_num_lives = 3;
    comic_hp = 1;
    comic_x = 50;
    comic_y = 9;
    comic_x_momentum = 3;
    comic_y_vel = 4;
    comic_is_falling_or_jumping = 0;
    comic_jump_counter = 0;
    key_state_jump = 1;
    previous_key_state_jump = 1;
    comic_x_checkpoint = 22;
    comic_y_checkpoint = 12;

    trigger_player_death(true);
    check(is_player_dying(), "death: player should enter dying state after trigger");
    check(should_show_player_death_animation(),
        "death: animation phase should be active immediately after trigger");

    int animation_ticks = 0;
    while (is_player_dying() && should_show_player_death_animation() && animation_ticks < 64) {
        update_player_death_sequence();
        animation_ticks++;
    }

    check(animation_ticks > 0, "death: animation phase should consume at least one tick");
    check(is_player_dying() && !should_show_player_death_animation(),
        "death: sequence should transition into too-bad wait phase");

    advance_death_sequence_until_complete();

    check(!is_player_dying(), "death: sequence should complete for respawn path");
    check(comic_num_lives == 2, "death: lives should decrement by one on respawn path");
    check(!game_over_triggered, "death: game_over flag should remain false when lives remain");
    check(comic_x == comic_x_checkpoint && comic_y == comic_y_checkpoint,
        "death: respawn path should restore checkpoint position");
    check(comic_hp == MAX_HP, "death: respawn path should restore full HP");
    check(comic_is_falling_or_jumping == 1,
        "death: respawn path should place player into falling state");
    check(comic_jump_counter == comic_jump_power,
        "death: respawn path should reset jump counter to jump power");
    check(key_state_jump == 0 && previous_key_state_jump == 0,
        "death: respawn path should clear jump input edge state");
}

static void test_player_death_sequence_game_over() {
    reset_physics_state();
    game_over_triggered = false;
    comic_num_lives = 1;
    comic_hp = 2;
    comic_x = 41;
    comic_y = 8;
    comic_x_checkpoint = 20;
    comic_y_checkpoint = 12;

    trigger_player_death(false);
    check(is_player_dying(), "game_over: player should enter dying state");
    check(!should_show_player_death_animation(),
        "game_over: animation visibility should be off when trigger requested no animation");

    advance_death_sequence_until_complete();

    check(!is_player_dying(), "game_over: death sequence should finish");
    check(comic_num_lives == 0, "game_over: lives should decrement to zero");
    check(game_over_triggered, "game_over: zero lives should trigger game-over flag");
    check(comic_x == 41 && comic_y == 8,
        "game_over: zero-lives path should not respawn to checkpoint");
    check(comic_hp == 2, "game_over: zero-lives path should not reset HP");
}

static void test_teleport_does_not_update_respawn_checkpoint() {
    reset_physics_state();

    comic_x_checkpoint = 22;
    comic_y_checkpoint = 12;

    comic_x = 40;
    comic_y = 8;

    apply_teleport_destination_if_ready(
        3,
        60,
        6,
        comic_x,
        comic_y);

    check(comic_x == 60 && comic_y == 6,
        "teleport_checkpoint: teleport frame should move player to destination");
    check(comic_x_checkpoint == 22 && comic_y_checkpoint == 12,
        "teleport_checkpoint: teleport should not modify respawn checkpoint");
}

static void test_high_score_bytes_conversion() {
    // Zero score
    const uint8_t s0[3] = {0, 0, 0};
    check(score_bytes_to_uint32(s0) == 0u,
          "high_score: zero score should be 0");

    // Maximum score: bytes[2]=99, bytes[1]=99, bytes[0]=99 → 99*10000 + 99*100 + 99 = 999999
    const uint8_t s1[3] = {99, 99, 99};
    check(score_bytes_to_uint32(s1) == 999999u,
          "high_score: max score {99,99,99} should be 999999");

    // Only byte[0] set: 50
    const uint8_t s2[3] = {50, 0, 0};
    check(score_bytes_to_uint32(s2) == 50u,
          "high_score: bytes={50,0,0} should be 50");

    // Only byte[1] set: 50*100 = 5000
    const uint8_t s3[3] = {0, 50, 0};
    check(score_bytes_to_uint32(s3) == 5000u,
          "high_score: bytes={0,50,0} should be 5000");

    // Only byte[2] set: 10*10000 = 100000
    const uint8_t s4[3] = {0, 0, 10};
    check(score_bytes_to_uint32(s4) == 100000u,
          "high_score: bytes={0,0,10} should be 100000");

    // Mixed: 5*10000 + 10*100 + 20 = 51020
    const uint8_t s5[3] = {20, 10, 5};
    check(score_bytes_to_uint32(s5) == 51020u,
          "high_score: bytes={20,10,5} should be 51020");

    // Sample score with only middle byte populated.
    const uint8_t s6[3] = {0, 2, 0};
    check(score_bytes_to_uint32(s6) == 200u,
          "high_score: bytes={0,2,0} should be 200");
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

static void test_door_entry_sets_checkpoint_for_respawn() {
    initialize_level_data();

    // Simulate entering lake stage 0 from forest stage 2 via a door.
    // In level data, lake stage 0 has reciprocal door { y=10, x=118, target_level=1, target_stage=2 }.
    current_level_number = LEVEL_NUMBER_LAKE;
    current_stage_number = 0;
    source_door_level_number = LEVEL_NUMBER_FOREST;
    source_door_stage_number = 2;

    // Seed checkpoint with sentinel values so we can verify it changes.
    comic_x_checkpoint = 14;
    comic_y_checkpoint = 12;

    load_new_level();

    // Spawn point should be in front of reciprocal door.
    check(comic_x == 119 && comic_y == 10,
        "door_entry_checkpoint: player should spawn at reciprocal door position");

    // Door entry must set checkpoint so death respawns at this door.
    check(comic_x_checkpoint == 119 && comic_y_checkpoint == 10,
        "door_entry_checkpoint: checkpoint should update to reciprocal door spawn");

    // Source door markers should be consumed after stage load.
    check(source_door_level_number == -1 && source_door_stage_number == -1,
        "door_entry_checkpoint: source door markers should reset after stage load");
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
// SCORE / AWARD POINTS TESTS
// ============================================================================

static void reset_score_bytes() {
    score_bytes[0] = 0;
    score_bytes[1] = 0;
    score_bytes[2] = 0;
}

static void test_award_points_no_carry() {
    // award_points(3) adds 300 displayed points.
    reset_score_bytes();
    award_points(3);
    check(score_bytes[0] == 3, "award_points: 3 units should add 3 to score_bytes[0]");
    check(score_bytes[1] == 0, "award_points: no carry into score_bytes[1] expected");
    check(score_bytes[2] == 0, "award_points: no carry into score_bytes[2] expected");
}

static void test_award_points_accumulates_in_byte0() {
    // score_bytes[0] is the primary accumulation byte for base-100 units.
    reset_score_bytes();
    score_bytes[0] = 42;
    award_points(5);
    check(score_bytes[0] == 47, "award_points: 5 units should add to score_bytes[0]");
    check(score_bytes[1] == 0,  "award_points: no carry into score_bytes[1] expected");
    check(score_bytes[2] == 0,  "award_points: no carry into score_bytes[2] expected");
}

static void test_award_points_carry_into_byte1() {
    // score_bytes[0] = 90, award_points(20): sum = 110, carry = 1 into byte[1].
    // This corresponds to adding 2,000 displayed points.
    reset_score_bytes();
    score_bytes[0] = 90;
    award_points(20);
    check(score_bytes[0] == 10, "award_points: score_bytes[0] should wrap to 10");
    check(score_bytes[1] == 1,  "award_points: carry of 1 should propagate into score_bytes[1]");
    check(score_bytes[2] == 0,  "award_points: no carry into score_bytes[2] expected");
}

static void test_award_points_full_carry_amount() {
    // award_points(200) from zero: carry = 2 into score_bytes[1] (20,000 points).
    reset_score_bytes();
    award_points(200);
    check(score_bytes[0] == 0, "award_points: 200 mod 100 should leave score_bytes[0] = 0");
    check(score_bytes[1] == 2, "award_points: full carry of 2 should reach score_bytes[1]");
    check(score_bytes[2] == 0, "award_points: no carry into score_bytes[2] expected");
}

static void test_award_points_large_value_above_255() {
    // Values > 255 must not be truncated.
    // award_points(300): sum = 300, carry = 3, score_bytes[1] = 3 (30,000 points).
    reset_score_bytes();
    award_points(300);
    check(score_bytes[0] == 0, "award_points: 300 mod 100 should leave score_bytes[0] = 0");
    check(score_bytes[1] == 3, "award_points: carry of 3 should reach score_bytes[1] (no 8-bit truncation)");
    check(score_bytes[2] == 0, "award_points: no carry into score_bytes[2] expected");
}

static void test_award_points_max_score_saturation() {
    // Overflow beyond score_bytes[2] should saturate the entire score to 99:99:99.
    reset_score_bytes();
    score_bytes[0] = 50;
    score_bytes[1] = 99;
    score_bytes[2] = 99;
    award_points(60);  // would overflow top byte; saturate to max
    check(score_bytes[0] == 99, "award_points: score_bytes[0] should saturate at 99");
    check(score_bytes[1] == 99, "award_points: score_bytes[1] should saturate at 99");
    check(score_bytes[2] == 99, "award_points: score_bytes[2] should saturate at 99");
}

static void test_award_points_large_carry_saturation() {
    // Large carry against a full top byte must saturate to max score.
    reset_score_bytes();
    score_bytes[0] = 99;
    score_bytes[1] = 99;
    score_bytes[2] = 99;
    award_points(300);
    check(score_bytes[0] == 99, "award_points: score_bytes[0] should remain saturated at 99");
    check(score_bytes[1] == 99, "award_points: score_bytes[1] should remain saturated at 99");
    check(score_bytes[2] == 99, "award_points: score_bytes[2] should remain saturated at 99");
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
    enemies[0].y = static_cast<uint8_t>(comic_y - 2);
    enemies[0].restraint = ENEMY_RESTRAINT_SKIP_THIS_TICK;
    
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
    enemies[0].y = static_cast<uint8_t>(comic_y + 1);
    enemies[0].restraint = ENEMY_RESTRAINT_SKIP_THIS_TICK;
    
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

    comic_x = 0;
    comic_y = 0;
    
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

// ============================================================================
// FIREBALL SYSTEM TESTS
// ============================================================================

static void test_fireball_meter_depletion_timing() {
    ActorSystem actor_system;
    actor_system.initialize();

    std::vector<uint8_t> tiles(128 * 10, 0);
    actor_system.comic_firepower = 1;
    actor_system.fireball_meter = 3;

    comic_x = 10;
    comic_y = 10;
    comic_facing = COMIC_FACING_RIGHT;
    camera_x = 0;

    actor_system.update(comic_x, comic_y, comic_facing, tiles.data(), camera_x, 1);
    check(actor_system.fireball_meter == 2, "fireball_meter: should decrement on first firing tick");

    actor_system.update(comic_x, comic_y, comic_facing, tiles.data(), camera_x, 1);
    check(actor_system.fireball_meter == 2, "fireball_meter: should not decrement on second firing tick");

    actor_system.update(comic_x, comic_y, comic_facing, tiles.data(), camera_x, 1);
    check(actor_system.fireball_meter == 1, "fireball_meter: should decrement on third firing tick");
}

static void test_fireball_meter_recharge_timing() {
    ActorSystem actor_system;
    actor_system.initialize();

    std::vector<uint8_t> tiles(128 * 10, 0);
    comic_x = 10;
    comic_y = 10;
    comic_facing = COMIC_FACING_RIGHT;
    camera_x = 0;

    // Advance once so the counter reaches the recharge phase.
    actor_system.update(comic_x, comic_y, comic_facing, tiles.data(), camera_x, 0);
    actor_system.fireball_meter = 10;

    actor_system.update(comic_x, comic_y, comic_facing, tiles.data(), camera_x, 0);
    check(actor_system.fireball_meter == 11, "fireball_meter: should recharge every other tick when idle");
}

static void test_fireball_offscreen_deactivates() {
    ActorSystem actor_system;
    actor_system.initialize();

    std::vector<uint8_t> tiles(128 * 10, 0);
    actor_system.comic_firepower = 1;
    comic_x = 10;
    comic_y = 10;
    comic_facing = COMIC_FACING_LEFT;
    camera_x = 0;

    auto& fireballs = const_cast<std::vector<fireball_t>&>(actor_system.get_fireballs());
    fireballs[0].x = 1;
    fireballs[0].y = 5;
    fireballs[0].vel = -2;
    fireballs[0].corkscrew_phase = 2;
    fireballs[0].animation = 0;
    fireballs[0].num_animation_frames = FIREBALL_NUM_FRAMES;

    actor_system.update(comic_x, comic_y, comic_facing, tiles.data(), camera_x, 0);
    check(fireballs[0].x == FIREBALL_DEAD && fireballs[0].y == FIREBALL_DEAD,
          "fireball_offscreen: should deactivate when leaving camera bounds");
}

static void test_fireball_collision_sets_white_spark() {
    ActorSystem actor_system;
    actor_system.initialize();

    std::vector<uint8_t> tiles(128 * 10, 0);
    actor_system.comic_firepower = 1;
    comic_x = 10;
    comic_y = 0;
    comic_facing = COMIC_FACING_RIGHT;
    camera_x = 0;

    auto& fireballs = const_cast<std::vector<fireball_t>&>(actor_system.get_fireballs());
    fireballs[0].x = 10;
    fireballs[0].y = 5;
    fireballs[0].vel = 0;
    fireballs[0].corkscrew_phase = 2;
    fireballs[0].animation = 0;
    fireballs[0].num_animation_frames = FIREBALL_NUM_FRAMES;

    auto& enemies = const_cast<std::vector<enemy_t>&>(actor_system.get_enemies());
    enemies[0].state = ENEMY_STATE_SPAWNED;
    enemies[0].x = 10;
    enemies[0].y = 5;
    enemies[0].behavior = 0;
    enemies[0].num_animation_frames = 0;

    actor_system.update(comic_x, comic_y, comic_facing, tiles.data(), camera_x, 0);
    check(enemies[0].state == ENEMY_STATE_WHITE_SPARK,
          "fireball_collision: enemy should enter WHITE_SPARK on hit");
    check(fireballs[0].x == FIREBALL_DEAD && fireballs[0].y == FIREBALL_DEAD,
          "fireball_collision: fireball should deactivate on hit");
}

static void test_fireball_corkscrew_motion() {
    ActorSystem actor_system;
    actor_system.initialize();

    std::vector<uint8_t> tiles(128 * 10, 0);
    actor_system.comic_firepower = 1;
    actor_system.comic_has_corkscrew = 1;
    comic_x = 10;
    comic_y = 10;
    comic_facing = COMIC_FACING_RIGHT;
    camera_x = 0;

    auto& fireballs = const_cast<std::vector<fireball_t>&>(actor_system.get_fireballs());
    fireballs[0].x = 10;
    fireballs[0].y = 5;
    fireballs[0].vel = 0;
    fireballs[0].corkscrew_phase = 2;
    fireballs[0].animation = 0;
    fireballs[0].num_animation_frames = FIREBALL_NUM_FRAMES;

    actor_system.update(comic_x, comic_y, comic_facing, tiles.data(), camera_x, 0);
    check(fireballs[0].y == 6 && fireballs[0].corkscrew_phase == 1,
          "fireball_corkscrew: should move down and flip to phase 1");

    actor_system.update(comic_x, comic_y, comic_facing, tiles.data(), camera_x, 0);
    check(fireballs[0].y == 5 && fireballs[0].corkscrew_phase == 2,
          "fireball_corkscrew: should move up and flip to phase 2");
}

/* ============================================================================
 * ITEM SYSTEM TESTS
 * ============================================================================ */

/**
 * Test item collection tracking
 */
static void test_item_collection_tracking() {
    ActorSystem actor_system;
    actor_system.initialize();

    // Initially, no items collected
    check(actor_system.comic_firepower == 0, 
          "item_collection: firepower should start at 0");
    check(actor_system.comic_has_boots == 0, 
          "item_collection: should not have boots initially");

    // Simulate collecting Blastola Cola
    actor_system.apply_item_effect(ITEM_BLASTOLA_COLA);
    check(actor_system.comic_firepower == 1, 
          "item_collection: Blastola Cola should increase firepower to 1");

    // Collect second Blastola Cola
    actor_system.apply_item_effect(ITEM_BLASTOLA_COLA);
    check(actor_system.comic_firepower == 2, 
          "item_collection: second Blastola Cola should increase firepower to 2");

    // Collect Boots
    actor_system.apply_item_effect(ITEM_BOOTS);
    check(actor_system.comic_has_boots == 1, 
          "item_collection: should have boots after collection");
}

// New test to ensure door key state syncs globally
static void test_actor_door_key_sync() {
    ActorSystem actor_system;
    actor_system.initialize();

    // Reset global state
    comic_has_door_key = 0;

    actor_system.apply_item_effect(ITEM_DOOR_KEY);
    check(actor_system.comic_has_door_key == 1, "actor system should have door key flag set");
    check(comic_has_door_key == 1, "global door key variable should be synced by ActorSystem");
}

/**
 * Test Blastola Cola firepower increase
 */
static void test_item_blastola_cola_firepower() {
    ActorSystem actor_system;
    actor_system.initialize();

    // Initially no firepower
    check(actor_system.comic_firepower == 0, 
          "blastola_cola: firepower should start at 0");

    // Collect 5 Blastola Colas (max firepower)
    for (int i = 0; i < 5; i++) {
        actor_system.apply_item_effect(ITEM_BLASTOLA_COLA);
    }
    check(actor_system.comic_firepower == 5, 
          "blastola_cola: should reach max firepower of 5");

    // Try to collect a 6th (should not exceed max)
    actor_system.apply_item_effect(ITEM_BLASTOLA_COLA);
    check(actor_system.comic_firepower == 5, 
          "blastola_cola: firepower should not exceed 5");
}

/**
 * Test Boots item increases jump power
 */
static void test_item_boots_jump_power() {
    ActorSystem actor_system;
    actor_system.initialize();

    // Initially no boots
    int default_jump_power = actor_system.get_jump_power();
    check(default_jump_power == JUMP_POWER_DEFAULT, 
          "boots_jump: should have default jump power initially");

    // Collect Boots
    actor_system.apply_item_effect(ITEM_BOOTS);
    int boots_jump_power = actor_system.get_jump_power();
    check(boots_jump_power == JUMP_POWER_WITH_BOOTS, 
          "boots_jump: should have increased jump power with boots");
    check(boots_jump_power > default_jump_power, 
          "boots_jump: boots jump power should be greater than default");
}

/**
 * Test Corkscrew item flag
 */
static void test_item_corkscrew_flag() {
    ActorSystem actor_system;
    actor_system.initialize();

    check(actor_system.comic_has_corkscrew == 0, 
          "corkscrew: should not have corkscrew initially");

    actor_system.apply_item_effect(ITEM_CORKSCREW);
    check(actor_system.comic_has_corkscrew == 1, 
          "corkscrew: should have corkscrew after collection");
}

/**
 * Test treasure collection and counting
 */
static void test_item_treasure_counting() {
    ActorSystem actor_system;
    actor_system.initialize();

    // Initially no treasures
    check(actor_system.comic_num_treasures == 0, 
          "treasures: treasure count should start at 0");
    check(actor_system.comic_has_gems == 0, 
          "treasures: should not have gems initially");
    check(actor_system.comic_has_crown == 0, 
          "treasures: should not have crown initially");
    check(actor_system.comic_has_gold == 0, 
          "treasures: should not have gold initially");

    // Collect Gems
    actor_system.apply_item_effect(ITEM_GEMS);
    check(actor_system.comic_has_gems == 1, 
          "treasures: should have gems after collection");
    check(actor_system.comic_num_treasures == 1, 
          "treasures: treasure count should be 1 after gems");

    // Collect Crown
    actor_system.apply_item_effect(ITEM_CROWN);
    check(actor_system.comic_has_crown == 1, 
          "treasures: should have crown after collection");
    check(actor_system.comic_num_treasures == 2, 
          "treasures: treasure count should be 2 after crown");

    // Collect Gold
    actor_system.apply_item_effect(ITEM_GOLD);
    check(actor_system.comic_has_gold == 1, 
          "treasures: should have gold after collection");
    check(actor_system.comic_num_treasures == 3, 
          "treasures: treasure count should be 3 after gold");

    // Collecting same treasure again should not increase count
    actor_system.apply_item_effect(ITEM_GEMS);
    check(actor_system.comic_num_treasures == 3, 
          "treasures: duplicate collection should not increase count");
}

/**
 * Test Door Key and Teleport Wand flags
 */
static void test_item_special_items() {
    ActorSystem actor_system;
    actor_system.initialize();

    // Door Key
    check(actor_system.comic_has_door_key == 0, 
          "special_items: should not have door key initially");
    actor_system.apply_item_effect(ITEM_DOOR_KEY);
    check(actor_system.comic_has_door_key == 1, 
          "special_items: should have door key after collection");

    // Teleport Wand
    check(actor_system.comic_has_teleport_wand == 0, 
          "special_items: should not have teleport wand initially");
    actor_system.apply_item_effect(ITEM_TELEPORT_WAND);
    check(actor_system.comic_has_teleport_wand == 1, 
          "special_items: should have teleport wand after collection");

    // Lantern
    check(actor_system.comic_has_lantern == 0, 
          "special_items: should not have lantern initially");
    actor_system.apply_item_effect(ITEM_LANTERN);
    check(actor_system.comic_has_lantern == 1, 
          "special_items: should have lantern after collection");

        // Shield (not full HP): schedule refill to MAX_HP
        comic_num_lives = 2;
        comic_hp = static_cast<uint8_t>(MAX_HP - 2);
        comic_hp_pending_increase = 0;
        actor_system.apply_item_effect(ITEM_SHIELD);
        check(comic_hp_pending_increase == 2,
            "special_items: shield should schedule refill when HP is not full");
        check(comic_num_lives == 2,
            "special_items: shield should not award extra life when HP is not full");

        // Shield (full HP, below cap): award extra life
        comic_hp = MAX_HP;
        comic_hp_pending_increase = 0;
        actor_system.apply_item_effect(ITEM_SHIELD);
        check(comic_num_lives == 3,
            "special_items: shield should award extra life when HP is full");
        check(comic_hp_pending_increase == 0,
            "special_items: full-HP shield below max lives should not schedule HP refill");

        // Shield (full HP, at cap): keep max lives and award comic-c bonus path.
        comic_num_lives = 5;
        comic_hp = MAX_HP;
        comic_hp_pending_increase = 0;
        score_bytes[0] = 0;
        score_bytes[1] = 0;
        score_bytes[2] = 0;
        actor_system.apply_item_effect(ITEM_SHIELD);
        check(comic_num_lives == 5,
            "special_items: shield at max lives should not increase life count");
        check(comic_hp_pending_increase == MAX_HP,
            "special_items: shield at max lives should set pending HP refill");
        check(score_bytes[0] == 25 && score_bytes[1] == 2 && score_bytes[2] == 0,
            "special_items: shield at max lives should award 22500 points");
}

// ===== Audio System Tests =====
// These tests verify audio initialization, shutdown, and priority-based playback

#if defined(HAVE_SDL2_MIXER)

static void test_audio_init_shutdown_idempotency() {
    // Initialize SDL audio and set dummy driver
    check(init_sdl_audio(), "audio_idempotency: SDL audio init should succeed");

    // Multiple initializations should succeed
    check(initialize_audio_system(), 
          "audio_idempotency: first initialization should succeed");
    check(is_audio_system_ready(), 
          "audio_idempotency: system should be ready after init");
    
    // Second init should be safe (idempotent)
    check(initialize_audio_system(), 
          "audio_idempotency: second initialization should succeed");
    check(is_audio_system_ready(), 
          "audio_idempotency: system should still be ready");
    
    // Multiple shutdowns should be safe
    shutdown_audio_system();
    check(!is_audio_system_ready(), 
          "audio_idempotency: system should not be ready after shutdown");
    
    shutdown_audio_system();  // Should not crash
    check(!is_audio_system_ready(), 
          "audio_idempotency: system should remain not ready after second shutdown");

    quit_sdl_audio();
}

static void test_audio_graceful_failure_when_not_initialized() {
    // Initialize SDL audio to satisfy SDL_mixer requirement
    check(init_sdl_audio(), "audio_uninitialized: SDL audio init should succeed");

    // Ensure audio is not initialized by our subsystem
    shutdown_audio_system();
    
    check(!is_audio_system_ready(), 
          "audio_uninitialized: system should not be ready");
    
    // Attempting to play sound should fail gracefully (return false, not crash)
    check(!play_game_sound(GameSound::FIRE), 
          "audio_uninitialized: play should fail when not initialized");
    check(!play_game_sound(GameSound::ENEMY_HIT), 
          "audio_uninitialized: enemy hit sound should fail when not initialized");

    quit_sdl_audio();
}

static void test_audio_priority_interrupt() {
    check(init_sdl_audio(), "audio_priority_interrupt: SDL audio init should succeed");
    
    check(initialize_audio_system(), 
          "audio_priority_interrupt: initialization should succeed");
    
    // Play lower priority sound (STAGE_TRANSITION = priority 3)
    check(play_game_sound(GameSound::STAGE_TRANSITION), 
          "audio_priority_interrupt: lower priority sound should play");
    
    // Higher priority sound (PLAYER_HIT = priority 8) should interrupt
    check(play_game_sound(GameSound::PLAYER_HIT), 
          "audio_priority_interrupt: higher priority sound should interrupt");
    
    // Even higher priority (PLAYER_DIE = priority 9) should interrupt
    check(play_game_sound(GameSound::PLAYER_DIE), 
          "audio_priority_interrupt: even higher priority should interrupt");
    
    shutdown_audio_system();
    quit_sdl_audio();
}

static void test_audio_priority_blocking() {
    check(init_sdl_audio(), "audio_priority_blocking: SDL audio init should succeed");
    
    check(initialize_audio_system(), 
          "audio_priority_blocking: initialization should succeed");
    
    // Play high priority sound (PLAYER_DIE = priority 9)
    check(play_game_sound(GameSound::PLAYER_DIE), 
          "audio_priority_blocking: high priority sound should play");
    
    // Lower priority sound (ENEMY_HIT = priority 4) should be blocked
    // while PLAYER_DIE is still playing.  Assert it returns false.
    bool blocked = !play_game_sound(GameSound::ENEMY_HIT);
    check(blocked, "audio_priority_blocking: lower priority sound should be blocked by active higher priority");
    
    // Wait for the high-priority sound to finish before trying again
    wait_for_sfx_channel_idle(1000);
    
    // After the channel is idle, lower priority sound should be able to play
    check(play_game_sound(GameSound::ENEMY_HIT), 
          "audio_priority_blocking: sound should play after previous completes");
    
    shutdown_audio_system();
    quit_sdl_audio();
}

static void test_audio_all_sounds_playable() {
    check(init_sdl_audio(), "audio_all_sounds: SDL audio init should succeed");
    
    check(initialize_audio_system(), 
          "audio_all_sounds: initialization should succeed");

    // Play each actual sound in non-decreasing priority order so that
    // none are accidentally blocked by a higher-priority sound still
    // playing.  Assert the return value so missing or malformed
    // definitions cause a failure.
    bool ok;

    ok = play_game_sound(GameSound::GAME_OVER);
    check(ok, "audio_all_sounds: GAME_OVER should play");
    wait_for_sfx_channel_idle(200);

    ok = play_game_sound(GameSound::STAGE_TRANSITION);
    check(ok, "audio_all_sounds: STAGE_TRANSITION should play");
    wait_for_sfx_channel_idle(200);

    ok = play_game_sound(GameSound::ENEMY_HIT);
    check(ok, "audio_all_sounds: ENEMY_HIT should play");
    wait_for_sfx_channel_idle(200);

    ok = play_game_sound(GameSound::FIRE);
    check(ok, "audio_all_sounds: FIRE should play");
    wait_for_sfx_channel_idle(200);

    ok = play_game_sound(GameSound::DOOR_OPEN);
    check(ok, "audio_all_sounds: DOOR_OPEN should play");
    wait_for_sfx_channel_idle(200);

    ok = play_game_sound(GameSound::ITEM_COLLECT);
    check(ok, "audio_all_sounds: ITEM_COLLECT should play");
    wait_for_sfx_channel_idle(200);

    ok = play_game_sound(GameSound::EXTRA_LIFE);
    check(ok, "audio_all_sounds: EXTRA_LIFE should play");
    wait_for_sfx_channel_idle(200);

    ok = play_game_sound(GameSound::TELEPORT);
    check(ok, "audio_all_sounds: TELEPORT should play");
    wait_for_sfx_channel_idle(200);

    ok = play_game_sound(GameSound::PLAYER_HIT);
    check(ok, "audio_all_sounds: PLAYER_HIT should play");
    wait_for_sfx_channel_idle(200);

    ok = play_game_sound(GameSound::PLAYER_DIE);
    check(ok, "audio_all_sounds: PLAYER_DIE should play");
    wait_for_sfx_channel_idle(200);

    // UNUSED_0 has no sequence (jump sound is omitted in original game)
    check(!play_game_sound(GameSound::UNUSED_0), 
          "audio_all_sounds: UNUSED_0 should not play (no jump sound)");

    shutdown_audio_system();
    quit_sdl_audio();
}

static void test_audio_music_playback() {
    check(init_sdl_audio(), "audio_music: SDL audio init should succeed");
    check(initialize_audio_system(), "audio_music: initialization should succeed");

    bool ok = play_game_music(GameMusic::TITLE);
    check(ok, "audio_music: title music should start");
    check(is_game_music_playing(), "audio_music: music should report playing");

    // Ensure SFX can play concurrently
    ok = play_game_sound(GameSound::FIRE);
    check(ok, "audio_music: SFX should play while music active");
    check(Mix_Playing(0) != 0, "audio_music: sfx channel should be active");

    stop_game_music();
    check(!is_game_music_playing(), "audio_music: music should stop");

    wait_for_sfx_channel_idle(500);
    ok = play_game_sound(GameSound::ITEM_COLLECT);
    check(ok, "audio_music: sfx should still play after music stopped");

    shutdown_audio_system();
    quit_sdl_audio();
}

#else

// Stub tests when SDL2_mixer is not available
static void test_audio_init_shutdown_idempotency() {
    check(!initialize_audio_system(), 
          "audio_no_mixer: init should fail without SDL2_mixer");
    check(!is_audio_system_ready(), 
          "audio_no_mixer: system should not be ready without SDL2_mixer");
}

static void test_audio_graceful_failure_when_not_initialized() {
    check(!play_game_sound(GameSound::FIRE), 
          "audio_no_mixer: play should fail without SDL2_mixer");
}

static void test_audio_priority_interrupt() {
    // No-op when SDL2_mixer not available
}

static void test_audio_priority_blocking() {
    // No-op when SDL2_mixer not available
}

static void test_audio_all_sounds_playable() {
    // No-op when SDL2_mixer not available
}

#endif

// ============================================================================
// UI SYSTEM TESTS
// ============================================================================

/**
 * Test score byte to digit conversion (base-100 to decimal)
 */
static void test_ui_score_base100_encoding() {
    uint8_t digits[6];
    
    // Test zero score
    uint8_t score_zero[3] = {0, 0, 0};
    UISystem::score_bytes_to_digits(score_zero, digits);
    check(digits[0] == 0 && digits[1] == 0 && digits[2] == 0 &&
          digits[3] == 0 && digits[4] == 0 && digits[5] == 0,
          "ui_score: zero should convert to all 0 digits");
    
    // Test score 99 (99, 0, 0)
    uint8_t score_99[3] = {99, 0, 0};
    UISystem::score_bytes_to_digits(score_99, digits);
    check(digits[4] == 9 && digits[5] == 9,
          "ui_score: score 99 should have rightmost digits as 9,9");
    
    // Test score 123,456 = (56, 34, 12)
    uint8_t score_123456[3] = {56, 34, 12};
    UISystem::score_bytes_to_digits(score_123456, digits);
    check(digits[0] == 1 && digits[1] == 2 && digits[2] == 3 &&
          digits[3] == 4 && digits[4] == 5 && digits[5] == 6,
          "ui_score: 123456 should convert to digits 1,2,3,4,5,6");
    
    // Test max score 999,999 = (99, 99, 99)
    uint8_t score_max[3] = {99, 99, 99};
    UISystem::score_bytes_to_digits(score_max, digits);
    check(digits[0] == 9 && digits[1] == 9 && digits[2] == 9 &&
          digits[3] == 9 && digits[4] == 9 && digits[5] == 9,
          "ui_score: max score should convert to all 9 digits");
}

struct TestCase {
    const char* name;
    void (*run)();
};

/**
 * Test fireball meter to cell state mapping
 */
static void test_ui_fireball_meter_cell_mapping() {
    // Test empty meter (0)
    check(UISystem::fireball_meter_to_cell_state(0, 0) == 0,
          "ui_fireball: meter 0, cell 0 should be empty");
    check(UISystem::fireball_meter_to_cell_state(0, 5) == 0,
          "ui_fireball: meter 0, all cells should be empty");
    
    // Test meter value 1 (first cell half-filled)
    check(UISystem::fireball_meter_to_cell_state(1, 0) == 1,
          "ui_fireball: meter 1, cell 0 should be half");
    check(UISystem::fireball_meter_to_cell_state(1, 1) == 0,
          "ui_fireball: meter 1, cell 1 should be empty");
    
    // Test meter value 2 (first cell full)
    check(UISystem::fireball_meter_to_cell_state(2, 0) == 2,
          "ui_fireball: meter 2, cell 0 should be full");
    check(UISystem::fireball_meter_to_cell_state(2, 1) == 0,
          "ui_fireball: meter 2, cell 1 should be empty");
    
    // Test meter value 7 (cells 0-2 full, cell 3 half, cells 4-5 empty)
    check(UISystem::fireball_meter_to_cell_state(7, 0) == 2,
          "ui_fireball: meter 7, cell 0 should be full");
    check(UISystem::fireball_meter_to_cell_state(7, 1) == 2,
          "ui_fireball: meter 7, cell 1 should be full");
    check(UISystem::fireball_meter_to_cell_state(7, 2) == 2,
          "ui_fireball: meter 7, cell 2 should be full");
    check(UISystem::fireball_meter_to_cell_state(7, 3) == 1,
          "ui_fireball: meter 7, cell 3 should be half");
    check(UISystem::fireball_meter_to_cell_state(7, 4) == 0,
          "ui_fireball: meter 7, cell 4 should be empty");
    
    // Test max meter value (12 - all cells full)
    for (uint8_t cell = 0; cell < 6; cell++) {
        check(UISystem::fireball_meter_to_cell_state(12, cell) == 2,
              "ui_fireball: meter 12, all cells should be full");
    }
    
    // Test invalid cell index
    check(UISystem::fireball_meter_to_cell_state(12, 6) == 0,
          "ui_fireball: invalid cell index should return 0");
}

/**
 * Test boots detection from jump power
 */
static void test_ui_boots_detection() {
    // JUMP_POWER_DEFAULT is 4, WITH_BOOTS is 5
    check(!UISystem::has_boots(4),
          "ui_boots: jump power 4 (default) should not indicate boots");
    check(UISystem::has_boots(5),
          "ui_boots: jump power 5 (with boots) should indicate boots");
    check(!UISystem::has_boots(0),
          "ui_boots: jump power 0 should not indicate boots");
    check(!UISystem::has_boots(3),
          "ui_boots: jump power 3 should not indicate boots");
}

/**
 * Test edge cases for score conversion
 */
static void test_ui_score_edge_cases() {
    uint8_t digits[6];
    
    // Test single digit in each position
    uint8_t score_1[3] = {1, 0, 0};
    UISystem::score_bytes_to_digits(score_1, digits);
    check(digits[5] == 1,
          "ui_score_edge: score 1 should have rightmost digit as 1");
    
    uint8_t score_100[3] = {0, 1, 0};
    UISystem::score_bytes_to_digits(score_100, digits);
    check(digits[3] == 1 && digits[4] == 0 && digits[5] == 0,
          "ui_score_edge: score 100 should convert to 0,0,0,1,0,0");
    
    uint8_t score_10000[3] = {0, 0, 1};
    UISystem::score_bytes_to_digits(score_10000, digits);
    check(digits[1] == 1 && digits[2] == 0,
          "ui_score_edge: score 10000 should have digits[1]=1");
}

/**
 * Test all meter states for comprehensive coverage
 */
static void test_ui_meter_all_states() {
    // Test each meter value from 0-12 maps correctly
    const uint8_t expected_states[13][6] = {
        // meter 0: all empty
        {0, 0, 0, 0, 0, 0},
        // meter 1: cell 0 half, rest empty
        {1, 0, 0, 0, 0, 0},
        // meter 2: cell 0 full, rest empty
        {2, 0, 0, 0, 0, 0},
        // meter 3: cell 0 full, cell 1 half
        {2, 1, 0, 0, 0, 0},
        // meter 4: cells 0-1 full
        {2, 2, 0, 0, 0, 0},
        // meter 5: cells 0-1 full, cell 2 half
        {2, 2, 1, 0, 0, 0},
        // meter 6: cells 0-2 full
        {2, 2, 2, 0, 0, 0},
        // meter 7: cells 0-2 full, cell 3 half
        {2, 2, 2, 1, 0, 0},
        // meter 8: cells 0-3 full
        {2, 2, 2, 2, 0, 0},
        // meter 9: cells 0-3 full, cell 4 half
        {2, 2, 2, 2, 1, 0},
        // meter 10: cells 0-4 full
        {2, 2, 2, 2, 2, 0},
        // meter 11: cells 0-4 full, cell 5 half
        {2, 2, 2, 2, 2, 1},
        // meter 12: all full
        {2, 2, 2, 2, 2, 2}
    };
    
    for (uint8_t meter = 0; meter <= 12; meter++) {
        for (uint8_t cell = 0; cell < 6; cell++) {
            uint8_t state = UISystem::fireball_meter_to_cell_state(meter, cell);
            check(state == expected_states[meter][cell],
                  "ui_meter_all_states: meter value mismatch");
        }
    }
}



static const std::vector<TestCase>& test_registry() {
    static const std::vector<TestCase> tests = {
        {"physics_tiles", test_physics_tiles},
        {"animation_looping", test_animation_looping},
        {"animation_non_looping", test_animation_non_looping},
        {"animation_zero_duration", test_animation_zero_duration},
        {"enemy_animation_sequence", test_enemy_animation_sequence},
        {"jump_edge_trigger", test_jump_edge_trigger},
        {"jump_recharge", test_jump_recharge},
        {"jump_height", test_jump_height},
        {"space_level_uses_lower_gravity", test_space_level_uses_lower_gravity},
        {"jump_top_clamped_to_playfield", test_jump_top_clamped_to_playfield},
        {"player_death_sequence_respawn", test_player_death_sequence_respawn},
        {"player_death_sequence_game_over", test_player_death_sequence_game_over},
        {"teleport_does_not_update_respawn_checkpoint", test_teleport_does_not_update_respawn_checkpoint},
        {"high_score_bytes_conversion", test_high_score_bytes_conversion},
        {"door_activation_alignment_x", test_door_activation_alignment_x},
        {"door_activation_alignment_y", test_door_activation_alignment_y},
        {"door_key_requirement", test_door_key_requirement},
        {"door_open_key_requirement", test_door_open_key_requirement},
        {"door_state_update_same_level", test_door_state_update_same_level},
        {"door_state_update_different_level", test_door_state_update_different_level},
        {"runtime_level_tiles_populated", test_runtime_level_tiles_populated},
        {"door_entry_sets_checkpoint_for_respawn", test_door_entry_sets_checkpoint_for_respawn},
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
        {"actor_restraint_throttling", test_actor_restraint_throttling},
        {"award_points_no_carry", test_award_points_no_carry},
        {"award_points_accumulates_in_byte0", test_award_points_accumulates_in_byte0},
        {"award_points_carry_into_byte1", test_award_points_carry_into_byte1},
        {"award_points_full_carry_amount", test_award_points_full_carry_amount},
        {"award_points_large_value_above_255", test_award_points_large_value_above_255},
        {"award_points_max_score_saturation", test_award_points_max_score_saturation},
        {"award_points_large_carry_saturation", test_award_points_large_carry_saturation},
        {"actor_door_key_sync", test_actor_door_key_sync},
        {"fireball_meter_depletion_timing", test_fireball_meter_depletion_timing},
        {"fireball_meter_recharge_timing", test_fireball_meter_recharge_timing},
        {"fireball_offscreen_deactivates", test_fireball_offscreen_deactivates},
        {"fireball_collision_sets_white_spark", test_fireball_collision_sets_white_spark},
        {"fireball_corkscrew_motion", test_fireball_corkscrew_motion},
        {"item_collection_tracking", test_item_collection_tracking},
        {"item_blastola_cola_firepower", test_item_blastola_cola_firepower},
        {"item_boots_jump_power", test_item_boots_jump_power},
        {"item_corkscrew_flag", test_item_corkscrew_flag},
        {"item_treasure_counting", test_item_treasure_counting},
        {"item_special_items", test_item_special_items},
        {"audio_init_shutdown_idempotency", test_audio_init_shutdown_idempotency},
        {"audio_graceful_failure_when_not_initialized", test_audio_graceful_failure_when_not_initialized},
        {"audio_priority_interrupt", test_audio_priority_interrupt},
        {"audio_priority_blocking", test_audio_priority_blocking},
        {"audio_all_sounds_playable", test_audio_all_sounds_playable},
        {"audio_music_playback", test_audio_music_playback},
        {"ui_score_base100_encoding", test_ui_score_base100_encoding},
        {"ui_fireball_meter_cell_mapping", test_ui_fireball_meter_cell_mapping},
        {"ui_boots_detection", test_ui_boots_detection},
        {"ui_score_edge_cases", test_ui_score_edge_cases},
        {"ui_meter_all_states", test_ui_meter_all_states}
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
    // Ensure level data is initialized before any test uses it
    initialize_level_data();

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
