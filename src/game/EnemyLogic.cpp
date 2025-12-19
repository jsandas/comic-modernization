#include "EnemyLogic.h"

namespace enemy {

void forceEnemyLeap(Enemy& e, int player_x) {
    // Match the simplified ASM harness behavior: x_vel = (e.x >= player_x) ? 1 : -1; y_vel = -7
    e.x_vel = (e.x >= player_x) ? 1 : -1;
    e.y_vel = -7;
}

} // namespace enemy
