/**
 * file_loader.cpp - Game asset file loaders
 * 
 * Loads tile maps (.PT) and tileset graphics (.TT2) from binary files.
 */

#include "file_loader.h"
#include <fstream>
#include <iostream>
#include <cstring>

/* Global asset path */
static std::string g_asset_path = "original/";

/* Map dimensions (must match physics.h constants) */
constexpr int PT_MAP_WIDTH = 128;
constexpr int PT_MAP_HEIGHT = 10;

void set_asset_path(const std::string& path)
{
    g_asset_path = path;
    /* Ensure trailing slash */
    if (!g_asset_path.empty() && g_asset_path.back() != '/') {
        g_asset_path += '/';
    }
}

bool load_pt_file(const std::string& filepath, pt_file_t& pt)
{
    std::string full_path = g_asset_path + filepath;
    
    std::ifstream file(full_path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "ERROR: Failed to open PT file: " << full_path << std::endl;
        return false;
    }
    
    /* Read width (little-endian uint16) */
    file.read(reinterpret_cast<char*>(&pt.width), sizeof(uint16_t));
    if (!file) {
        std::cerr << "ERROR: Failed to read width from PT file: " << full_path << std::endl;
        return false;
    }
    
    /* Read height (little-endian uint16) */
    file.read(reinterpret_cast<char*>(&pt.height), sizeof(uint16_t));
    if (!file) {
        std::cerr << "ERROR: Failed to read height from PT file: " << full_path << std::endl;
        return false;
    }
    
    /* Validate dimensions */
    if (pt.width != PT_MAP_WIDTH || pt.height != PT_MAP_HEIGHT) {
        std::cerr << "ERROR: PT file has unexpected dimensions: " << pt.width << "x" << pt.height
                  << " (expected " << PT_MAP_WIDTH << "x" << PT_MAP_HEIGHT << ")" << std::endl;
        return false;
    }
    
    /* Read tile data (1280 bytes) */
    int expected_bytes = PT_MAP_WIDTH * PT_MAP_HEIGHT;
    file.read(reinterpret_cast<char*>(pt.tiles), expected_bytes);
    
    int bytes_read = file.gcount();
    if (bytes_read != expected_bytes) {
        std::cerr << "ERROR: PT file incomplete. Expected " << expected_bytes << " bytes, got " 
                  << bytes_read << std::endl;
        return false;
    }
    
    return true;
}

bool load_tt2_file(const std::string& filepath, uint8_t* tileset_buffer, int tileset_size,
                   uint8_t& out_last_passable)
{
    if (!tileset_buffer || tileset_size <= 0) {
        std::cerr << "ERROR: Invalid tileset buffer provided to load_tt2_file" << std::endl;
        return false;
    }
    
    std::string full_path = g_asset_path + filepath + ".TT2";
    
    std::ifstream file(full_path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "ERROR: Failed to open TT2 file: " << full_path << std::endl;
        return false;
    }
    
    /* Read 4-byte header */
    tt2_header_t header;
    file.read(reinterpret_cast<char*>(&header), sizeof(tt2_header_t));
    
    if (!file || file.gcount() != sizeof(tt2_header_t)) {
        std::cerr << "ERROR: Failed to read TT2 header from: " << full_path << std::endl;
        return false;
    }
    
    /* Extract and store the last_passable value */
    out_last_passable = header.last_passable;
    
    /* Read variable-length tileset graphics data */
    /* Clear the buffer first */
    std::memset(tileset_buffer, 0, tileset_size);
    
    /* Read as much data as available (up to buffer size) */
    file.read(reinterpret_cast<char*>(tileset_buffer), tileset_size);
    int bytes_read = file.gcount();
    
    if (bytes_read == 0) {
        std::cerr << "WARNING: TT2 file has no tile graphics data: " << full_path << std::endl;
        /* This is non-critical - we can still render with an empty tileset */
    }
    
    return true;
}
