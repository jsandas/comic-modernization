#pragma once

#include <string>
#include <filesystem>
#include "game/GameState.h"

class MapLoader {
public:
    explicit MapLoader(const std::filesystem::path& dataPath);

    // Load the tilemap for a given pt preview filename (e.g., "forest0.pt") and tileset name (e.g., "forest.tt2")
    // Returns a TileMap object with tiles and solidity filled where available
    TileMap loadTileMap(const std::string& ptFilename, const std::string& tilesetName);

private:
    std::filesystem::path dataPath;

    // Helpers
    std::string twoHex(int v) const;
};