#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <filesystem>
#include <SDL2/SDL.h>

class AssetManager {
public:
    AssetManager();
    ~AssetManager();

    // Initialize asset manager with data directory
    bool initialize();

    // Load and cache textures from PNG files
    SDL_Texture* getTexture(const std::string& name);

    // Load and cache audio buffers from WAV files
    SDL_AudioSpec* getAudioSpec(const std::string& name);

    // Get the data directory path
    const std::filesystem::path& getDataPath() const { return dataPath; }

    // SDL renderer for creating textures
    void setRenderer(SDL_Renderer* renderer) { this->renderer = renderer; }

private:
    std::filesystem::path dataPath;
    SDL_Renderer* renderer = nullptr;
    std::unordered_map<std::string, SDL_Texture*> textures;
    std::unordered_map<std::string, SDL_AudioSpec*> audioSpecs;

    // Helper to resolve data path relative to executable
    std::filesystem::path findDataDirectory();

    // Load PNG file and convert to SDL_Texture
    SDL_Texture* loadPNG(const std::string& filename);

    // Load WAV file and convert to SDL_AudioSpec
    SDL_AudioSpec* loadWAV(const std::string& filename);
};
