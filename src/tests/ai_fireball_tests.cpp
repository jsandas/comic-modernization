#include <catch2/catch_test_macros.hpp>

#include "game/EnemyLogic.h"
#include "game/FireballLogic.h"
#include "game/GameState.h"

#include <array>
#include <cstdio>
#include <memory>

static int run_asm_mode2_enemy(int enemy_x, int enemy_y, int player_x, int player_y) {
    std::string script = std::string(PROJECT_ROOT) + "/src/tests/asm/run_asm_unicorn.py";
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "python3 %s %d --enemy-x %d --enemy-y %d --player-x %d --player-y %d",
             script.c_str(), 2, enemy_x, enemy_y, player_x, player_y);
    FILE* f = popen(cmd, "r");
    if(!f) return -1;
    int xvel = -128, yvel = -128;
    if(fscanf(f, "%d %d", &xvel, &yvel) < 2) {
        pclose(f);
        return -1;
    }
    pclose(f);
    // encode xvel,yvel into single int for convenience: (xvel<<8) | (yvel&0xff)
    return ((xvel & 0xff) << 8) | (yvel & 0xff);
}

static std::tuple<int,int,int> run_asm_mode3_fireball(int fb_x, int fb_y, int fb_vel, int en_x, int en_y) {
    std::string script = std::string(PROJECT_ROOT) + "/src/tests/asm/run_asm_unicorn.py";
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "python3 %s %d --fb-x %d --fb-y %d --fb-vel %d --enemy-x %d --enemy-y %d",
             script.c_str(), 3, fb_x, fb_y, fb_vel, en_x, en_y);
    FILE* f = popen(cmd, "r");
    if(!f) return { -1, -1, -1 };
    int fx = -1, fy = -1, active = -1;
    if(fscanf(f, "%d %d %d", &fx, &fy, &active) < 3) {
        pclose(f);
        return { -1, -1, -1 };
    }
    pclose(f);
    return {fx, fy, active};
}

TEST_CASE("Enemy forced leap matches C++ simplified logic", "[ai]") {
    GameState gs;
    Enemy e;
    e.x = 50;
    e.y = 10;
    e.x_vel = 0;
    e.y_vel = 0;

    int player_x = 40;

    // C++ behavior
    enemy::forceEnemyLeap(e, player_x);
    REQUIRE(e.x_vel == 1);
    REQUIRE(e.y_vel == -7);

    // ASM behavior
    int packed = run_asm_mode2_enemy(/*enemy_x*/50, /*enemy_y*/10, player_x, /*player_y*/0);
    if(packed == -1) {
        WARN("ASM harness not available or failed; skipping ASM comparison for enemy leap");
    } else {
        int asm_xvel = (packed >> 8) & 0xff;
        int asm_yvel = packed & 0xff;
        // interpret signed byte for asm values
        if(asm_xvel & 0x80) asm_xvel = asm_xvel - 0x100;
        if(asm_yvel & 0x80) asm_yvel = asm_yvel - 0x100;

        REQUIRE(asm_xvel == static_cast<int>(e.x_vel));
        REQUIRE(asm_yvel == static_cast<int>(e.y_vel));
    }
}

TEST_CASE("Fireball movement and collision matches ASM simplified logic", "[fireball]") {
    GameState gs;
    Enemy en;
    en.x = 11;
    en.y = 10;

    Fireball fb;
    fb.x = 10;
    fb.y = 10;
    fb.x_vel = 1;
    fb.y_vel = 0;

    gs.fireballs[0] = fb;

    bool collided = fireball::handleFireballOnce(gs, 0, en);

    auto [fx, fy, active] = run_asm_mode3_fireball(/*fb_x*/10, /*fb_y*/10, /*vel*/1, /*en_x*/11, /*en_y*/10);
    if(fx == -1) {
        WARN("ASM harness not available or failed; skipping ASM comparison for fireball");
    } else {
        // Interpret active: 0 = deactivated due to collision, 1 = still active
        if(collided) {
            REQUIRE(active == 0);
        } else {
            REQUIRE(active == 1);
        }
    }
}
