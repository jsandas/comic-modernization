#pragma once

#include <SDL2/SDL.h>
#include <memory>
#include <string>
#include <cstdint>

// Forward declarations
class Graphics;
class AssetManager;
struct GameState;

class UI {
public:
    UI();
    ~UI();

    // Initialize UI system with graphics and asset manager
    bool initialize(Graphics* graphics, AssetManager* assets);
    
    // Render the entire UI overlay based on game state
    void render(const GameState& game_state);

private:
    Graphics* graphics = nullptr;
    AssetManager* assetManager = nullptr;

    // Render individual UI elements
    void renderHPBar(const GameState& game_state);
    void renderFireballMeter(const GameState& game_state);
    void renderShieldMeter(const GameState& game_state);
    void renderLivesAtBottom(const GameState& game_state);
    void renderRightPanelScore(const GameState& game_state);
    void renderInventory(const GameState& game_state);
    
    // Helper to render a simple colored rectangle
    void drawRect(int x, int y, int width, int height, uint8_t color_index);
    
    // Helper to render text (if text rendering is implemented)
    void drawText(int x, int y, const std::string& text);

    // ===== UI Layout =====
    // Screen dimensions: 320x200
    // Right third (columns 213-319): Score, fireball, shield, inventory
    // Bottom (rows 184-199): Lives display
    
    // Left side UI (playfield area margin)
    static constexpr int HP_BAR_X = 8;
    static constexpr int HP_BAR_Y = 8;
    static constexpr int HP_BAR_WIDTH = 56;
    static constexpr int HP_BAR_HEIGHT = 8;
    
    // Right third of screen starts at x = 213 (320 * 2/3)
    static constexpr int RIGHT_PANEL_X = 213;
    static constexpr int PANEL_MARGIN = 8;
    
    // Score at top of right panel
    static constexpr int SCORE_X = RIGHT_PANEL_X + PANEL_MARGIN;
    static constexpr int SCORE_Y = 8;
    
    // Fireball meter in right panel
    static constexpr int FIREBALL_METER_X = RIGHT_PANEL_X + PANEL_MARGIN;
    static constexpr int FIREBALL_METER_Y = 32;
    static constexpr int FIREBALL_METER_WIDTH = 92;
    static constexpr int FIREBALL_METER_HEIGHT = 8;
    
    // Shield meter in right panel
    static constexpr int SHIELD_METER_X = RIGHT_PANEL_X + PANEL_MARGIN;
    static constexpr int SHIELD_METER_Y = 48;
    static constexpr int SHIELD_METER_WIDTH = 92;
    static constexpr int SHIELD_METER_HEIGHT = 8;
    
    // Inventory display in right panel
    static constexpr int INVENTORY_X = RIGHT_PANEL_X + PANEL_MARGIN;
    static constexpr int INVENTORY_Y = 64;
    
    // Lives at bottom of screen
    static constexpr int LIVES_Y = 184;
    static constexpr int LIVES_X = 8;
};

