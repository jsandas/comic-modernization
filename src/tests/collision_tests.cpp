#include <iostream>
#include <SDL2/SDL.h>
#include "game/GameState.h"
#include "input/Input.h"

int main() {
    GameState gs;

    // Prepare a simple TileMap and set a single solid tile
    TileMap m;
    const int tx = 5;
    const int ty = 3;
    int idx = ty * GameConstants::SCREEN_WIDTH_TILES + tx;
    m.solidity[idx] = 1;

    gs.current_map = std::make_unique<TileMap>(m);

    // Basic isSolidTileAt
    if (!gs.isSolidTileAt(tx, ty)) {
        std::cerr << "isSolidTileAt failed: expected true at (" << tx << "," << ty << ")\n";
        return 1;
    }
    if (gs.isSolidTileAt(-1, 0)) {
        std::cerr << "isSolidTileAt failed: negative index should be false\n";
        return 1;
    }

    // isRectSolid tests
    int player_w = GameConstants::PLAYER_WIDTH_TILES * GameConstants::TILE_SIZE;
    int player_h = GameConstants::PLAYER_HEIGHT_TILES * GameConstants::TILE_SIZE;

    // Rectangle that overlaps the solid tile
    int px = tx * GameConstants::TILE_SIZE;
    int py = ty * GameConstants::TILE_SIZE;
    if (!gs.isRectSolid(px, py, player_w, player_h)) {
        std::cerr << "isRectSolid failed: expected overlap to be solid\n";
        return 1;
    }

    // Rectangle just above the tile should not be solid
    if (gs.isRectSolid(px, py - player_h, player_w, player_h)) {
        std::cerr << "isRectSolid failed: rectangle above should not be solid\n";
        return 1;
    }

    // isOnGround: place player so bottom is exactly on tile ty
    gs.comic_x = px;
    gs.comic_y = py - player_h;
    if (!gs.isOnGround()) {
        std::cerr << "isOnGround failed: expected true when player rests on tile below\n";
        return 1;
    }

    // --- Wall slide / resolution tests ---
    // Place player to the left of a solid tile and attempt to move right into it
    {
        Input input;
        GameState g;
        g.current_map = std::make_unique<TileMap>(*gs.current_map);
        // place player vertically so it overlaps tile row ty
        int py_for_wall = ty * GameConstants::TILE_SIZE - (player_h - GameConstants::TILE_SIZE);
        g.comic_y = py_for_wall;
        g.comic_x = tx * GameConstants::TILE_SIZE - player_w - 1; // 1px gap

        SDL_Event ev;
        ev.type = SDL_KEYDOWN;
        ev.key.keysym.scancode = SDL_SCANCODE_RIGHT;
        input.handleEvent(ev);

        g.update(input);

        int expected_x = tx * GameConstants::TILE_SIZE - player_w - 1;
        if (g.comic_x != expected_x) {
            std::cerr << "Wall-slide-right failed: expected x=" << expected_x << " got=" << g.comic_x << "\n";
            return 1;
        }
    }

    // Place player to the right of a solid tile and attempt to move left into it
    {
        Input input;
        GameState g;
        g.current_map = std::make_unique<TileMap>(*gs.current_map);
        int py_for_wall = ty * GameConstants::TILE_SIZE - (player_h - GameConstants::TILE_SIZE);
        g.comic_y = py_for_wall;
        g.comic_x = (tx + 1) * GameConstants::TILE_SIZE + 1; // to the right, small offset

        SDL_Event ev;
        ev.type = SDL_KEYDOWN;
        ev.key.keysym.scancode = SDL_SCANCODE_LEFT;
        input.handleEvent(ev);

        g.update(input);

        int expected_x = (tx + 1) * GameConstants::TILE_SIZE + 1;
        if (g.comic_x != expected_x) {
            std::cerr << "Wall-slide-left failed: expected x=" << expected_x << " got=" << g.comic_x << "\n";
            return 1;
        }
    }

    // --- Diagonal corner tests (down-right and down-left) ---
    // Scenario: player moves horizontally into a tile while concurrently falling; final position must not overlap solids
    {
        Input input;
        GameState g;
        g.current_map = std::make_unique<TileMap>(*gs.current_map);

        // Place a solid tile at bottom-right of the test column
        int corner_tx = tx + 1;
        int corner_ty = ty + 1;
        int corner_idx = corner_ty * GameConstants::SCREEN_WIDTH_TILES + corner_tx;
        g.current_map->solidity[corner_idx] = 1;

        // place player just left and slightly above so falling will attempt to intersect bottom-right
        int start_x = tx * GameConstants::TILE_SIZE - player_w - 1;
        int start_y = corner_ty * GameConstants::TILE_SIZE - player_h - (GameConstants::TILE_SIZE / 2);
        g.comic_x = start_x;
        g.comic_y = start_y;
        g.comic_y_vel = 4; // falling

        SDL_Event ev;
        ev.type = SDL_KEYDOWN;
        ev.key.keysym.scancode = SDL_SCANCODE_RIGHT;
        input.handleEvent(ev);

        g.update(input);

        if (g.isRectSolid(g.comic_x, g.comic_y, player_w, player_h)) {
            std::cerr << "Diagonal down-right collision failed: player overlaps solid after move\n";
            return 1;
        }
    }

    {
        Input input;
        GameState g;
        g.current_map = std::make_unique<TileMap>(*gs.current_map);

        // Place a solid tile at bottom-left of the test column
        int corner_tx = tx - 1;
        int corner_ty = ty + 1;
        int corner_idx = corner_ty * GameConstants::SCREEN_WIDTH_TILES + corner_tx;
        g.current_map->solidity[corner_idx] = 1;

        // place player just right and slightly above so falling will attempt to intersect bottom-left
        int start_x = (tx + 1) * GameConstants::TILE_SIZE + 1;
        int start_y = corner_ty * GameConstants::TILE_SIZE - player_h - (GameConstants::TILE_SIZE / 2);
        g.comic_x = start_x;
        g.comic_y = start_y;
        g.comic_y_vel = 4; // falling

        SDL_Event ev;
        ev.type = SDL_KEYDOWN;
        ev.key.keysym.scancode = SDL_SCANCODE_LEFT;
        input.handleEvent(ev);

        g.update(input);

        if (g.isRectSolid(g.comic_x, g.comic_y, player_w, player_h)) {
            std::cerr << "Diagonal down-left collision failed: player overlaps solid after move\n";
            return 1;
        }
    }

    // --- Multi-tile rectangle crossing test ---
    {
        GameState g;
        g.current_map = std::make_unique<TileMap>(*gs.current_map);

        // Make two adjacent solid tiles
        int tx0 = tx + 2;
        int ty0 = ty;
        int idx0 = ty0 * GameConstants::SCREEN_WIDTH_TILES + tx0;
        int idx1 = ty0 * GameConstants::SCREEN_WIDTH_TILES + (tx0 + 1);
        g.current_map->solidity[idx0] = 1;
        g.current_map->solidity[idx1] = 1;

        // Rectangle that spans the two tiles horizontally should be reported solid
        int rect_x = tx0 * GameConstants::TILE_SIZE;
        int rect_y = ty0 * GameConstants::TILE_SIZE;
        int rect_w = GameConstants::TILE_SIZE * 2;
        int rect_h = GameConstants::TILE_SIZE;

        if (!g.isRectSolid(rect_x, rect_y, rect_w, rect_h)) {
            std::cerr << "Multi-tile rect crossing failed: expected solid but got passable\n";
            return 1;
        }
    }

    // --- Wall-resolution alignment test ---
    {
        // Place a tall wall tile and attempt to step into it from varying offsets; ensure final x is aligned
        Input input;
        GameState g;
        g.current_map = std::make_unique<TileMap>(*gs.current_map);

        int wall_tx = tx + 4;
        int wall_ty = ty;
        int wall_idx = wall_ty * GameConstants::SCREEN_WIDTH_TILES + wall_tx;
        g.current_map->solidity[wall_idx] = 1;

        int py_for_wall = ty * GameConstants::TILE_SIZE - (player_h - GameConstants::TILE_SIZE);
        g.comic_y = py_for_wall;

        // Try starting offset so targetX will be inside wall
        g.comic_x = wall_tx * GameConstants::TILE_SIZE - player_w - 3; // 3px gap

        SDL_Event ev;
        ev.type = SDL_KEYDOWN;
        ev.key.keysym.scancode = SDL_SCANCODE_RIGHT;
        input.handleEvent(ev);

        g.update(input);

        int expected_x = wall_tx * GameConstants::TILE_SIZE - player_w - 1;
        if (g.comic_x != expected_x) {
            std::cerr << "Wall-resolution alignment failed: expected x=" << expected_x << " got=" << g.comic_x << "\n";
            return 1;
        }
    }

    // --- Overhang / jump-up (ceiling collision) test ---
    {
        GameState g;
        g.current_map = std::make_unique<TileMap>(*gs.current_map);

        int ceiling_tx = tx;
        int ceiling_ty = ty - 2; // a couple tiles above ground tile
        int ceiling_idx = ceiling_ty * GameConstants::SCREEN_WIDTH_TILES + ceiling_tx;
        g.current_map->solidity[ceiling_idx] = 1;

        // place player below the ceiling and give upward velocity (jump)
        g.comic_x = ceiling_tx * GameConstants::TILE_SIZE;
        g.comic_y = (ceiling_ty + 1) * GameConstants::TILE_SIZE; // just below the ceiling tile
        g.comic_y_vel = -12; // moving up

        Input input; // no horizontal input
        // Check target overlap
        int target_y = g.comic_y + g.comic_y_vel;
        if (!g.isRectSolid(g.comic_x, target_y, player_w, player_h)) {
            std::cerr << "Overhang test setup failed: expected target to overlap ceiling\n";
            return 1;
        }

        g.update(input);

        // After update, upward motion should be stopped
        if (g.comic_y_vel != 0) {
            std::cerr << "Overhang collision failed: expected y_vel to be 0 after hitting ceiling, got " << g.comic_y_vel << "\n";
            return 1;
        }

        // Player's top should be just below the ceiling tile
        if (g.comic_y < (ceiling_ty + 1) * GameConstants::TILE_SIZE) {
            std::cerr << "Overhang collision failed: player top is above ceiling bottom y=" << g.comic_y << "\n";
            return 1;
        }
    }

    // --- Narrow-gap passing (horizontal) test ---
    {
        GameState g;
        g.current_map = std::make_unique<TileMap>(*gs.current_map);

        // Create two vertical columns with a one-tile gap at tx=6
        int left_col = tx + 5;
        int right_col = tx + 7;
        int gap_tx = left_col + 1; // should be tx+6
        int gap_ty = ty; // same vertical position

        // Make solids spanning 3 tiles vertically at gap row
        for (int r = 0; r < 3; ++r) {
            g.current_map->solidity[(gap_ty + r) * GameConstants::SCREEN_WIDTH_TILES + left_col] = 1;
            g.current_map->solidity[(gap_ty + r) * GameConstants::SCREEN_WIDTH_TILES + right_col] = 1;
        }

        // Place player aligned vertically with gap and to the left of gap
        int py_for_gap = gap_ty * GameConstants::TILE_SIZE - (player_h - GameConstants::TILE_SIZE);
        g.comic_y = py_for_gap;
        g.comic_x = left_col * GameConstants::TILE_SIZE - player_w - 10; // start a bit further left so multiple ticks move in

        // Press right repeatedly until player enters gap or we timeout
        bool entered = false;
        for (int i = 0; i < 32; ++i) {
            Input input;
            SDL_Event ev;
            ev.type = SDL_KEYDOWN;
            ev.key.keysym.scancode = SDL_SCANCODE_RIGHT;
            input.handleEvent(ev);
            g.update(input);
            if (!g.isRectSolid(g.comic_x, g.comic_y, player_w, player_h) && g.comic_x >= gap_tx * GameConstants::TILE_SIZE - 1) {
                entered = true;
                break;
            }
            // clear input between ticks (simulate key release)
            ev.type = SDL_KEYUP;
            input.handleEvent(ev);
        }

        if (!entered) {
            std::cerr << "Narrow-gap passing failed: player did not pass into gap (x=" << g.comic_x << ")\n";
            return 1;
        }
    }

    // --- Spawn placement tests using sample maps ---
    {
        GameState g;
        std::filesystem::path dataPath = std::filesystem::current_path() / "reference" / "assets";
        if (!std::filesystem::exists(dataPath)) {
            std::cerr << "Spawn test skipped: assets path not found: " << dataPath << "\n";
        } else {
            // test a few levels
            std::vector<int> levels = { GameConstants::LEVEL_NUMBER_FOREST, GameConstants::LEVEL_NUMBER_CASTLE, GameConstants::LEVEL_NUMBER_LAKE };
            for (int lvl : levels) {
                GameState lg;
                if (!lg.loadLevel(lvl, dataPath)) {
                    std::cerr << "Spawn test failed: loadLevel returned false for level " << lvl << "\n";
                    return 1;
                }
                if (!lg.isOnGround()) {
                    std::cerr << "Spawn placement failed for level " << lvl << ": player not grounded after load\n";
                    return 1;
                }
                if (lg.comic_y < 0 || lg.comic_y > (GameConstants::SCREEN_HEIGHT - player_h)) {
                    std::cerr << "Spawn placement out of bounds for level " << lvl << ": y=" << lg.comic_y << "\n";
                    return 1;
                }
            }
        }
    }

    std::cout << "All collision tests passed\n";
    return 0;
}
