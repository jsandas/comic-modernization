#include "ui_system.h"
#include <iostream>
#include <sstream>

UISystem::UISystem()
        : inventory_animation_counter(0),
            life_icon_bright(nullptr),
      life_icon_dark(nullptr),
      meter_full(nullptr),
      meter_half(nullptr),
      meter_empty(nullptr)
{
}

UISystem::~UISystem() {
    cleanup();
}

bool UISystem::initialize() {
    if (!g_graphics) {
        std::cerr << "Graphics system not initialized" << std::endl;
        return false;
    }
    
    // Load score digit sprites (0-9)
    for (int i = 0; i < 10; i++) {
        std::ostringstream oss;
        oss << "score_digit_" << i;
        Sprite* sprite = load_ui_sprite(oss.str());
        if (!sprite) {
            std::cerr << "Failed to load score digit sprite " << i << std::endl;
            return false;
        }
        score_digit_sprites.push_back(sprite);
    }
    
    // Load life icon sprites
    life_icon_bright = load_ui_sprite("life_icon_bright");
    if (!life_icon_bright) {
        std::cerr << "Failed to load life icon bright sprite" << std::endl;
        return false;
    }
    
    life_icon_dark = load_ui_sprite("life_icon_dark");
    if (!life_icon_dark) {
        std::cerr << "Failed to load life icon dark sprite" << std::endl;
        return false;
    }
    
    // Load meter sprites
    meter_full = load_ui_sprite("meter_full");
    if (!meter_full) {
        std::cerr << "Failed to load meter full sprite" << std::endl;
        return false;
    }
    
    meter_half = load_ui_sprite("meter_half");
    if (!meter_half) {
        std::cerr << "Failed to load meter half sprite" << std::endl;
        return false;
    }
    
    meter_empty = load_ui_sprite("meter_empty");
    if (!meter_empty) {
        std::cerr << "Failed to load meter empty sprite" << std::endl;
        return false;
    }
    
    // Load Blastola Cola sprites (even/odd frames)
    for (const char* suffix : {"even", "odd"}) {
        std::ostringstream oss;
        oss << "cola_" << suffix;
        Sprite* sprite = load_ui_sprite(oss.str());
        if (!sprite) {
            std::cerr << "Failed to load Blastola Cola sprite: " << oss.str() << std::endl;
            return false;
        }
        blastola_cola_sprites.push_back(sprite);
    }
    
    // Load corkscrew sprites (even/odd frames)
    for (const char* suffix : {"even", "odd"}) {
        std::ostringstream oss;
        oss << "corkscrew_" << suffix;
        Sprite* sprite = load_ui_sprite(oss.str());
        if (!sprite) {
            std::cerr << "Failed to load corkscrew sprite: " << oss.str() << std::endl;
            return false;
        }
        corkscrew_sprites.push_back(sprite);
    }
    
    // Load door key sprites (even/odd frames)
    for (const char* suffix : {"even", "odd"}) {
        std::ostringstream oss;
        oss << "doorkey_" << suffix;
        Sprite* sprite = load_ui_sprite(oss.str());
        if (!sprite) {
            std::cerr << "Failed to load door key sprite: " << oss.str() << std::endl;
            return false;
        }
        door_key_sprites.push_back(sprite);
    }
    
    // Load boots sprites (even/odd frames)
    for (const char* suffix : {"even", "odd"}) {
        std::ostringstream oss;
        oss << "boots_" << suffix;
        Sprite* sprite = load_ui_sprite(oss.str());
        if (!sprite) {
            std::cerr << "Failed to load boots sprite: " << oss.str() << std::endl;
            return false;
        }
        boots_sprites.push_back(sprite);
    }
    
    // Load lantern sprites (even/odd frames)
    for (const char* suffix : {"even", "odd"}) {
        std::ostringstream oss;
        oss << "lantern_" << suffix;
        Sprite* sprite = load_ui_sprite(oss.str());
        if (!sprite) {
            std::cerr << "Failed to load lantern sprite: " << oss.str() << std::endl;
            return false;
        }
        lantern_sprites.push_back(sprite);
    }
    
    // Load teleport wand sprites (even/odd frames)
    for (const char* suffix : {"even", "odd"}) {
        std::ostringstream oss;
        oss << "teleportwand_" << suffix;
        Sprite* sprite = load_ui_sprite(oss.str());
        if (!sprite) {
            std::cerr << "Failed to load teleport wand sprite: " << oss.str() << std::endl;
            return false;
        }
        teleport_wand_sprites.push_back(sprite);
    }
    
    // Load gems sprites (even/odd frames)
    for (const char* suffix : {"even", "odd"}) {
        std::ostringstream oss;
        oss << "gems_" << suffix;
        Sprite* sprite = load_ui_sprite(oss.str());
        if (!sprite) {
            std::cerr << "Failed to load gems sprite: " << oss.str() << std::endl;
            return false;
        }
        gems_sprites.push_back(sprite);
    }
    
    // Load crown sprites (even/odd frames)
    for (const char* suffix : {"even", "odd"}) {
        std::ostringstream oss;
        oss << "crown_" << suffix;
        Sprite* sprite = load_ui_sprite(oss.str());
        if (!sprite) {
            std::cerr << "Failed to load crown sprite: " << oss.str() << std::endl;
            return false;
        }
        crown_sprites.push_back(sprite);
    }
    
    // Load gold sprites (even/odd frames)
    for (const char* suffix : {"even", "odd"}) {
        std::ostringstream oss;
        oss << "gold_" << suffix;
        Sprite* sprite = load_ui_sprite(oss.str());
        if (!sprite) {
            std::cerr << "Failed to load gold sprite: " << oss.str() << std::endl;
            return false;
        }
        gold_sprites.push_back(sprite);
    }
    
    return true;
}

