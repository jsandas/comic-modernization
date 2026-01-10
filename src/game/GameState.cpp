#include "GameState.h"
#include "../assets/MapLoader.h"
#include "../input/Input.h"
#include <iostream>

static const struct LevelDescriptor {
    const char* tileset;
    const char* pt0;
    const char* pt1;
    const char* pt2;
} level_table[] = {
    { "lake.tt2",  "lake0.pt",  "lake1.pt",  "lake2.pt" },
    { "forest.tt2","forest0.pt","forest1.pt","forest2.pt" },
    { "space.tt2", "space0.pt", "space1.pt", "space2.pt" },
    { "comp.tt2",  "comp0.pt",  "comp1.pt",  "comp2.pt" },
    { "cave.tt2",  "cave0.pt", "cave1.pt", "cave2.pt" },
    { "shed.tt2",  "shed0.pt", "shed1.pt", "shed2.pt" },
    { "castle.tt2","castle0.pt","castle1.pt","castle2.pt" },
    { "base.tt2",  "base0.pt",  "base1.pt",  "base2.pt" }
};

bool GameState::loadLevel(int level_number, const std::filesystem::path& dataPath) {
    if (level_number < 0 || level_number >= static_cast<int>(sizeof(level_table)/sizeof(level_table[0]))) {
        std::cerr << "Invalid level number: " << level_number << std::endl;
        return false;
    }

    const LevelDescriptor& desc = level_table[level_number];
    MapLoader loader(dataPath);

    // For now, load the pt0 of the level
    TileMap m = loader.loadTileMap(desc.pt0, desc.tileset);
    current_map = std::make_unique<TileMap>(m);

    // Reset camera and player position
    camera_x = 0;
    comic_facing = 1;

    // Choose spawn position: find a column near the left where there's a ground tile with empty space above it
    const int player_w = GameConstants::PLAYER_WIDTH_TILES * GameConstants::TILE_SIZE;
    const int player_h = GameConstants::PLAYER_HEIGHT_TILES * GameConstants::TILE_SIZE;
    int chosen_tx = -1;
    int ground_row = -1;
    int tiles_h = GameConstants::SCREEN_HEIGHT_TILES;
    int tiles_w = GameConstants::SCREEN_WIDTH_TILES;

    // Search entire width for a ground edge (empty above, solid below)
    for (int tx = 0; tx < tiles_w; ++tx) {
        for (int ty = 1; ty < tiles_h; ++ty) {
            int idx = ty * tiles_w + tx;
            int above = (ty - 1) * tiles_w + tx;
            if (idx >= 0 && idx < static_cast<int>(current_map->solidity.size()) &&
                above >= 0 && above < static_cast<int>(current_map->solidity.size())) {
                if (current_map->solidity[idx] && !current_map->solidity[above]) {
                    chosen_tx = tx;
                    ground_row = ty;
                    break;
                }
            }
        }
        if (chosen_tx >= 0) break;
    }

    if (chosen_tx >= 0) {
        // Offset spawn a couple pixels to avoid immediate horizontal collision with adjacent tiles
        comic_x = chosen_tx * GameConstants::TILE_SIZE + 1;
        // Try to place player at the first y where player sits on ground (not overlapping)
        int maxY = GameConstants::SCREEN_HEIGHT - player_h;
        bool placed = false;
        for (int y = 0; y <= maxY; ++y) {
            // check vertical space of player's height
            if (!isRectSolid(comic_x, y, player_w, player_h) && isRectSolid(comic_x, y + 1, player_w, player_h)) {
                comic_y = y;
                placed = true;
                break;
            }
        }
        if (!placed) {
            // fallback to ground_row alignment
            comic_y = ground_row * GameConstants::TILE_SIZE - player_h;
            if (comic_y < 0) comic_y = 0;
        }
    } else {
        // No suitable ground edge was found; search for any column with a solid tile and place player above it
        int found_tx = -1;
        int found_ty = -1;
        for (int tx = 0; tx < tiles_w; ++tx) {
            for (int ty = tiles_h - 1; ty >= 0; --ty) {
                if (isSolidTileAt(tx, ty)) {
                    found_tx = tx;
                    found_ty = ty;
                    break;
                }
            }
            if (found_tx >= 0) break;
        }

        if (found_tx >= 0) {
            comic_x = found_tx * GameConstants::TILE_SIZE + 1;
            comic_y = found_ty * GameConstants::TILE_SIZE - player_h;
            if (comic_y < 0) comic_y = 0;
        } else {
            // final fallback: sensible default near bottom (add small offset to avoid immediate wall collision)
            comic_x = 16 + 1;
            comic_y = GameConstants::SCREEN_HEIGHT - player_h - 16;
        }

        // Drop down until standing on ground or until bottom
        int maxY = GameConstants::SCREEN_HEIGHT - player_h;
        for (int y = comic_y; y <= maxY; ++y) {
            if (isRectSolid(comic_x, y + 1, player_w, player_h)) {
                comic_y = y;
                break;
            }
        }
    }

    int maxY = GameConstants::SCREEN_HEIGHT - player_h;
    if (!isOnGround()) {
        bool found_ground = false;
        // Search downward from current y to bottom
        for (int y = comic_y; y <= maxY; ++y) {
            if (y + 1 <= maxY && !isRectSolid(comic_x, y, player_w, player_h) && isRectSolid(comic_x, y + 1, player_w, player_h)) {
                comic_y = y;
                found_ground = true;
                break;
            }
        }
        // If not found, search upward
        if (!found_ground) {
            for (int y = comic_y; y >= 0; --y) {
                if (y + 1 <= maxY && !isRectSolid(comic_x, y, player_w, player_h) && isRectSolid(comic_x, y + 1, player_w, player_h)) {
                    comic_y = y;
                    found_ground = true;
                    break;
                }
            }
        }
        // Final fallback: clamp to range
        if (!found_ground) {
            if (comic_y < 0) comic_y = 0;
            if (comic_y > maxY) comic_y = maxY;
        }
    }




    return true;
}

