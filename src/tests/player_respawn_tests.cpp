#include <iostream>
#include <SDL2/SDL.h>
#include "game/GameState.h"
#include "input/Input.h"

int main() {
    // 1) Respawn when lives remain: lives decremented, HP reset, invuln set
    {
        GameState g;
        g.current_map = std::make_unique<TileMap>();
        g.comic_num_lives = 2;
        g.comic_hp = 1;
        g.comic_x = 50; g.comic_y = 50;

        Enemy e{}; e.behavior = GameConstants::ENEMY_BEHAVIOR_BOUNCE; e.x = 50; e.y = 50;
        g.enemies.clear(); g.enemies.push_back(e);

        Input input;
        g.update(input);
        if (g.comic_num_lives != 1) { std::cerr << "Respawn test: lives not decremented (expected 1 got " << (int)g.comic_num_lives << ")\n"; return 1; }
        if (g.comic_hp != GameConstants::MAX_HP) { std::cerr << "Respawn test: HP not reset on respawn\n"; return 1; }
        if (g.comic_invuln_ticks == 0) { std::cerr << "Respawn test: invulnerability not set on respawn\n"; return 1; }
        if (g.game_over) { std::cerr << "Respawn test: game_over should be false when lives remain\n"; return 1; }
    }

    // 2) Game over when no lives remain: game_over remains true and no respawn
    {
        GameState g;
        g.current_map = std::make_unique<TileMap>();
        g.comic_num_lives = 0;
        g.comic_hp = 1;
        g.comic_x = 200; g.comic_y = 200;

        Enemy e{}; e.behavior = GameConstants::ENEMY_BEHAVIOR_BOUNCE; e.x = 200; e.y = 200;
        g.enemies.clear(); g.enemies.push_back(e);

        Input input;
        g.update(input);
        if (!g.game_over) { std::cerr << "Game over test: expected game_over true when lives exhausted\n"; return 1; }
    }

    std::cout << "Player respawn/death tests passed" << std::endl;
    return 0;
}
