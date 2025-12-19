#pragma once

#include "GameState.h"

namespace fireball {

// Handle a single fireball slot (index) for one tick. Returns true if a collision
// with the provided enemy occurred (and the fireball was deactivated).
bool handleFireballOnce(GameState& gs, size_t index, Enemy& maybeEnemy);

} // namespace fireball
