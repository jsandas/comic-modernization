#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <filesystem>
#include "Constants.h"

// Enemy structure
struct Enemy {
    uint8_t y, x;
    int8_t y_vel, x_vel;
    uint8_t state;
    uint8_t behavior;
    uint8_t facing;
    uint8_t restraint;
    uint8_t animation_frame;
    uint8_t num_animation_frames;
    uint8_t spawn_timer;
};

// Fireball structure
struct Fireball {
    uint8_t y, x;
    int8_t y_vel, x_vel;
    uint8_t facing;
};

// Item structure
struct Item {
    uint8_t y, x;
    uint8_t type;
    bool collected;
};

// Door structure
struct Door {
    uint8_t y, x;
    uint8_t destination_level;
    uint8_t destination_stage;
};

// Tile map (128x10 tiles)
struct TileMap {
    std::vector<uint8_t> tiles;
    std::vector<uint8_t> solidity;  // 1 = solid, 0 = passable

    TileMap() : tiles(GameConstants::SCREEN_WIDTH_TILES * GameConstants::SCREEN_HEIGHT_TILES, 0),
                solidity(GameConstants::SCREEN_WIDTH_TILES * GameConstants::SCREEN_HEIGHT_TILES, 0) {}
};

// Level structure
struct Level {
    std::string name;
    uint8_t level_number;
    std::vector<TileMap> stages;  // 3 stages per level
    std::vector<std::vector<Enemy>> stage_enemies;  // enemies per stage
    std::vector<std::vector<Item>> stage_items;    // items per stage
    std::vector<std::vector<Door>> stage_doors;    // doors per stage
};

// Game state structure
struct GameState {
    // Player data (use signed ints for pixels)
    int16_t comic_x, comic_y;
    int16_t comic_x_vel, comic_y_vel;
    uint8_t comic_facing;  // 0 = left, 1 = right
    uint8_t comic_hp;
    uint8_t comic_num_lives;
    uint8_t comic_fireball_meter;
    uint8_t comic_shield_meter;  // Shield protection meter
    bool comic_has_lantern;
    bool comic_has_boots;        // Jump ability
    bool comic_has_teleport;     // Teleport wand
    bool comic_has_crown;        // Crown item
    bool comic_has_gold;         // Gold item
    uint8_t comic_num_gems;      // Gem count

    // Damage/invulnerability counter (ticks)
    int comic_invuln_ticks = 0;
    // Current level/stage
    uint8_t current_level_number;
    uint8_t current_stage_number;
    std::unique_ptr<Level> current_level;
    std::unique_ptr<TileMap> current_map;

    // Physics and collision: update per tick based on input
    void update(const class Input& input);

    // Collision helpers
    bool isSolidTileAt(int tx, int ty) const;
    bool isRectSolid(int px, int py, int w, int h) const;
    bool isOnGround() const;

    // Enemies and projectiles
    std::vector<Enemy> enemies;
    std::vector<Fireball> fireballs;

    // Game state flags
    bool game_over;
    bool stage_complete;
    uint16_t score;
    uint8_t win_counter;  // animation counter for winning a stage

    // Camera position
    int16_t camera_x;

    GameState() 
        : comic_x(0), comic_y(0), comic_x_vel(0), comic_y_vel(0),
          comic_facing(1), comic_hp(GameConstants::MAX_HP),
          comic_num_lives(GameConstants::MAX_NUM_LIVES - 1),
          comic_fireball_meter(0), comic_shield_meter(0),
          comic_has_lantern(false), comic_has_boots(false),
          comic_has_teleport(false), comic_has_crown(false),
          comic_has_gold(false), comic_num_gems(0),
          current_level_number(GameConstants::LEVEL_NUMBER_FOREST),
          current_stage_number(0),
          game_over(false), stage_complete(false),
          score(0), win_counter(0), camera_x(0) {
        enemies.resize(GameConstants::MAX_NUM_ENEMIES);
        fireballs.resize(GameConstants::MAX_NUM_FIREBALLS);
        current_map = std::make_unique<TileMap>();
    }

    // Load level/pt data from data directory (uses preview PNGs + tileset images). Returns true on success.
    bool loadLevel(int level_number, const std::filesystem::path& dataPath);
};
