#include "Collision.h"
#include "Constants.h"

namespace collision {

static inline int tile_index_for(int tile_x, int tile_y) {
    if(tile_x < 0) tile_x = 0;
    if(tile_x >= GameConstants::SCREEN_WIDTH_TILES) tile_x = GameConstants::SCREEN_WIDTH_TILES - 1;
    if(tile_y < 0) tile_y = 0;
    if(tile_y >= GameConstants::SCREEN_HEIGHT_TILES) tile_y = GameConstants::SCREEN_HEIGHT_TILES - 1;
    return tile_y * GameConstants::SCREEN_WIDTH_TILES + tile_x;
}

static inline bool rect_overlaps_solid(const TileMap& map, int x, int y, int w, int h) {
    // compute tile extents (inclusive)
    int left_tile = x / GameConstants::TILE_SIZE;
    int right_tile = (x + w - 1) / GameConstants::TILE_SIZE;
    int top_tile = y / GameConstants::TILE_SIZE;
    int bottom_tile = (y + h - 1) / GameConstants::TILE_SIZE;

    for(int ty = top_tile; ty <= bottom_tile; ++ty) {
        for(int tx = left_tile; tx <= right_tile; ++tx) {
            int idx = tile_index_for(tx, ty);
            if(map.solidity[idx]) return true;
        }
    }
    return false;
}

bool checkVerticalEnemyMapCollision(const TileMap& map, int x, int y) {
    // Enemy is assumed to occupy a TILE_SIZE x TILE_SIZE box
    return rect_overlaps_solid(map, x, y, GameConstants::TILE_SIZE, GameConstants::TILE_SIZE);
}

bool checkHorizontalEnemyMapCollision(const TileMap& map, int x, int y) {
    // Same logic for horizontal movement for now; symmetrical check
    return rect_overlaps_solid(map, x, y, GameConstants::TILE_SIZE, GameConstants::TILE_SIZE);
}

} // namespace collision