void UISystem::cleanup() {
    // Note: We don't own the Sprite pointers - they're owned by GraphicsSystem
    // Just clear our vectors
    score_digit_sprites.clear();
    blastola_cola_sprites.clear();
    corkscrew_sprites.clear();
    door_key_sprites.clear();
    boots_sprites.clear();
    lantern_sprites.clear();
    teleport_wand_sprites.clear();
    gems_sprites.clear();
    crown_sprites.clear();
    gold_sprites.clear();
}

Sprite* UISystem::load_ui_sprite(const std::string& sprite_name) {
    if (!g_graphics) return nullptr;
    
    // UI sprites are not directional, so we use empty direction string
    if (!g_graphics->load_sprite(sprite_name, "")) {
        return nullptr;
    }
    
    return g_graphics->get_sprite(sprite_name, "");
}

void UISystem::render_sprite_at(Sprite* sprite, int x, int y, int width, int height) {
    if (!g_graphics || !sprite) return;
    g_graphics->render_sprite_scaled(x, y, *sprite, width, height, false);
}

void UISystem::render_hud(
    uint8_t score_bytes[3],
    uint8_t num_lives,
    uint8_t hp,
    uint8_t fireball_meter,
    uint8_t firepower,
    bool has_corkscrew,
    bool has_door_key,
    bool has_teleport_wand,
    bool has_lantern,
    bool has_gems,
    bool has_crown,
    bool has_gold,
    uint8_t jump_power)
{
    inventory_animation_counter ^= 1;

    render_score(score_bytes);
    render_lives(num_lives);
    render_hp_meter(hp);
    render_fireball_meter(fireball_meter);
    render_inventory(firepower, has_corkscrew, has_door_key, has_teleport_wand,
                    has_lantern, has_gems, has_crown, has_gold, jump_power);
}

