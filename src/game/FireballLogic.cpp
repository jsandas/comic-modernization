#include "FireballLogic.h"
#include "Constants.h"

namespace fireball {

bool handleFireballOnce(GameState& gs, size_t index, Enemy& maybeEnemy) {
    if(index >= gs.fireballs.size()) return false;
    auto& fb = gs.fireballs[index];
    // FIREBALL_DEAD is 0xff in the ASM; consider x==0xff as dead
    if(fb.x == 0xff && fb.y == 0xff) return false;

    // move horizontally
    fb.x = static_cast<uint8_t>(static_cast<int>(fb.x) + fb.x_vel);
    // simple vertical corkscrew emulation: move by sign of x_vel
    fb.y = static_cast<uint8_t>(static_cast<int>(fb.y) + (fb.x_vel >= 0 ? 1 : -1));

    // Check collision with maybeEnemy (simple AABB: within 1 unit)
    int dx = static_cast<int>(fb.x) - static_cast<int>(maybeEnemy.x);
    if(dx < 0) dx = -dx;
    int dy = static_cast<int>(fb.y) - static_cast<int>(maybeEnemy.y);
    if(dy < 0) dy = -dy;
    if(dx <= 1 && dy <= 1) {
        fb.x = 0xff;
        fb.y = 0xff;
        return true;
    }

    // bounds: if off-screen, deactivate
    if(static_cast<int>(fb.x) < 0 || static_cast<int>(fb.x) >= GameConstants::SCREEN_WIDTH_TILES) {
        fb.x = 0xff; fb.y = 0xff; return false;
    }
    return false;
}

} // namespace fireball
