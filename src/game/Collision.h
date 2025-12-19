#pragma once

#include "GameState.h"

namespace collision {

// Check vertical collision for an enemy-sized box at pixel coordinates (x, y).
// Returns true if any overlapped tile is solid.
bool checkVerticalEnemyMapCollision(const TileMap& map, int x, int y);

// Check horizontal collision for an enemy-sized box at pixel coordinates (x, y).
// Returns true if any overlapped tile is solid.
bool checkHorizontalEnemyMapCollision(const TileMap& map, int x, int y);

} // namespace collision
