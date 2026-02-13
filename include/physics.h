#ifndef PHYSICS_H
#define PHYSICS_H

#include <cstdint>
#include <string>

// Physics constants (from jsandas/comic-c physics.h)
constexpr int COMIC_GRAVITY = 5;           // Gravity (units of 1/8 game units per tick)
constexpr int COMIC_GRAVITY_SPACE = 3;     // Reduced gravity in space level
constexpr int TERMINAL_VELOCITY = 23;      // Maximum downward velocity
constexpr int JUMP_POWER_DEFAULT = 4;      // Default jump power
constexpr int JUMP_POWER_WITH_BOOTS = 5;   // Jump power with Boots item
constexpr int JUMP_ACCELERATION = 7;       // Upward acceleration during jump

// Map dimensions (from jsandas/comic-c globals.h)
constexpr int MAP_WIDTH_TILES = 128;       // Map width in tiles
constexpr int MAP_HEIGHT_TILES = 10;       // Map height in tiles
constexpr int MAP_WIDTH = 256;             // Map width in game units (128 * 2)
constexpr int MAP_HEIGHT = 20;             // Map height in game units (10 * 2)
constexpr int PLAYFIELD_WIDTH = 24;        // Visible playfield width in game units
constexpr int PLAYFIELD_HEIGHT = 20;       // Visible playfield height in game units

// Functions
void process_jump_input();
void handle_fall_or_jump();
void move_left();
void move_right();

// Tile system
void init_test_level();
bool load_level_from_file(const std::string& level_name, int stage_number);
uint8_t get_tile_at(uint8_t x, uint8_t y);
bool is_tile_solid(uint8_t tile_id);

#endif // PHYSICS_H
