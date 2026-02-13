#ifndef FILE_LOADER_H
#define FILE_LOADER_H

#include <cstdint>
#include <string>
#include "level.h"

/* Tileset size constant */
constexpr int MAX_TILESET_SIZE = 128 * 128;  /* 128 tiles, 128 bytes each */

/**
 * pt_file_t - Tile map structure (from .PT file)
 * 
 * One stage is a 128×10 tile map. Each tile is 1 byte (0-255).
 * Tiles with ID > tileset_last_passable are solid obstacles.
 */
struct pt_file_t {
    uint16_t width;                      /* Always 128 */
    uint16_t height;                     /* Always 10 */
    uint8_t tiles[128 * 10];             /* 1280 bytes (128×10 tile map) */
};

/**
 * TT2 file header structure
 * 
 * First 4 bytes of .TT2 file contain metadata about the tileset.
 */
struct tt2_header_t {
    uint8_t last_passable;  /* Maximum tile ID that's passable (tiles > this are solid) */
    uint8_t unused1;        /* Unused/reserved */
    uint8_t unused2;        /* Unused/reserved */
    uint8_t flags;          /* Flags byte (purpose unclear) */
};

/**
 * Load PT file (tile map)
 * 
 * PT format:
 *   Bytes 0-1: Width (uint16_t, always 128)
 *   Bytes 2-3: Height (uint16_t, always 10)
 *   Bytes 4-1283: Tile IDs (1280 bytes = 128 * 10)
 * 
 * Returns true on success, false on error
 */
bool load_pt_file(const std::string& filepath, pt_file_t& pt);

/**
 * Load TT2 file (tileset graphics)
 * 
 * TT2 format:
 *   Bytes 0-3: Header (last_passable, unused1, unused2, flags)
 *   Bytes 4+: Variable-length tile graphics data
 *     Each tile = 128 bytes (EGA format: 4 planes × 32 bytes = 16×16 pixels)
 *     Variable number of tiles (4K-11K file size = 32-96 tiles typically)
 * 
 * Returns true on success, false on error
 * On success, out_last_passable is set from header
 */
bool load_tt2_file(const std::string& filepath, uint8_t* tileset_buffer, int tileset_size,
                   uint8_t& out_last_passable);

/**
 * Set the base path for loading game assets
 * 
 * The loader will search in this path for all game files.
 * Default is "original/" if not set.
 */
void set_asset_path(const std::string& path);

#endif /* FILE_LOADER_H */
