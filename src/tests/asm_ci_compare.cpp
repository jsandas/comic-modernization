#include <cstdio>
#include <cstdlib>
#include <string>
#include <array>
#include <tuple>

#include "game/GameState.h"
#include "game/Collision.h"
#include "game/EnemyLogic.h"
#include "game/FireballLogic.h"

using namespace collision;

static int run_asm_raw(const std::string& cmd) {
    FILE* f = popen(cmd.c_str(), "r");
    if(!f) return -1;
    int a=-1;
    // read whole line(s)
    if(fscanf(f, "%d", &a) < 1) a = -1;
    pclose(f);
    return a;
}

static std::pair<int,int> run_asm_enemy(int en_x, int en_y, int pl_x, int pl_y) {
    std::string script = std::string(PROJECT_ROOT) + "/src/tests/asm/run_asm_unicorn.py";
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "python3 %s %d --enemy-x %d --enemy-y %d --player-x %d --player-y %d",
             script.c_str(), 2, en_x, en_y, pl_x, pl_y);
    FILE* f = popen(cmd, "r");
    if(!f) return {127,127};
    int x=-128,y=-128;
    if(fscanf(f, "%d %d", &x, &y) < 2) { pclose(f); return {127,127}; }
    pclose(f);
    // interpret signed bytes
    if(x & 0x80) x -= 0x100;
    if(y & 0x80) y -= 0x100;
    return {x,y};
}

static std::tuple<int,int,int> run_asm_fireball(int fb_x, int fb_y, int fb_vel, int en_x, int en_y) {
    std::string script = std::string(PROJECT_ROOT) + "/src/tests/asm/run_asm_unicorn.py";
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "python3 %s %d --fb-x %d --fb-y %d --fb-vel %d --enemy-x %d --enemy-y %d",
             script.c_str(), 3, fb_x, fb_y, fb_vel, en_x, en_y);
    FILE* f = popen(cmd, "r");
    if(!f) return {-1,-1,-1};
    int fx=-1,fy=-1,active=-1;
    if(fscanf(f, "%d %d %d", &fx, &fy, &active) < 3) { pclose(f); return {-1,-1,-1}; }
    pclose(f);
    return {fx,fy,active};
}

int main() {
    int fail = 0;

    // 1) Vertical collision
    {
        GameState gs;
        int tx=2, ty=2;
        gs.current_map->solidity[ty * GameConstants::SCREEN_WIDTH_TILES + tx] = 1;
        int x = tx * GameConstants::TILE_SIZE + 1;
        int y = ty * GameConstants::TILE_SIZE + 1;
        bool cpp_res = checkVerticalEnemyMapCollision(*gs.current_map, x, y);
        std::string script = std::string(PROJECT_ROOT) + "/src/tests/asm/run_asm_unicorn.py";
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "python3 %s %d --x %d --y %d --set-tile %d %d 1",
                 script.c_str(), 0, x/8, y/8, tx, ty);
        int asm_res = run_asm_raw(cmd);
        bool asm_bool = (asm_res != 0);
        if(cpp_res != asm_bool) {
            fprintf(stderr, "Mismatch VERT: cpp=%d asm=%d (cmd=%s)\n", (int)cpp_res, asm_res, cmd);
            fail = 1;
        } else {
            fprintf(stdout, "OK VERT: cpp=%d asm=%d\n", (int)cpp_res, asm_res);
        }
    }

    // 2) Horizontal collision
    {
        GameState gs;
        int tx=4, ty=3;
        gs.current_map->solidity[ty * GameConstants::SCREEN_WIDTH_TILES + tx] = 1;
        int x = tx * GameConstants::TILE_SIZE - GameConstants::TILE_SIZE/2;
        int y = ty * GameConstants::TILE_SIZE + 2;
        bool cpp_res = checkHorizontalEnemyMapCollision(*gs.current_map, x, y);
        std::string script = std::string(PROJECT_ROOT) + "/src/tests/asm/run_asm_unicorn.py";
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "python3 %s %d --x %d --y %d --set-tile %d %d 1",
                 script.c_str(), 1, x/8, y/8, tx, ty);
        int asm_res = run_asm_raw(cmd);
        bool asm_bool = (asm_res != 0);
        if(cpp_res != asm_bool) {
            fprintf(stderr, "Mismatch HORIZ: cpp=%d asm=%d (cmd=%s)\n", (int)cpp_res, asm_res, cmd);
            fail = 2;
        } else {
            fprintf(stdout, "OK HORIZ: cpp=%d asm=%d\n", (int)cpp_res, asm_res);
        }
    }

    // 3) Enemy leap
    {
        int ex=50, ey=10, px=40, py=0;
        Enemy e;
        e.x = ex; e.y = ey; e.x_vel = 0; e.y_vel = 0;
        enemy::forceEnemyLeap(e, px);
        auto [ax, ay] = run_asm_enemy(ex, ey, px, py);
        if(ax==127 && ay==127) { fprintf(stderr, "ASM enemy harness failed to run\n"); fail |= 4; }
        if(e.x_vel != ax || e.y_vel != ay) {
            fprintf(stderr, "Mismatch ENEMY: cpp=(%d,%d) asm=(%d,%d)\n", (int)e.x_vel, (int)e.y_vel, ax, ay);
            fail |= 8;
        } else {
            fprintf(stdout, "OK ENEMY: cpp=(%d,%d) asm=(%d,%d)\n", (int)e.x_vel, (int)e.y_vel, ax, ay);
        }
    }

    // 4) Fireball tick
    {
        GameState gs;
        Enemy en; en.x = 11; en.y = 10;
        Fireball fb; fb.x = 10; fb.y = 10; fb.x_vel = 1; fb.y_vel = 0;
        gs.fireballs[0] = fb;
        bool collided = fireball::handleFireballOnce(gs, 0, en);
        auto [fx,fy,active] = run_asm_fireball(10,10,1,11,10);
        if(fx==-1) { fprintf(stderr, "ASM fireball harness failed to run\n"); fail |= 16; }
        int asm_active = (active == 1) ? 1 : 0;
        if(collided && asm_active != 0) {
            fprintf(stderr, "Mismatch FIREBALL: cpp collided but asm active=1\n"); fail |= 32;
        } else if(!collided && asm_active == 0) {
            fprintf(stderr, "Mismatch FIREBALL: cpp not collided but asm active=0\n"); fail |= 64;
        } else {
            fprintf(stdout, "OK FIREBALL: cpp_collided=%d asm_active=%d\n", (int)collided, asm_active);
        }
    }

    return fail;
}
