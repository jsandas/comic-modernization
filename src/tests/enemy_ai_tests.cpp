#include <iostream>
#include <SDL2/SDL.h>
#include "game/GameState.h"
#include "input/Input.h"

int main() {
    // Bounce behavior: reverses when hitting a wall
    {
        GameState g;
        g.current_map = std::make_unique<TileMap>();
        int wall_tx = 10;
        int wall_ty = 5;
        g.current_map->solidity[wall_ty * GameConstants::SCREEN_WIDTH_TILES + wall_tx] = 1;

        Enemy en{};
        en.behavior = GameConstants::ENEMY_BEHAVIOR_BOUNCE;
        en.x = static_cast<uint8_t>(wall_tx * GameConstants::TILE_SIZE - 8);
        en.y = static_cast<uint8_t>(wall_ty * GameConstants::TILE_SIZE - GameConstants::TILE_SIZE);
        en.x_vel = 2;
        g.enemies.clear(); g.enemies.push_back(en);

        Input input;
        // Run a few ticks — should hit the wall and reverse
        bool reversed = false;
        for (int i = 0; i < 8; ++i) {
            g.update(input);
            if (g.enemies[0].x_vel < 0) { reversed = true; break; }
        }
        if (!reversed) {
            std::cerr << "Bounce behavior failed to reverse on wall\n";
            return 1;
        }
    }

    // Leap behavior: initiates a jump and lands
    {
        GameState g;
        g.current_map = std::make_unique<TileMap>();
        int ground_tx = 6; int ground_ty = 7;
        g.current_map->solidity[ground_ty * GameConstants::SCREEN_WIDTH_TILES + ground_tx] = 1;

        Enemy en{};
        en.behavior = GameConstants::ENEMY_BEHAVIOR_LEAP;
        en.x = static_cast<uint8_t>(ground_tx * GameConstants::TILE_SIZE);
        en.y = static_cast<uint8_t>(ground_ty * GameConstants::TILE_SIZE - GameConstants::TILE_SIZE);
        en.state = 0; // trigger a leap
        g.enemies.clear(); g.enemies.push_back(en);

        Input input;
        g.update(input); // should start leap
        if (g.enemies[0].y_vel >= 0) { std::cerr << "Leap didn't start (expected negative y_vel)\n"; return 1; }

        // Run until it lands (y_vel becomes 0 after collision)
        bool landed = false;
        for (int i = 0; i < 30; ++i) {
            g.update(input);
            if (g.enemies[0].y_vel == 0) { landed = true; break; }
        }
        if (!landed) { std::cerr << "Leap didn't land within expected ticks\n"; return 1; }
    }

    // Roll behavior: moves horizontally continuously and reverses on obstacle
    {
        GameState g;
        g.current_map = std::make_unique<TileMap>();
        int wall_tx = 12; int wall_ty = 4;
        g.current_map->solidity[wall_ty * GameConstants::SCREEN_WIDTH_TILES + wall_tx] = 1;

        Enemy en{};
        en.behavior = GameConstants::ENEMY_BEHAVIOR_ROLL;
        en.x = static_cast<uint8_t>(wall_tx * GameConstants::TILE_SIZE - 20);
        en.y = static_cast<uint8_t>(wall_ty * GameConstants::TILE_SIZE - GameConstants::TILE_SIZE);
        en.x_vel = 2;
        g.enemies.clear(); g.enemies.push_back(en);

        Input input;
        bool reversed = false;
        for (int i = 0; i < 16; ++i) { g.update(input); if (g.enemies[0].x_vel < 0) { reversed = true; break; } }
        if (!reversed) { std::cerr << "Roll didn't reverse on obstacle\n"; return 1; }
    }

    // Seek behavior: moves towards player
    {
        GameState g;
        g.current_map = std::make_unique<TileMap>();
        Enemy en{};
        en.behavior = GameConstants::ENEMY_BEHAVIOR_SEEK;
        en.x = 20; en.y = 20; en.x_vel = 0;
        g.enemies.clear(); g.enemies.push_back(en);
        g.comic_x = 100; g.comic_y = 20;
        Input input;
        g.update(input);
        if (g.enemies[0].x <= 20) { std::cerr << "Seek didn't move toward player\n"; return 1; }
    }

    // Shy behavior: moves away when player close
    {
        GameState g;
        g.current_map = std::make_unique<TileMap>();
        Enemy en{};
        en.behavior = GameConstants::ENEMY_BEHAVIOR_SHY;
        en.x = 100; en.y = 40; en.x_vel = 0;
        g.enemies.clear(); g.enemies.push_back(en);
        // place player close
        g.comic_x = 110; g.comic_y = 40;
        Input input;
        g.update(input);
        if (g.enemies[0].x >= 100) { std::cerr << "Shy didn't move away from player\n"; return 1; }
    }

    std::cout << "All enemy-AI tests passed" << std::endl;
    return 0;
}