// Collision helpers
bool GameState::isSolidTileAt(int tx, int ty) const {
    if (!current_map) return false;
    if (tx < 0 || ty < 0 || tx >= GameConstants::SCREEN_WIDTH_TILES || ty >= GameConstants::SCREEN_HEIGHT_TILES) return false;
    int idx = ty * GameConstants::SCREEN_WIDTH_TILES + tx;
    return current_map->solidity[idx] != 0;
}

bool GameState::isRectSolid(int px, int py, int w, int h) const {
    if (!current_map) return false;
    int left = px / GameConstants::TILE_SIZE;
    int right = (px + w - 1) / GameConstants::TILE_SIZE;
    int top = py / GameConstants::TILE_SIZE;
    int bottom = (py + h - 1) / GameConstants::TILE_SIZE;
    for (int ty = top; ty <= bottom; ++ty) {
        for (int tx = left; tx <= right; ++tx) {
            if (isSolidTileAt(tx, ty)) return true;
        }
    }
    return false;
}

bool GameState::isOnGround() const {
    // Check one pixel below bottom of player
    int player_h = GameConstants::PLAYER_HEIGHT_TILES * GameConstants::TILE_SIZE;
    int foot_y = comic_y + player_h;
    int left = comic_x / GameConstants::TILE_SIZE;
    int right = (comic_x + GameConstants::PLAYER_WIDTH_TILES * GameConstants::TILE_SIZE - 1) / GameConstants::TILE_SIZE;
    int ty = foot_y / GameConstants::TILE_SIZE;
    for (int tx = left; tx <= right; ++tx) {
        if (isSolidTileAt(tx, ty)) return true;
    }
    return false;
}

