#include "UI.h"
#include "Graphics.h"
#include "assets/AssetManager.h"
#include "game/GameState.h"
#include "game/Constants.h"
#include <iostream>
#include <cstdio>
#include <string>

UI::UI() : graphics(nullptr), assetManager(nullptr) {
}

UI::~UI() {
}

bool UI::initialize(Graphics* graphics_ptr, AssetManager* assets_ptr) {
    graphics = graphics_ptr;
    assetManager = assets_ptr;
    
    if (!graphics || !assetManager) {
        std::cerr << "UI initialization failed: missing graphics or asset manager" << std::endl;
        return false;
    }
    
    std::cout << "UI system initialized" << std::endl;
    return true;
}

void UI::render(const GameState& game_state) {
    if (!graphics) {
        return;
    }
    
    // Render all UI elements in order
    renderHPBar(game_state);
    renderFireballMeter(game_state);
    renderShieldMeter(game_state);
    renderRightPanelScore(game_state);
    renderInventory(game_state);
    renderLivesAtBottom(game_state);
}

void UI::renderHPBar(const GameState& game_state) {
    // HP bar is 12 pips, but we display in increments of 2
    // So MAX_HP of 6 = 12 pips on screen
    int hp_pips = game_state.comic_hp * 2;
    int max_hp_pips = GameConstants::MAX_HP * 2;
    
    // Draw background bar
    drawRect(HP_BAR_X, HP_BAR_Y, HP_BAR_WIDTH, HP_BAR_HEIGHT, 0);  // Black background
    
    // Draw HP pips
    int pip_width = HP_BAR_WIDTH / max_hp_pips;
    for (int i = 0; i < max_hp_pips; ++i) {
        int pip_x = HP_BAR_X + (i * pip_width);
        
        if (i < hp_pips) {
            // Draw a filled pip in red for remaining HP
            drawRect(pip_x, HP_BAR_Y, pip_width - 1, HP_BAR_HEIGHT, 12);  // Light red (color 12)
        } else {
            // Draw empty pip in dark gray
            drawRect(pip_x, HP_BAR_Y, pip_width - 1, HP_BAR_HEIGHT, 8);  // Dark gray (color 8)
        }
    }
    
    // Optionally, draw a border
    // This would require a different approach since we only have drawRect
}

void UI::renderFireballMeter(const GameState& game_state) {
    // Fireball meter has 12 pips, positioned in right panel
    int fireball_pips = game_state.comic_fireball_meter;
    int max_fireball_pips = GameConstants::MAX_FIREBALL_METER;
    
    // Draw background bar
    drawRect(FIREBALL_METER_X, FIREBALL_METER_Y, FIREBALL_METER_WIDTH, FIREBALL_METER_HEIGHT, 0);
    
    // Draw fireball pips
    int pip_width = FIREBALL_METER_WIDTH / max_fireball_pips;
    for (int i = 0; i < max_fireball_pips; ++i) {
        int pip_x = FIREBALL_METER_X + (i * pip_width);
        
        if (i < fireball_pips) {
            // Draw a filled pip in yellow for fireball charge
            drawRect(pip_x, FIREBALL_METER_Y, pip_width - 1, FIREBALL_METER_HEIGHT, 14);  // Yellow (color 14)
        } else {
            // Draw empty pip in dark gray
            drawRect(pip_x, FIREBALL_METER_Y, pip_width - 1, FIREBALL_METER_HEIGHT, 8);
        }
    }
}

void UI::renderShieldMeter(const GameState& game_state) {
    // Shield meter shows protection status
    int shield_pips = game_state.comic_shield_meter;
    int max_shield_pips = GameConstants::MAX_SHIELD_METER;
    
    // Draw background bar
    drawRect(SHIELD_METER_X, SHIELD_METER_Y, SHIELD_METER_WIDTH, SHIELD_METER_HEIGHT, 0);
    
    // Draw shield pips
    int pip_width = SHIELD_METER_WIDTH / max_shield_pips;
    for (int i = 0; i < max_shield_pips; ++i) {
        int pip_x = SHIELD_METER_X + (i * pip_width);
        
        if (i < shield_pips) {
            // Draw a filled pip in cyan for shield protection
            drawRect(pip_x, SHIELD_METER_Y, pip_width - 1, SHIELD_METER_HEIGHT, 3);  // Cyan (color 3)
        } else {
            // Draw empty pip in dark gray
            drawRect(pip_x, SHIELD_METER_Y, pip_width - 1, SHIELD_METER_HEIGHT, 8);
        }
    }
}

void UI::renderLivesAtBottom(const GameState& game_state) {
    // Display lives at the bottom of the screen
    // Format: "LIVES: X"
    char lives_text[32];
    snprintf(lives_text, sizeof(lives_text), "LIVES: %d", game_state.comic_num_lives);
    drawText(LIVES_X, LIVES_Y, lives_text);
}

void UI::renderRightPanelScore(const GameState& game_state) {
    // Display current score in right panel
    char score_text[32];
    snprintf(score_text, sizeof(score_text), "SCORE:%d", game_state.score);
    drawText(SCORE_X, SCORE_Y, score_text);
}

void UI::renderInventory(const GameState& game_state) {
    // Display collected items in right panel
    // Items: Lantern, Boots, Teleport Wand, Crown, Gold, Gems
    char inventory_text[128] = "";
    int text_y = INVENTORY_Y;
    
    // Collect all items into a string
    std::string items;
    
    if (game_state.comic_has_lantern) {
        if (!items.empty()) items += " ";
        items += "LANTERN";
    }
    if (game_state.comic_has_boots) {
        if (!items.empty()) items += " ";
        items += "BOOTS";
    }
    if (game_state.comic_has_teleport) {
        if (!items.empty()) items += " ";
        items += "TELEPORT";
    }
    if (game_state.comic_has_crown) {
        if (!items.empty()) items += " ";
        items += "CROWN";
    }
    if (game_state.comic_has_gold) {
        if (!items.empty()) items += " ";
        items += "GOLD";
    }
    if (game_state.comic_num_gems > 0) {
        if (!items.empty()) items += " ";
        snprintf(inventory_text, sizeof(inventory_text), "GEMS:%d", game_state.comic_num_gems);
        items += inventory_text;
    }
    
    if (!items.empty()) {
        drawText(INVENTORY_X, text_y, items);
    } else {
        drawText(INVENTORY_X, text_y, "-");  // Empty inventory
    }
}

void UI::drawRect(int x, int y, int width, int height, uint8_t color_index) {
    if (!graphics || !graphics->getRenderer()) {
        return;
    }
    
    // Get the color from the palette
    SDL_Color color = {
        static_cast<uint8_t>((GameConstants::EGA_PALETTE[color_index] >> 16) & 0xFF),
        static_cast<uint8_t>((GameConstants::EGA_PALETTE[color_index] >> 8) & 0xFF),
        static_cast<uint8_t>(GameConstants::EGA_PALETTE[color_index] & 0xFF),
        255
    };
    
    SDL_Renderer* renderer = graphics->getRenderer();
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    
    SDL_Rect rect = {x, y, width, height};
    SDL_RenderFillRect(renderer, &rect);
}

void UI::drawText(int x, int y, const std::string& text) {
    // This is a placeholder for text rendering
    // In the original game, this would be done using the sprite graphics
    // For now, we'll just log that we would draw text here
    // TODO: Implement proper text rendering using sprite fonts from assets
    (void)x; (void)y; (void)text;  // Suppress unused variable warnings
    // std::cout << "UI: Would draw text '" << text << "' at (" << x << ", " << y << ")" << std::endl;
}
