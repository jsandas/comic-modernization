#include <iostream>
#include <SDL2/SDL.h>
#include "game/GameState.h"
#include "input/Input.h"

int main() {
    // Enemy hits player: HP decreases and invulnerability prevents immediate repeat damage
    {
        GameState g;
        g.current_map = std::make_unique<TileMap>();
        g.comic_hp = 6;
        g.comic_x = 100; g.comic_y = 100;

        Enemy e{}; e.behavior = GameConstants::ENEMY_BEHAVIOR_SEEK; e.x = 100; e.y = 100; e.x_vel = 0;
        g.enemies.clear(); g.enemies.push_back(e);
        // Keep player exactly overlapped so enemy stays still (seek will set x_vel=0 when aligned)
        g.comic_x = 100; g.comic_y = 100;

        Input input;
        g.update(input);
        // After first hit, comic_hp should have decreased
        if (g.comic_hp != 5) { std::cerr << "Enemy hit did not decrement HP (expected 5 got " << (int)g.comic_hp << ")\n"; return 1; }

        // Another update while invulnerable should not decrement further
        g.update(input);
        // While invulnerable, hp should not drop further
        if (g.comic_hp != 5) { std::cerr << "Invulnerability did not protect from repeated damage\n"; return 1; }

        // Fast-forward invulnerability and collide again by adding a second overlapping enemy
        g.comic_invuln_ticks = 0;
        Enemy e2{}; e2.behavior = GameConstants::ENEMY_BEHAVIOR_SEEK; e2.x = g.enemies[0].x; e2.y = g.enemies[0].y; e2.x_vel = 0;
        g.enemies.push_back(e2);
        // After clearing invulnerability and adding a second overlapping enemy, second update should decrement HP again
        g.update(input);
        if (g.comic_hp != 4) { std::cerr << "Second hit did not decrement HP as expected (got " << (int)g.comic_hp << ")\n"; return 1; }
    }

    // Multi-enemy overlap: two enemies moving into each other should reverse velocities rather than occupy same tile
    {
        GameState g;
        g.current_map = std::make_unique<TileMap>();
        Enemy a{}; a.behavior = GameConstants::ENEMY_BEHAVIOR_ROLL; a.x = 120; a.y = 80; a.x_vel = 2;
        Enemy b{}; b.behavior = GameConstants::ENEMY_BEHAVIOR_ROLL; b.x = 132; b.y = 80; b.x_vel = -2;
        g.enemies.clear(); g.enemies.push_back(a); g.enemies.push_back(b);

        Input input;
        g.update(input);
        // After update, if they would overlap, their velocities should have reversed
        if (!(g.enemies[0].x_vel < 0 && g.enemies[1].x_vel > 0)) {
            std::cerr << "Enemy-enemy overlap did not reverse velocities as expected\n";
            return 1;
        }
    }

    std::cout << "Enemy-player interaction tests passed" << std::endl;
    return 0;
}