void GameState::update(const Input& input) {
    // Simple player physics and collision
    const int player_w = GameConstants::PLAYER_WIDTH_TILES * GameConstants::TILE_SIZE;
    const int player_h = GameConstants::PLAYER_HEIGHT_TILES * GameConstants::TILE_SIZE;

    // Horizontal input — disabled during invulnerability/recovery
    if (comic_invuln_ticks == 0) {
        if (input.moveLeft()) {
            comic_x_vel = -2;
            comic_facing = 0;
        } else if (input.moveRight()) {
            comic_x_vel = 2;
            comic_facing = 1;
        } else {
            // no horizontal input; keep current velocity so physics-driven effects (e.g., knockback)
            // continue to be applied and friction will decay the velocity over time
        }
    }


    // Jump
    if (comic_invuln_ticks == 0 && input.jump() && isOnGround()) {
        comic_y_vel = -6;
    }

    // Gravity
    int new_y_vel = comic_y_vel + 1; // gravity per tick
    if (new_y_vel > 8) new_y_vel = 8;
    comic_y_vel = static_cast<int16_t>(new_y_vel);

    // Move horizontally and apply collision
    int target_x = comic_x + comic_x_vel;
    if (!isRectSolid(target_x, comic_y, player_w, player_h)) {
        comic_x = target_x;
    } else {
        // Slide against wall: compute the tile index from the *attempted* target position so
        // we place the player just outside the tile they tried to enter (avoids overshooting).
        if (comic_x_vel > 0) {
            int tx = (target_x + player_w) / GameConstants::TILE_SIZE;
            comic_x = tx * GameConstants::TILE_SIZE - player_w - 1;
        } else if (comic_x_vel < 0) {
            int tx = target_x / GameConstants::TILE_SIZE;
            comic_x = (tx + 1) * GameConstants::TILE_SIZE + 1;
        }
        comic_x_vel = 0;
    }

    // Move vertically and apply collision
    int target_y = comic_y + comic_y_vel;
    if (!isRectSolid(comic_x, target_y, player_w, player_h)) {
        comic_y = target_y;
    } else {
        if (comic_y_vel > 0) {
            // landed on ground: align bottom to just above ground tile
            int ty = (target_y + player_h) / GameConstants::TILE_SIZE;
            comic_y = ty * GameConstants::TILE_SIZE - player_h;
        } else if (comic_y_vel < 0) {
            // hit ceiling: place just below ceiling
            int ty = target_y / GameConstants::TILE_SIZE;
            comic_y = (ty + 1) * GameConstants::TILE_SIZE;
        }
        comic_y_vel = 0;
    }

    // Keep camera centered-ish
    int half_screen = GameConstants::SCREEN_WIDTH / 2;
    camera_x = comic_x - half_screen;
    if (camera_x < 0) camera_x = 0;
    if (camera_x > (GameConstants::SCREEN_WIDTH_TILES * GameConstants::TILE_SIZE - GameConstants::SCREEN_WIDTH)) {
        camera_x = GameConstants::SCREEN_WIDTH_TILES * GameConstants::TILE_SIZE - GameConstants::SCREEN_WIDTH;
    }

    // Friction: decay horizontal velocity toward zero (applies during knockback/recovery)
    if (comic_x_vel > 0) comic_x_vel = static_cast<int16_t>(comic_x_vel - 1);
    else if (comic_x_vel < 0) comic_x_vel = static_cast<int16_t>(comic_x_vel + 1);

    // Minimal enemy AI update (Phase 3 stub implementations)
    // Each enemy is treated as a rectangle of size 1 tile x 1 tile for collision purposes.
    const int enemy_w = GameConstants::TILE_SIZE;
    const int enemy_h = GameConstants::TILE_SIZE;
    for (auto &e : enemies) {
        if (e.spawn_timer > 0) {
            if (e.spawn_timer > 0) --e.spawn_timer;
            continue;
        }

        int ex = static_cast<int>(e.x);
        int ey = static_cast<int>(e.y);

        // Movement helpers: perform per-pixel stepping to avoid tunneling at high speeds
        auto stepHorizontal = [&](Enemy &en, int &px, int py) {
            int8_t vx = en.x_vel;
            if (vx == 0) return;
            int dir = (vx > 0) ? 1 : -1;
            int steps = std::abs(vx);
            for (int s = 0; s < steps; ++s) {
                if (!isRectSolid(px + dir, py, enemy_w, enemy_h)) {
                    px += dir;
                } else {
                    // collision on this step: stop and indicate collision
                    en.x_vel = -en.x_vel; // default behavior is to reverse; caller can adjust
                    return;
                }
            }
        };

        auto stepHorizontalNoReverse = [&](Enemy &en, int &px, int py) {
            int8_t vx = en.x_vel;
            if (vx == 0) return;
            int dir = (vx > 0) ? 1 : -1;
            int steps = std::abs(vx);
            for (int s = 0; s < steps; ++s) {
                if (!isRectSolid(px + dir, py, enemy_w, enemy_h)) {
                    px += dir;
                } else {
                    // blocked: stop moving and clear velocity
                    en.x_vel = 0;
                    return;
                }
            }
        };

        auto stepVertical = [&](Enemy &en, int &py, int px) {
            int8_t vy = en.y_vel;
            if (vy == 0) return;
            int dir = (vy > 0) ? 1 : -1;
            int steps = std::abs(vy);
            for (int s = 0; s < steps; ++s) {
                if (!isRectSolid(px, py + dir, enemy_w, enemy_h)) {
                    py += dir;
                } else {
                    // collision: align and zero velocity
                    if (dir > 0) {
                        int ty = (py + dir + enemy_h) / GameConstants::TILE_SIZE;
                        py = ty * GameConstants::TILE_SIZE - enemy_h;
                    } else {
                        int ty = (py + dir) / GameConstants::TILE_SIZE;
                        py = (ty + 1) * GameConstants::TILE_SIZE;
                    }
                    en.y_vel = 0;
                    return;
                }
            }
        };

        switch (e.behavior) {
            case GameConstants::ENEMY_BEHAVIOR_BOUNCE: {
                // Simple horizontal patrol that reverses when hitting a wall
                if (e.x_vel == 0) e.x_vel = 1;
                stepHorizontal(e, ex, ey);
                e.x = static_cast<uint8_t>(ex);
                break;
            }
            case GameConstants::ENEMY_BEHAVIOR_LEAP: {
                // If state==0, initiate a jump; otherwise use state as cooldown
                if (e.state == 0) {
                    e.y_vel = -6;
                    e.state = 12; // cooldown ticks
                } else if (e.state > 0) {
                    --e.state;
                }
                // Apply vertical movement with simple gravity
                int new_y_vel = static_cast<int>(e.y_vel) + 1;
                if (new_y_vel > 8) new_y_vel = 8;
                e.y_vel = static_cast<int8_t>(new_y_vel);
                stepVertical(e, ey, ex);
                e.y = static_cast<uint8_t>(ey);
                break;
            }
            case GameConstants::ENEMY_BEHAVIOR_ROLL: {
                // Continuous horizontal movement; reverse on obstacle similar to bounce
                if (e.x_vel == 0) e.x_vel = 2;
                stepHorizontal(e, ex, ey);
                e.x = static_cast<uint8_t>(ex);
                break;
            }
            case GameConstants::ENEMY_BEHAVIOR_SEEK: {
                // Move horizontally toward player
                if (comic_x > ex) e.x_vel = 1;
                else if (comic_x < ex) e.x_vel = -1;
                else e.x_vel = 0;
                stepHorizontalNoReverse(e, ex, ey);
                e.x = static_cast<uint8_t>(ex);
                break;
            }
            case GameConstants::ENEMY_BEHAVIOR_SHY: {
                // If player is within 32 pixels, move away horizontally
                int dx = comic_x - ex;
                if (std::abs(dx) < 32) {
                    e.x_vel = (dx > 0) ? -1 : 1;
                    stepHorizontalNoReverse(e, ex, ey);
                    e.x = static_cast<uint8_t>(ex);
                } else {
                    e.x_vel = 0;
                }
                break;
            }
            default:
                break;
        }
    }

    // Enemy-player collision and enemy-enemy overlap resolution
    // Apply invulnerability ticks for the player
    if (comic_invuln_ticks > 0) --comic_invuln_ticks;

    // 1) Enemy-player collisions: decrement HP when overlapping and not invulnerable
    auto rects_overlap = [](int ax, int ay, int aw, int ah, int bx, int by, int bw, int bh) {
        return !(ax + aw - 1 < bx || bx + bw - 1 < ax || ay + ah - 1 < by || by + bh - 1 < ay);
    };

    for (auto &e : enemies) {
        int ex = static_cast<int>(e.x);
        int ey = static_cast<int>(e.y);
        bool overlap = rects_overlap(comic_x, comic_y, player_w, player_h, ex, ey, GameConstants::TILE_SIZE, GameConstants::TILE_SIZE);
        if (comic_invuln_ticks == 0 && overlap) {
            if (comic_hp > 0) --comic_hp;

            // Apply knockback: push player away from the enemy and give a small upward kick
            int px = comic_x;
            int ex_px = ex;
            if (ex_px > px) {
                comic_x_vel = -4;
            } else if (ex_px < px) {
                comic_x_vel = 4;
            } else {
                // Enemy exactly overlapping horizontally; push upward only
                comic_x_vel = 0;
            }
            comic_y_vel = -4;

            // Set brief invulnerability after taking damage
            comic_invuln_ticks = 10;

            // If HP has dropped to zero, either respawn (if lives remain) or set game_over
            if (comic_hp == 0) {
                if (comic_num_lives > 0) {
                    // consume a life and respawn at sensible default
                    --comic_num_lives;
                    comic_hp = GameConstants::MAX_HP;
                    const int player_h = GameConstants::PLAYER_HEIGHT_TILES * GameConstants::TILE_SIZE;
                    comic_x = 16 + 1;
                    comic_y = GameConstants::SCREEN_HEIGHT - player_h - 16;
                    comic_x_vel = 0;
                    comic_y_vel = 0;
                    comic_invuln_ticks = 60; // longer recovery on respawn
                    game_over = false;
                } else {
                    game_over = true;
                }
            }
        }
    }

    // 2) Enemy-enemy overlap: if two enemies overlap, reverse their x velocities to separate
    for (size_t i = 0; i < enemies.size(); ++i) {
        for (size_t j = i + 1; j < enemies.size(); ++j) {
            auto &a = enemies[i];
            auto &b = enemies[j];
            int ax = static_cast<int>(a.x);
            int ay = static_cast<int>(a.y);
            int bx = static_cast<int>(b.x);
            int by = static_cast<int>(b.y);
            if (rects_overlap(ax, ay, GameConstants::TILE_SIZE, GameConstants::TILE_SIZE, bx, by, GameConstants::TILE_SIZE, GameConstants::TILE_SIZE)) {
                a.x_vel = -a.x_vel;
                b.x_vel = -b.x_vel;
            }
        }
    }
}