void UISystem::render_score(const uint8_t score_bytes[3]) {
    // Render 3 base-100 bytes as 6 decimal digits
    // Score position: X=232-280, Y=24
    // Each digit is 8 pixels wide, 16 pixels tall
    
    if (score_digit_sprites.empty()) return;
    
    for (int digit_idx = 0; digit_idx < 3; digit_idx++) {
        uint8_t base100_value = score_bytes[digit_idx];
        
        // Convert base-100 to two decimal digits (high, low)
        uint8_t high_decimal = base100_value / 10;
        uint8_t low_decimal = base100_value % 10;
        
        // Calculate X position (rightmost pair at 264, then 248, then 232)
        int base_x = 264 - (digit_idx * 16);
        
        // Render high digit first at base_x
        if (high_decimal < score_digit_sprites.size()) {
            render_sprite_at(score_digit_sprites[high_decimal], base_x, 24, 8, 16);
        }
        
        // Render low digit at base_x + 8
        if (low_decimal < score_digit_sprites.size()) {
            render_sprite_at(score_digit_sprites[low_decimal], base_x + 8, 24, 8, 16);
        }
    }
}

void UISystem::render_lives(uint8_t num_lives) {
    // Render life icons: bright for active lives, dark for inactive
    // Position: Y=180, X=24 + (life_count × 24) pixels
    // MAX_NUM_LIVES = 5
    
    if (!life_icon_bright || !life_icon_dark) return;
    
    constexpr uint8_t MAX_NUM_LIVES = 5;
    constexpr int START_X = 48;
    constexpr int LIFE_SPACING = 24;
    constexpr int LIVES_Y = 180;
    
    for (uint8_t life = 0; life < MAX_NUM_LIVES; life++) {
        int x = START_X + (life * LIFE_SPACING);
        
        // Render bright icon if life is active, dark otherwise
        if (life < num_lives) {
            render_sprite_at(life_icon_bright, x, LIVES_Y, 16, 16);
        } else {
            render_sprite_at(life_icon_dark, x, LIVES_Y, 16, 16);
        }
    }
}

void UISystem::render_hp_meter(uint8_t hp) {
    // Render 6 HP cells
    // Position: Y=82, X=248-288 (cells at 8-pixel intervals)
    // Each cell is 8×16 pixels
    // Cell N shows full if hp >= N, otherwise empty
    // hp range: 0-6 (MAX_HP = 6)
    
    if (!meter_full || !meter_empty) return;
    
    constexpr uint8_t MAX_HP = 6;
    constexpr int METER_Y = 82;
    constexpr int CELL_WIDTH = 8;
    constexpr uint8_t MAX_CELLS = 6;
    constexpr int HUD_METER_RIGHT_EDGE_X = 296;
    constexpr int START_X = HUD_METER_RIGHT_EDGE_X - (MAX_CELLS * CELL_WIDTH);
    
    for (uint8_t cell = 0; cell < MAX_HP; cell++) {
        int x = START_X + (cell * CELL_WIDTH);
        
        // Render full if hp > cell, empty otherwise
        if (hp > cell) {
            render_sprite_at(meter_full, x, METER_Y, 8, 16);
        } else {
            render_sprite_at(meter_empty, x, METER_Y, 8, 16);
        }
    }
}

void UISystem::render_fireball_meter(uint8_t meter) {
    // Render 6 cells representing 0-12 meter value (2 units per cell)
    // Position: Y=54, X=240-280 (cells at 8-pixel intervals)
    // Each cell is 8×16 pixels
    // Meter mapping: cells represent meter value pairs (1-2, 3-4, 5-6, etc.)
    // Odd meter values → half cell, even meter values → full cell
    
    if (!meter_full || !meter_half || !meter_empty) return;
    
    constexpr int METER_Y = 54;
    constexpr int CELL_WIDTH = 8;
    constexpr uint8_t MAX_CELLS = 6;
    constexpr int HUD_METER_RIGHT_EDGE_X = 296;
    constexpr int START_X = HUD_METER_RIGHT_EDGE_X - (MAX_CELLS * CELL_WIDTH);
    
    for (uint8_t cell = 0; cell < MAX_CELLS; cell++) {
        int x = START_X + (cell * CELL_WIDTH);
        
        // Determine meter value represented by this cell
        uint8_t meter_value_low = (cell * 2) + 1;   // 1, 3, 5, ...
        uint8_t meter_value_high = (cell * 2) + 2;  // 2, 4, 6, ...
        
        if (meter >= meter_value_high) {
            // Full cell
            render_sprite_at(meter_full, x, METER_Y, 8, 16);
        } else if (meter >= meter_value_low) {
            // Half cell
            render_sprite_at(meter_half, x, METER_Y, 8, 16);
        } else {
            // Empty cell
            render_sprite_at(meter_empty, x, METER_Y, 8, 16);
        }
    }
}

