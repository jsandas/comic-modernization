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
        en.x = static_cast<int16_t>(wall_tx * GameConstants::TILE_SIZE - 8);
        en.y = static_cast<int16_t>(wall_ty * GameConstants::TILE_SIZE); // align vertically with wall tile
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
        en.x = static_cast<int16_t>(ground_tx * GameConstants::TILE_SIZE);
        en.y = static_cast<int16_t>(ground_ty * GameConstants::TILE_SIZE - GameConstants::TILE_SIZE);
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
        en.x = static_cast<int16_t>(wall_tx * GameConstants::TILE_SIZE - 20);
        en.y = static_cast<int16_t>(wall_ty * GameConstants::TILE_SIZE);
        en.x_vel = 2;
        g.enemies.clear(); g.enemies.push_back(en);

        Input input;
        bool reversed = false;
        for (int i = 0; i < 16; ++i) {
            g.update(input);
            if (g.enemies[0].x_vel < 0) { reversed = true; break; }
        }
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

    // --- Edge-case tests ---
    // Bounce: immediate collision at boundary shouldn't overlap wall and should reverse
    {
        GameState g;
        g.current_map = std::make_unique<TileMap>();
        int wall_tx = 15, wall_ty = 5;
        g.current_map->solidity[wall_ty * GameConstants::SCREEN_WIDTH_TILES + wall_tx] = 1;

        Enemy en{};
        en.behavior = GameConstants::ENEMY_BEHAVIOR_BOUNCE;
        // Place enemy so its right edge is just left of the wall; a move of +2 would push into wall
        en.x = static_cast<int16_t>(wall_tx * GameConstants::TILE_SIZE - GameConstants::TILE_SIZE - 1);
        en.y = static_cast<int16_t>(wall_ty * GameConstants::TILE_SIZE);
        en.x_vel = 2;
        g.enemies.clear(); g.enemies.push_back(en);

        Input input;
        g.update(input);
        if (g.enemies[0].x_vel >= 0) { std::cerr << "Bounce edge-case failed to reverse on immediate wall\n"; return 1; }
        int right_edge = g.enemies[0].x + GameConstants::TILE_SIZE - 1;
        if (right_edge >= wall_tx * GameConstants::TILE_SIZE) { std::cerr << "Bounce edge-case overlap wall\n"; return 1; }
    }

    // Leap: ensure state cooldown is set and decreases; also ceiling collision during leap stops y_vel
    {
        GameState g;
        g.current_map = std::make_unique<TileMap>();
        int ground_tx = 6, ground_ty = 7;
        // Use small tile indices to match Enemy's uint8 coordinate range
        int ceiling_tx = 8, ceiling_ty = 3;
        g.current_map->solidity[ground_ty * GameConstants::SCREEN_WIDTH_TILES + ground_tx] = 1;
        g.current_map->solidity[ceiling_ty * GameConstants::SCREEN_WIDTH_TILES + ceiling_tx] = 1;

        // Cooldown behavior
        Enemy en1{}; en1.behavior = GameConstants::ENEMY_BEHAVIOR_LEAP; en1.x = static_cast<int16_t>(ground_tx * GameConstants::TILE_SIZE); en1.y = static_cast<int16_t>(ground_ty * GameConstants::TILE_SIZE - GameConstants::TILE_SIZE); en1.state = 0;
        g.enemies.clear(); g.enemies.push_back(en1);
        Input input;
        g.update(input);
        if (g.enemies[0].state != 12) { std::cerr << "Leap cooldown not set\n"; return 1; }
        g.update(input);
        if (g.enemies[0].state != 11) { std::cerr << "Leap cooldown not decrementing\n"; return 1; }

        // Ceiling collision: place enemy under ceiling and force an upward velocity to hit it
        Enemy en2{}; en2.behavior = GameConstants::ENEMY_BEHAVIOR_LEAP; en2.x = static_cast<int16_t>(ceiling_tx * GameConstants::TILE_SIZE); en2.y = static_cast<int16_t>((ceiling_ty + 1) * GameConstants::TILE_SIZE); en2.state = 0;
        g.enemies.clear(); g.enemies.push_back(en2);
        g.enemies[0].y_vel = -8; // strong upward
        // Debug: print pre-update info
        std::cerr << "Leap ceiling debug: start y=" << (int)g.enemies[0].y << " y_vel=" << (int)g.enemies[0].y_vel << " ceiling_y=" << (ceiling_ty * GameConstants::TILE_SIZE) << "\n";
        int enemy_w = GameConstants::TILE_SIZE, enemy_h = GameConstants::TILE_SIZE;
        // Simulate the new_y_vel the GameState update will compute when state==0
        int simulated_new_y_vel = -6 + 1; // state==0 sets -6 then add gravity
        int simulated_target_y = g.enemies[0].y + simulated_new_y_vel;
        std::cerr << "Leap ceiling debug: simulated_target_y=" << simulated_target_y << " isSolidTileAt=" << g.isSolidTileAt(ceiling_tx, ceiling_ty) << " isRectSolid(sim_target)=" << g.isRectSolid(g.enemies[0].x, simulated_target_y, enemy_w, enemy_h) << "\n";
        g.update(input);
        std::cerr << "Leap ceiling debug: after y=" << (int)g.enemies[0].y << " y_vel=" << (int)g.enemies[0].y_vel << "\n";
        if (g.enemies[0].y_vel != 0) { std::cerr << "Leap ceiling collision failed to stop upward motion\n"; return 1; }
        if (g.enemies[0].y < (ceiling_ty + 1) * GameConstants::TILE_SIZE) { std::cerr << "Leap ceiling alignment failed\n"; return 1; }
    }

    // Roll: high-speed roll should reverse on obstacle and not overlap
    {
        GameState g;
        g.current_map = std::make_unique<TileMap>();
        int wall_tx = 12, wall_ty = 4;
        g.current_map->solidity[wall_ty * GameConstants::SCREEN_WIDTH_TILES + wall_tx] = 1;

        Enemy en{}; en.behavior = GameConstants::ENEMY_BEHAVIOR_ROLL;
        en.x = static_cast<int16_t>(wall_tx * GameConstants::TILE_SIZE - GameConstants::TILE_SIZE - 2);
        en.y = static_cast<int16_t>(wall_ty * GameConstants::TILE_SIZE);
        en.x_vel = 6; // faster than default
        g.enemies.clear(); g.enemies.push_back(en);

        Input input;
        bool reversed = false;
        std::cerr << "roll start x=" << (int)g.enemies[0].x << " x_vel=" << (int)g.enemies[0].x_vel << " wall_x=" << (wall_tx * GameConstants::TILE_SIZE) << "\n";
        for (int i = 0; i < 12; ++i) {
            int tx = g.enemies[0].x + g.enemies[0].x_vel;
            std::cerr << "tick " << i << " pre x=" << (int)g.enemies[0].x << " target_x=" << tx << " x_vel=" << (int)g.enemies[0].x_vel << " isRectSolidTarget=" << g.isRectSolid(tx, g.enemies[0].y, GameConstants::TILE_SIZE, GameConstants::TILE_SIZE) << "\n";
            g.update(input);
            if (g.enemies[0].x_vel < 0) { reversed = true; break; }
        }
        if (!reversed) { std::cerr << "Roll edge-case failed to reverse\n"; return 1; }
    }

    // Seek: if blocked directly by a wall in front, x_vel should become 0 and position unchanged
    {
        GameState g;
        g.current_map = std::make_unique<TileMap>();
        Enemy en{}; en.behavior = GameConstants::ENEMY_BEHAVIOR_SEEK; en.x = 40; en.y = 40; en.x_vel = 0;
        g.enemies.clear(); g.enemies.push_back(en);
        // place player to the right but a wall immediately to the right of enemy
        g.comic_x = 200; g.comic_y = 40;
        int wall_tx = (en.x + GameConstants::TILE_SIZE) / GameConstants::TILE_SIZE; // tile to the right
        int wall_ty = en.y / GameConstants::TILE_SIZE;
        g.current_map->solidity[wall_ty * GameConstants::SCREEN_WIDTH_TILES + wall_tx] = 1;

        Input input;
        g.update(input);
        if (g.enemies[0].x != 40 || g.enemies[0].x_vel != 0) { std::cerr << "Seek didn't stop when blocked\n"; return 1; }
    }

    // Shy: threshold and blocked behavior
    {
        GameState g;
        g.current_map = std::make_unique<TileMap>();
        Enemy en{}; en.behavior = GameConstants::ENEMY_BEHAVIOR_SHY; en.x = 120; en.y = 40; en.x_vel = 0;
        g.enemies.clear(); g.enemies.push_back(en);

        // exactly at threshold: player at +32 -> no movement
        g.comic_x = 152; g.comic_y = 40; // dx = 32
        Input input;
        g.update(input);
        if (g.enemies[0].x != 120) { std::cerr << "Shy moved at threshold (should not)\n"; return 1; }

        // inside threshold: move away
        g.comic_x = 149; // dx = 29
        g.update(input);
        if (g.enemies[0].x == 120) { std::cerr << "Shy didn't move away when player close\n"; return 1; }

        // blocked by wall: place wall in direction it would move and ensure it doesn't move into wall
        int wall_tx = (g.enemies[0].x + (g.comic_x > g.enemies[0].x ? -1 : 1)) / GameConstants::TILE_SIZE; // approximate tile near
        int wall_ty = g.enemies[0].y / GameConstants::TILE_SIZE;
        g.current_map->solidity[wall_ty * GameConstants::SCREEN_WIDTH_TILES + wall_tx] = 1;
        g.update(input);
        if (g.enemies[0].x == wall_tx * GameConstants::TILE_SIZE) { std::cerr << "Shy moved into blocking wall\n"; return 1; }
    }

    std::cout << "All enemy-AI tests passed" << std::endl;
    return 0;
}
