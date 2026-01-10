#pragma once

#include <cstdint>
#include <array>

// Game constants from assembly
namespace GameConstants {
    // Resolution
    constexpr int SCREEN_WIDTH = 320;
    constexpr int SCREEN_HEIGHT = 200;
    constexpr int SCREEN_WIDTH_TILES = 128;
    constexpr int SCREEN_HEIGHT_TILES = 10;
    constexpr int TILE_SIZE = 16;

    // Game tick rate (approximately 18.2 Hz / 2)
    constexpr float TICK_RATE = 9.109f;  // Hz
    constexpr int TICK_INTERVAL_MS = 110;  // milliseconds

    // Physics constants
    constexpr int COMIC_GRAVITY = 5;
    constexpr int COMIC_GRAVITY_SPACE = 3;
    constexpr int TERMINAL_VELOCITY = 23;  // in units of 1/8 game units per tick

    // Game mechanics
    constexpr int MAX_HP = 6;
    constexpr int MAX_FIREBALL_METER = 12;
    constexpr int MAX_SHIELD_METER = 8;  // Shield protection meter
    constexpr int MAX_NUM_LIVES = 5;
    constexpr int MAX_NUM_ENEMIES = 4;
    constexpr int MAX_NUM_FIREBALLS = 5;
    constexpr int MAX_NUM_DOORS = 3;
    constexpr int TELEPORT_DISTANCE = 6;
    constexpr int ENEMY_DESPAWN_RADIUS = 30;

    // EGA color palette (RGB values, will be converted to SDL)
    // Standard CGA/EGA palette
    constexpr std::array<uint32_t, 16> EGA_PALETTE = {{
        0x000000,  // 0: Black
        0x0000AA,  // 1: Blue
        0x00AA00,  // 2: Green
        0x00AAAA,  // 3: Cyan
        0xAA0000,  // 4: Red
        0xAA00AA,  // 5: Magenta
        0xAA5500,  // 6: Brown
        0xAAAAAA,  // 7: Light Gray
        0x555555,  // 8: Dark Gray
        0x5555FF,  // 9: Light Blue
        0x55FF55,  // 10: Light Green
        0x55FFFF,  // 11: Light Cyan
        0xFF5555,  // 12: Light Red
        0xFF55FF,  // 13: Light Magenta
        0xFFFF55,  // 14: Yellow
        0xFFFFFF   // 15: White
    }};

    // Level numbers
    enum LevelNumber {
        LEVEL_NUMBER_LAKE = 0,
        LEVEL_NUMBER_FOREST = 1,
        LEVEL_NUMBER_SPACE = 2,
        LEVEL_NUMBER_COMP = 3,
        LEVEL_NUMBER_CAVE = 4,
        LEVEL_NUMBER_SHED = 5,
        LEVEL_NUMBER_CASTLE = 6,
        LEVEL_NUMBER_BASE = 7
    };

    // Player sprite dimensions (in tiles)
    constexpr int PLAYER_WIDTH_TILES = 1;
    constexpr int PLAYER_HEIGHT_TILES = 4; // player occupies 4 tiles vertically (sprite height)

    // Enemy behaviors
    enum EnemyBehavior {
        ENEMY_BEHAVIOR_BOUNCE = 0,
        ENEMY_BEHAVIOR_LEAP = 1,
        ENEMY_BEHAVIOR_ROLL = 2,
        ENEMY_BEHAVIOR_SEEK = 3,
        ENEMY_BEHAVIOR_SHY = 4
    };
}
