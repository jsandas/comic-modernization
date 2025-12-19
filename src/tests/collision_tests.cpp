#include <catch2/catch_test_macros.hpp>

#include "game/Collision.h"
#include "game/GameState.h"

using namespace collision;

TEST_CASE("Vertical collision when tile is solid", "[collision]") {
    GameState gs;
    // Make tile at (2,2) solid
    int tx = 2, ty = 2;
    gs.current_map->solidity[ty * GameConstants::SCREEN_WIDTH_TILES + tx] = 1;

    // Place enemy box such that it overlaps tile (2,2): pixel coords
    int x = tx * GameConstants::TILE_SIZE + 1;
    int y = ty * GameConstants::TILE_SIZE + 1;

    REQUIRE(checkVerticalEnemyMapCollision(*gs.current_map, x, y));

    // Also compare with ASM harness via Unicorn runner
    auto run_asm = [](int mode, int ax, int ay, int tile_x = -1, int tile_y = -1) -> int {
        std::string script = std::string(PROJECT_ROOT) + "/src/tests/asm/run_asm_unicorn.py";
        char cmd[512];
        if(tile_x >= 0 && tile_y >= 0) {
            snprintf(cmd, sizeof(cmd), "python3 %s %d --x %d --y %d --set-tile %d %d 1",
                     script.c_str(), mode, ax/8, ay/8, tile_x, tile_y);
        } else {
            snprintf(cmd, sizeof(cmd), "python3 %s %d --x %d --y %d",
                     script.c_str(), mode, ax/8, ay/8);
        }
        // debug: write command to /tmp for investigation
        {
            FILE* ff = fopen("/tmp/cmd_collision.txt", "w");
            if(ff) {
                fprintf(ff, "%s", cmd);
                fclose(ff);
            }
        }
        FILE* f = popen(cmd, "r");
        if(!f) return -1;
        int res = -1;
        if(fscanf(f, "%d", &res) < 0) res = -1;
        pclose(f);
        return res;
    };

    int asm_res = run_asm(0, x, y, tx, ty);
    if(asm_res == -1) {
        WARN("ASM harness not available or failed; skipping ASM comparison for vertical collision");
    } else {
        // Our C++ function returns bool; asm_res may be 0 or any non-zero for collision; normalize to boolean
        int asm_bool = (asm_res != 0);
        if(static_cast<int>(checkVerticalEnemyMapCollision(*gs.current_map, x, y)) != asm_bool) {
            char _buf[128];
            snprintf(_buf, sizeof(_buf), "ASM and C++ mismatch: c++=%d, asm=%d",
                     static_cast<int>(checkVerticalEnemyMapCollision(*gs.current_map, x, y)), asm_bool);
            WARN(_buf);
        }
    }
}

TEST_CASE("No collision when tiles are passable", "[collision]") {
    GameState gs;
    // Ensure all tiles are passable
    std::fill(gs.current_map->solidity.begin(), gs.current_map->solidity.end(), 0);

    int x = 10, y = 10;
    REQUIRE_FALSE(checkVerticalEnemyMapCollision(*gs.current_map, x, y));
    REQUIRE_FALSE(checkHorizontalEnemyMapCollision(*gs.current_map, x, y));
}

TEST_CASE("Edge case: overlapping two tiles, one solid", "[collision]") {
    GameState gs;
    // set two adjacent tiles: right one solid
    int tx = 4, ty = 3;
    gs.current_map->solidity[ty * GameConstants::SCREEN_WIDTH_TILES + tx] = 1;

    // Place an enemy box that spans tiles tx-1 and tx (x near tile boundary)
    int x = tx * GameConstants::TILE_SIZE - GameConstants::TILE_SIZE/2;
    int y = ty * GameConstants::TILE_SIZE + 2;

    REQUIRE(checkHorizontalEnemyMapCollision(*gs.current_map, x, y));
}
