#include <iostream>
#include <SDL2/SDL.h>
#include "game/GameState.h"
#include "input/Input.h"

int main() {
    // 1) Knockback on damage: enemy on the right pushes player left
    {
        GameState g;
        g.current_map = std::make_unique<TileMap>();
        g.comic_hp = 6;
        g.comic_x = 100; g.comic_y = 100;

        Enemy e{}; e.behavior = GameConstants::ENEMY_BEHAVIOR_BOUNCE; e.x = 110; e.y = 100; // to the right (overlapping by a few pixels)
        g.enemies.clear(); g.enemies.push_back(e);

        Input input;
        g.update(input);
        if (g.comic_hp != 5) { std::cerr << "Knockback test: HP not decremented\n"; return 1; }
        if (g.comic_x_vel == 0) { std::cerr << "Knockback test: expected non-zero x_vel when hit, got " << g.comic_x_vel << "\n"; return 1; }
        if (g.comic_y_vel >= 0) { std::cerr << "Knockback test: expected upward y_vel when hit, got " << g.comic_y_vel << "\n"; return 1; }
    }

    // 2) Damage-on-contact across behaviors: ensure each behavior damages the player when overlapping
    {
        for (int b = GameConstants::ENEMY_BEHAVIOR_BOUNCE; b <= GameConstants::ENEMY_BEHAVIOR_SHY; ++b) {
            GameState g;
            g.current_map = std::make_unique<TileMap>();
            g.comic_hp = 6;
            g.comic_x = 50; g.comic_y = 50;
            Enemy e{}; e.behavior = static_cast<uint8_t>(b); e.x = 50; e.y = 50;
            g.enemies.clear(); g.enemies.push_back(e);
            Input input;
            g.update(input);
            if (g.comic_hp != 5) { std::cerr << "Damage behavior " << b << " did not hit player as expected\n"; return 1; }
        }
    }

    // 3) Player death handling: HP reaches 0 -> game_over = true
    {
        GameState g;
        g.current_map = std::make_unique<TileMap>();
        g.comic_hp = 1;
        g.comic_x = 200; g.comic_y = 200;

        Enemy e{}; e.behavior = GameConstants::ENEMY_BEHAVIOR_BOUNCE; e.x = 200; e.y = 200;
        g.enemies.clear(); g.enemies.push_back(e);
        Input input;
        g.update(input);
        if (g.comic_hp != 0) { std::cerr << "Death test: HP not zero after lethal hit\n"; return 1; }
        if (!g.game_over) { std::cerr << "Death test: game_over not set when HP reached 0\n"; return 1; }
    }

    std::cout << "Player damage/knockback/death tests passed" << std::endl;
    return 0;
}
