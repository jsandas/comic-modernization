#ifndef ACTORS_H
#define ACTORS_H

#include <cstdint>
#include <vector>
#include "graphics.h"
#include "level.h"
#include "physics.h"

/* Enemy state constants */
constexpr uint8_t ENEMY_STATE_DESPAWNED = 0;      /* Not yet spawned */
constexpr uint8_t ENEMY_STATE_SPAWNED = 1;        /* Active and moving */
constexpr uint8_t ENEMY_STATE_WHITE_SPARK = 2;    /* Death animation (frames 2-7) */
constexpr uint8_t ENEMY_STATE_RED_SPARK = 8;      /* Death animation (frames 8-13) */
constexpr uint8_t DEATH_ANIMATION_LAST_FRAME = 5; /* Offset from start to last frame */

/* Enemy restraint (movement throttle) */
constexpr uint8_t ENEMY_RESTRAINT_MOVE_THIS_TICK = 0;   /* Move, then skip next */
constexpr uint8_t ENEMY_RESTRAINT_SKIP_THIS_TICK = 1;   /* Skip, then move next */
constexpr uint8_t ENEMY_RESTRAINT_MOVE_EVERY_TICK = 2;  /* Always move (fast) */

/* Enemy physics constants */
constexpr int ENEMY_DESPAWN_RADIUS = 30;  /* Game units from Comic */
constexpr int ENEMY_VELOCITY_SHIFT = 3;   /* Bit shift for 1/8 unit movement precision */
constexpr int8_t ENEMY_GRAVITY = 2;       /* Acceleration per tick (vs COMIC_GRAVITY = 5) */

/* Respawn timer cycle: 20→40→60→80→100→20 */
constexpr uint8_t RESPAWN_TIMER_MIN = 20;
constexpr uint8_t RESPAWN_TIMER_MAX = 100;
constexpr uint8_t RESPAWN_TIMER_STEP = 20;

/* Enemy facing directions (animation frame offsets) */
constexpr uint8_t ENEMY_FACING_LEFT = 0;
constexpr uint8_t ENEMY_FACING_RIGHT = 5;

/**
 * enemy_t - Enemy state
 *
 * Tracks position, velocity, animation, and behavior state of a single enemy.
 */
struct enemy_t {
    uint8_t y;                           /* Y position (game units) */
    uint8_t x;                           /* X position (game units) */
    int8_t x_vel;                        /* Horizontal velocity */
    int8_t y_vel;                        /* Vertical velocity */

    /* Dual-use field: spawn timer when despawned, animation frame when spawned */
    uint8_t spawn_timer_and_animation;

    uint8_t num_animation_frames;        /* Frames in animation */
    uint8_t behavior;                    /* AI behavior type (see ENEMY_BEHAVIOR_*) */
    uint8_t state;                       /* Current state (ENEMY_STATE_*) */
    uint8_t facing;                      /* 0=left, 5=right (in units of animation frames) */
    uint8_t restraint;                   /* Movement throttle (ENEMY_RESTRAINT_*) */

    /* Cached references for animation and sprite rendering */
    SpriteAnimationData* animation_data;
};

/**
 * ActorSystem - Manages all enemies, fireballs, and items
 *
 * Handles enemy spawning, AI behavior, collision detection, animation,
 * and cleanup.
 */
class ActorSystem {
public:
    ActorSystem();
    ~ActorSystem();

    /* Initialize the actor system */
    bool initialize();

    /* Update all actors for one game tick */
    void update(
        uint8_t comic_x, uint8_t comic_y,
        uint8_t comic_facing,
        const uint8_t* current_tiles,
        int camera_x
    );

    /* Get array of enemies */
    const std::vector<enemy_t>& get_enemies() const { return enemies; }

    /* Reset all enemies (called when loading a new stage) */
    void reset_for_stage();

    /* Setup enemies for a stage from level data */
    void setup_enemies_for_stage(
        const class level_t* level,
        int stage_number,
        GraphicsSystem* graphics_system
    );

    /* Check if a specific tile is solid */
    bool is_tile_solid(uint8_t tile_id) const;

    /* Get tile at coordinates */
    uint8_t get_tile_at(uint8_t x, uint8_t y) const;

protected:
    /* Enemy array (max 4 enemies per stage) */
    std::vector<enemy_t> enemies;

    /* Level and stage context */
    const uint8_t* current_tiles;
    int current_map_width_tiles;
    int current_map_height_tiles;
    uint8_t tileset_last_passable;

    /* Spawning control */
    uint8_t spawned_this_tick;
    uint8_t spawn_offset_cycle;
    uint8_t enemy_respawn_counter_cycle;

    /* Global state updates (passed from game loop) */
    uint8_t g_comic_x;
    uint8_t g_comic_y;
    uint8_t g_comic_facing;
    int g_camera_x;

    /* Private helper functions */
    bool maybe_spawn_enemy(int enemy_index);
    void handle_single_enemy(int enemy_index);
    void update_enemy_animation(enemy_t* enemy);
    void check_enemy_despawn(enemy_t* enemy);
    void check_enemy_player_collision(enemy_t* enemy);

    /* AI behavior functions */
    void enemy_behavior_bounce(enemy_t* enemy);
    void enemy_behavior_leap(enemy_t* enemy);
    void enemy_behavior_roll(enemy_t* enemy);
    void enemy_behavior_seek(enemy_t* enemy);
    void enemy_behavior_shy(enemy_t* enemy);

    /* Collision detection helpers */
    bool check_horizontal_enemy_map_collision(uint8_t x, uint8_t y) const;
    bool check_vertical_enemy_map_collision(uint8_t x, uint8_t y) const;
};

#endif /* ACTORS_H */
