#pragma once

#include "GameState.h"

namespace enemy {

// Force an enemy to perform a leap (deterministic test helper).
// Sets enemy.x_vel to +1 or -1 depending on enemy.x vs player_x and sets y_vel to -7.
void forceEnemyLeap(Enemy& e, int player_x);

} // namespace enemy
