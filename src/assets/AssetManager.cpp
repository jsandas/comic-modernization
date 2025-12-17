#include "AssetManager.h"
#include <iostream>
#ifdef HAVE_SDL2_IMAGE
#include <SDL2/SDL_image.h>
#endif

AssetManager::AssetManager() 
    : renderer(nullptr) {
}

AssetManager::~AssetManager() {
    // Clean up cached textures
    for (auto& [name, info] : textures) {
        if (info.texture) {
            SDL_DestroyTexture(info.texture);
        }
    }
    textures.clear();

    // Clean up cached audio specs
    for (auto& [name, spec] : audioSpecs) {
        if (spec) {
            delete spec;
        }
    }
    audioSpecs.clear();
}

bool AssetManager::initialize() {
    dataPath = findDataDirectory();
    
    if (!std::filesystem::exists(dataPath)) {
        std::cerr << "Error: Data directory not found at: " << dataPath << std::endl;
        return false;
    }

    std::cout << "Asset Manager initialized. Data path: " << dataPath << std::endl;
    return true;
}

std::filesystem::path AssetManager::findDataDirectory() {
    // Get the directory where the executable is running
    // Assuming the executable is in the build/ directory
    // and data is in build/data/
    std::filesystem::path executablePath = std::filesystem::current_path();
    std::filesystem::path dataDir = executablePath / "data";
    
    // If that doesn't exist, try relative path from executable location
    if (!std::filesystem::exists(dataDir)) {
        // Try ../data relative to build directory
        dataDir = executablePath.parent_path() / "data";
    }
    
    return dataDir;
}

SDL_Texture* AssetManager::getTexture(const std::string& name) {
    auto it = textures.find(name);
    if (it != textures.end()) {
        return it->second.texture;
    }

    SDL_Texture* texture = loadPNG(name);
    if (texture) {
        TextureInfo info;
        info.texture = texture;
        SDL_QueryTexture(texture, nullptr, nullptr, &info.width, &info.height);
        textures[name] = info;
        return texture;
    }

    return nullptr;
}

TextureInfo AssetManager::getTextureInfo(const std::string& name) {
    auto it = textures.find(name);
    if (it != textures.end()) {
        return it->second;
    }

    SDL_Texture* texture = loadPNG(name);
    if (texture) {
        TextureInfo info;
        info.texture = texture;
        SDL_QueryTexture(texture, nullptr, nullptr, &info.width, &info.height);
        textures[name] = info;
        return info;
    }

    return TextureInfo();
}

SDL_AudioSpec* AssetManager::getAudioSpec(const std::string& name) {
    auto it = audioSpecs.find(name);
    if (it != audioSpecs.end()) {
        return it->second;
    }

    SDL_AudioSpec* spec = loadWAV(name);
    if (spec) {
        audioSpecs[name] = spec;
        return spec;
    }

    return nullptr;
}

SDL_Texture* AssetManager::loadPNG(const std::string& filename) {
#ifndef HAVE_SDL2_IMAGE
    std::cerr << "Error: SDL2_image not available for PNG loading" << std::endl;
    return nullptr;
#endif

    if (!renderer) {
        std::cerr << "Error: Renderer not set for AssetManager" << std::endl;
        return nullptr;
    }

    std::filesystem::path filePath = dataPath / (filename + ".png");

    if (!std::filesystem::exists(filePath)) {
        std::cerr << "Warning: PNG file not found: " << filePath << std::endl;
        return nullptr;
    }

#ifdef HAVE_SDL2_IMAGE
    SDL_Surface* surface = IMG_Load(filePath.c_str());
    if (!surface) {
        std::cerr << "Error loading PNG: " << IMG_GetError() << std::endl;
        return nullptr;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    if (!texture) {
        std::cerr << "Error creating texture from surface: " << SDL_GetError() << std::endl;
        return nullptr;
    }

    return texture;
#else
    return nullptr;
#endif
}

SDL_AudioSpec* AssetManager::loadWAV(const std::string& filename) {
    std::filesystem::path filePath = dataPath / (filename + ".wav");

    if (!std::filesystem::exists(filePath)) {
        std::cerr << "Warning: WAV file not found: " << filePath << std::endl;
        return nullptr;
    }

    SDL_AudioSpec* spec = new SDL_AudioSpec();
    if (SDL_LoadWAV(filePath.c_str(), spec, nullptr, nullptr) == nullptr) {
        std::cerr << "Error loading WAV: " << SDL_GetError() << std::endl;
        delete spec;
        return nullptr;
    }

    return spec;
}