void UISystem::render_inventory(
    uint8_t firepower,
    bool has_corkscrew,
    bool has_door_key,
    bool has_teleport_wand,
    bool has_lantern,
    bool has_gems,
    bool has_crown,
    bool has_gold,
    uint8_t jump_power)
{
    // Inventory grid layout:
    // Row 1 (Y=112): Blastola Cola (X=232), Corkscrew (X=256), Door Key (X=280)
    // Row 2 (Y=136): Boots (X=232), Lantern (X=256), Teleport Wand (X=280)
    // Row 3 (Y=160): Gems (X=232), Crown (X=256), Gold (X=280)
    
    // Each sprite is 16×16 pixels
    
    // Row 1: Y=112
    {
        // Blastola Cola - rendered if firepower > 0 (show 1-5 variant)
        if (firepower > 0 && !blastola_cola_sprites.empty()) {
            size_t frame = inventory_animation_counter % blastola_cola_sprites.size();
            render_sprite_at(blastola_cola_sprites[frame], 232, 112, 16, 16);
        }
        
        // Corkscrew - rendered if has_corkscrew (use even frame for now)
        if (has_corkscrew && !corkscrew_sprites.empty()) {
            size_t frame = inventory_animation_counter % corkscrew_sprites.size();
            render_sprite_at(corkscrew_sprites[frame], 256, 112, 16, 16);
        }
        
        // Door Key - rendered if has_door_key (use even frame for now)
        if (has_door_key && !door_key_sprites.empty()) {
            size_t frame = inventory_animation_counter % door_key_sprites.size();
            render_sprite_at(door_key_sprites[frame], 280, 112, 16, 16);
        }
    }
    
    // Row 2: Y=136
    {
        // Boots - rendered if jump_power > 4 (boots increase jump power from 4 to 5)
        if (jump_power > 4 && !boots_sprites.empty()) {
            size_t frame = inventory_animation_counter % boots_sprites.size();
            render_sprite_at(boots_sprites[frame], 232, 136, 16, 16);
        }
        
        // Lantern - rendered if has_lantern (use even frame for now)
        if (has_lantern && !lantern_sprites.empty()) {
            size_t frame = inventory_animation_counter % lantern_sprites.size();
            render_sprite_at(lantern_sprites[frame], 256, 136, 16, 16);
        }
        
        // Teleport Wand - rendered if has_teleport_wand (use even frame for now)
        if (has_teleport_wand && !teleport_wand_sprites.empty()) {
            size_t frame = inventory_animation_counter % teleport_wand_sprites.size();
            render_sprite_at(teleport_wand_sprites[frame], 280, 136, 16, 16);
        }
    }
    
    // Row 3: Y=160
    {
        // Gems (1st Treasure) - rendered if has_gems (use even frame for now)
        if (has_gems && !gems_sprites.empty()) {
            size_t frame = inventory_animation_counter % gems_sprites.size();
            render_sprite_at(gems_sprites[frame], 232, 160, 16, 16);
        }
        
        // Crown (2nd Treasure) - rendered if has_crown (use even frame for now)
        if (has_crown && !crown_sprites.empty()) {
            size_t frame = inventory_animation_counter % crown_sprites.size();
            render_sprite_at(crown_sprites[frame], 256, 160, 16, 16);
        }
        
        // Gold (3rd Treasure) - rendered if has_gold (use even frame for now)
        if (has_gold && !gold_sprites.empty()) {
            size_t frame = inventory_animation_counter % gold_sprites.size();
            render_sprite_at(gold_sprites[frame], 280, 160, 16, 16);
        }
    }
}
