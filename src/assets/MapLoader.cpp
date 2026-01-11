#include "MapLoader.h"
#include <SDL2/SDL.h>
#ifdef HAVE_SDL2_IMAGE
#include <SDL2/SDL_image.h>
#endif
#include <iostream>
#include <vector>
#include <limits>

using namespace std;

MapLoader::MapLoader(const filesystem::path& dataPath)
    : dataPath(dataPath) {}

static SDL_Surface* loadSurfaceConverted(const filesystem::path& p) {
#ifdef HAVE_SDL2_IMAGE
    SDL_Surface* s = IMG_Load(p.string().c_str());
    if (!s) return nullptr;
    SDL_Surface* conv = SDL_ConvertSurfaceFormat(s, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(s);
    return conv;
#else
    // SDL_image not available in this build; cannot load PNG previews
    (void)p;
    std::cerr << "Warning: SDL2_image not available; cannot load preview: " << p << std::endl;
    return nullptr;
#endif
}

static uint64_t surfaceChecksum(SDL_Surface* s) {
    uint64_t sum = 0;
    if (!s) return sum;
    const uint8_t* pixels = static_cast<const uint8_t*>(s->pixels);
    size_t total = s->h * s->pitch;
    for (size_t i = 0; i < total; ++i) sum += pixels[i];
    return sum;
}

TileMap MapLoader::loadTileMap(const std::string& ptFilename, const std::string& tilesetName) {
    TileMap map;

    // Try preview PNG and optional solidity PNG
    filesystem::path preview = dataPath / (ptFilename + ".png");
    filesystem::path solidityPreview = dataPath / (ptFilename + ".solidity.png");

    SDL_Surface* previewSurface = nullptr;
    SDL_Surface* soliditySurface = nullptr;

    if (filesystem::exists(preview)) {
        previewSurface = loadSurfaceConverted(preview);
        if (!previewSurface) {
            cerr << "Warning: failed to load preview PNG: " << preview << endl;
        }
    } else {
        cerr << "Preview PNG not found: " << preview << endl;
    }

    if (filesystem::exists(solidityPreview)) {
        soliditySurface = loadSurfaceConverted(solidityPreview);
        if (!soliditySurface) {
            cerr << "Warning: failed to load solidity PNG: " << solidityPreview << endl;
        } else {
            cerr << "Loaded solidity preview: " << solidityPreview << " (" << soliditySurface->w << "x" << soliditySurface->h << ")" << endl;
        }
    }

    // Load tileset tiles (00..ff)
    vector<SDL_Surface*> tiles;
    tiles.reserve(256);
    vector<uint64_t> checksums;
    checksums.reserve(256);

    for (int i = 0; i < 256; ++i) {
        string filename = tilesetName + "-" + twoHex(i) + ".png";
        filesystem::path p = dataPath / filename;
        if (!filesystem::exists(p)) {
            // Push nullptrs for missing tiles
            tiles.push_back(nullptr);
            checksums.push_back(0);
            continue;
        }
        SDL_Surface* ts = loadSurfaceConverted(p);
        if (!ts) {
            tiles.push_back(nullptr);
            checksums.push_back(0);
            continue;
        }
        tiles.push_back(ts);
        checksums.push_back(surfaceChecksum(ts));
    }

    if (!previewSurface) {
        // No preview available—leave map as zeros
        // Free tile surfaces
        for (auto t : tiles) if (t) SDL_FreeSurface(t);
        return map;
    }

    int tiles_w = previewSurface->w / GameConstants::TILE_SIZE;
    int tiles_h = previewSurface->h / GameConstants::TILE_SIZE;

    // Clamp to game constants
    tiles_w = min(tiles_w, GameConstants::SCREEN_WIDTH_TILES);
    tiles_h = min(tiles_h, GameConstants::SCREEN_HEIGHT_TILES);

    // For each tile, compute region checksum and pick best matching tile
    for (int ty = 0; ty < tiles_h; ++ty) {
        for (int tx = 0; tx < tiles_w; ++tx) {
            // Create a temporary surface for the region
            SDL_Surface* region = SDL_CreateRGBSurfaceWithFormat(0, GameConstants::TILE_SIZE, GameConstants::TILE_SIZE, 32, SDL_PIXELFORMAT_RGBA32);
            if (!region) continue;

            // Blit from preview to region
            SDL_Rect src{tx * GameConstants::TILE_SIZE, ty * GameConstants::TILE_SIZE, GameConstants::TILE_SIZE, GameConstants::TILE_SIZE};
            SDL_Rect dst{0, 0, GameConstants::TILE_SIZE, GameConstants::TILE_SIZE};
            SDL_BlitSurface(previewSurface, &src, region, &dst);

            uint64_t rsum = surfaceChecksum(region);

            // Find best tile by checksum difference
            int best = 0;
            uint64_t bestDiff = std::numeric_limits<uint64_t>::max();
            for (size_t i = 0; i < checksums.size(); ++i) {
                if (tiles[i] == nullptr) continue;
                uint64_t diff = (rsum > checksums[i]) ? (rsum - checksums[i]) : (checksums[i] - rsum);
                if (diff < bestDiff) {
                    bestDiff = diff;
                    best = static_cast<int>(i);
                }
            }

            // Assign tile id
            int tile_idx = ty * GameConstants::SCREEN_WIDTH_TILES + tx;
            if (tile_idx >= 0 && tile_idx < static_cast<int>(map.tiles.size())) {
                map.tiles[tile_idx] = static_cast<uint8_t>(best);
            }

            SDL_FreeSurface(region);
        }
    }

    // Fill remaining tiles (outside preview) with 0

    // Solidity: sample solidity image center of each tile region if present
    if (soliditySurface) {
        // Compute average luminance to determine whether dark=solid or light=solid
        uint64_t sum = 0;
        size_t count = 0;
        for (int y = 0; y < soliditySurface->h; ++y) {
            uint8_t* row = (uint8_t*)soliditySurface->pixels + y * soliditySurface->pitch;
            for (int x = 0; x < soliditySurface->w; ++x) {
                uint8_t r = row[x*4 + 0];
                uint8_t g = row[x*4 + 1];
                uint8_t b = row[x*4 + 2];
                uint8_t lum = static_cast<uint8_t>((uint16_t)r + (uint16_t)g + (uint16_t)b) / 3;
                sum += lum;
                ++count;
            }
        }
        uint8_t avgLum = (count > 0) ? static_cast<uint8_t>(sum / count) : 255;
        bool darkMeansSolid = (avgLum < 128);
        std::cerr << "Solidity avg lum: " << (int)avgLum << " (darkMeansSolid=" << darkMeansSolid << ")" << std::endl;

        for (int ty = 0; ty < tiles_h; ++ty) {
            for (int tx = 0; tx < tiles_w; ++tx) {
                int cx = tx * GameConstants::TILE_SIZE + GameConstants::TILE_SIZE / 2;
                int cy = ty * GameConstants::TILE_SIZE + GameConstants::TILE_SIZE / 2;
                if (cx >= 0 && cx < soliditySurface->w && cy >= 0 && cy < soliditySurface->h) {
                    uint8_t* pixel = (uint8_t*)soliditySurface->pixels + cy * soliditySurface->pitch + cx * 4;
                    uint8_t r = pixel[0];
                    uint8_t g = pixel[1];
                    uint8_t b = pixel[2];
                    uint8_t lum = static_cast<uint8_t>((uint16_t)r + (uint16_t)g + (uint16_t)b) / 3;
                    int idx = ty * GameConstants::SCREEN_WIDTH_TILES + tx;
                    if (idx >= 0 && idx < static_cast<int>(map.solidity.size())) {
                        if (darkMeansSolid) {
                            map.solidity[idx] = (lum < 128) ? 1 : 0;
                        } else {
                            map.solidity[idx] = (lum > 128) ? 1 : 0;
                        }
                    }
                }
            }
        }

        // Debug: print solidity for first 3 rows and 10 columns
        std::cerr << "Solidity sample:" << std::endl;
        for (int y = 0; y < 3 && y < tiles_h; ++y) {
            for (int x = 0; x < 10; ++x) {
                int idx = y * GameConstants::SCREEN_WIDTH_TILES + x;
                std::cerr << (int)map.solidity[idx] << " ";
            }
            std::cerr << std::endl;
        }
    }

    // Free surfaces
    for (auto t : tiles) if (t) SDL_FreeSurface(t);
    if (previewSurface) SDL_FreeSurface(previewSurface);
    if (soliditySurface) SDL_FreeSurface(soliditySurface);

    return map;
}

string MapLoader::twoHex(int v) const {
    static const char* hex = "0123456789abcdef";
    string s(2, '0');
    s[0] = hex[(v >> 4) & 0xf];
    s[1] = hex[v & 0xf];
    return s;
}