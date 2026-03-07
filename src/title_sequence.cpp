#include "../include/title_sequence.h"
#include "../include/graphics.h"
#include "../include/audio.h"
#include <SDL2/SDL_image.h>
#include <iostream>
#include <string>

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

// Original EGA resolution
constexpr int EGA_WIDTH = 320;
constexpr int EGA_HEIGHT = 200;

// Fade-in timing for palette-step transitions (original: wait_n_ticks(1) per step)
constexpr int FADE_STEP_DELAY_MS = 55;

// Title screen display time: wait_n_ticks(14) at ~55 ms/tick ≈ 770 ms
constexpr int TITLE_DISPLAY_MS = 770;

// Asset filenames (pre-converted PNGs from EGA originals)
static const char* TITLE_SCREEN_FILE   = "sys000.ega.png";  // Title image
static const char* STORY_SCREEN_FILE   = "sys001.ega.png";  // Story image
static const char* GAME_UI_FILE        = "sys003.ega.png";  // In-game HUD background
static const char* ITEMS_SCREEN_FILE   = "sys004.ega.png";  // Items/controls screen

// ---------------------------------------------------------------------------
// Module state
// ---------------------------------------------------------------------------

// HUD texture retained for gameplay rendering
static SDL_Texture* s_hud_texture = nullptr;

// Paletted surfaces for title sequence screens (kept as-is, not converted to RGBA)
static SDL_Surface* s_title_surface = nullptr;
static SDL_Surface* s_story_surface = nullptr;
static SDL_Surface* s_items_surface = nullptr;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/**
 * Load a fullscreen EGA image (320x200 PNG) as a paletted SDL_Surface.
 * Preserves the indexed color palette from the original PNG file.
 * Returns nullptr on failure.
 */
static SDL_Surface* load_fullscreen_paletted_surface(GraphicsSystem* graphics,
                                                      const char* filename) {
    std::string path = graphics->get_asset_path(filename);

    SDL_Surface* surface = IMG_Load(path.c_str());
    if (!surface) {
        std::cerr << "Title sequence: failed to load " << filename
                  << " (" << IMG_GetError() << ")" << std::endl;
        return nullptr;
    }

    // If the surface has a palette, keep it as-is.
    // If not, this indicates an issue with the PNG format conversion.
    if (!surface->format->palette) {
        std::cerr << "Title sequence: " << filename
                  << " is not paletted (expected 8-bit indexed color)" << std::endl;
    }

    return surface;
}

/**
 * Create a texture from a paletted surface with optional palette modifications.
 */
static SDL_Texture* surface_to_texture(SDL_Renderer* renderer,
                                       SDL_Surface* surface) {
    // Convert paletted surface to RGBA for rendering
    SDL_Surface* converted = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
    if (!converted) {
        std::cerr << "Title sequence: failed to convert surface format: "
                  << SDL_GetError() << std::endl;
        return nullptr;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, converted);
    SDL_FreeSurface(converted);

    if (!texture) {
        std::cerr << "Title sequence: failed to create texture: "
                  << SDL_GetError() << std::endl;
    }

    return texture;
}

/**
 * Compute a destination rect that scales a 320x200 image to fill the current
 * renderer output while preserving aspect ratio (letterboxed if needed).
 */
static SDL_Rect compute_display_rect(SDL_Renderer* renderer) {
    int win_w = 0, win_h = 0;
    SDL_GetRendererOutputSize(renderer, &win_w, &win_h);

    // Compute integer scale that fits both dimensions
    float scale_x = static_cast<float>(win_w) / EGA_WIDTH;
    float scale_y = static_cast<float>(win_h) / EGA_HEIGHT;
    float scale = (scale_x < scale_y) ? scale_x : scale_y;

    int dst_w = static_cast<int>(EGA_WIDTH * scale);
    int dst_h = static_cast<int>(EGA_HEIGHT * scale);

    SDL_Rect rect;
    rect.x = (win_w - dst_w) / 2;
    rect.y = (win_h - dst_h) / 2;
    rect.w = dst_w;
    rect.h = dst_h;
    return rect;
}

/**
 * Display a paletted surface with the 5-step palette fade-in effect from the original.
 *
 * Replicates palette_fade_in() from the DOS code:
 *   Step 1: All three entries (2,10,12) → dark gray (0x18)
 *   Step 2: All three → light gray (0x07)
 *   Step 3: Background stays gray, items/title → white (0x1f)
 *   Step 4: Background→green (0x02), items→bright green (0x1a), title stays white
 *   Step 5: Title → bright red (0x1c)
 *
 * Each step waits ~55ms (original: wait_n_ticks(1)).
 * 
 * Returns false ONLY if SDL_QUIT was received. Format/palette errors are treated
 * as non-fatal (logs warning, skips fade effect, returns true to continue).
 */
static bool fade_in_paletted_surface(SDL_Renderer* renderer, SDL_Surface* surface,
                                     GraphicsSystem* graphics, const char* filename) {
    if (!surface || !surface->format || !surface->format->palette) {
        std::cerr << "Warning: fade_in_paletted_surface: surface is not paletted, skipping fade effect\n";
        return true;  // Non-fatal: skip fade, continue sequence
    }

    SDL_Palette* pal = surface->format->palette;
    SDL_Rect dst = compute_display_rect(renderer);

    // Palette entry indices (from original DOS code)
    constexpr int PALETTE_REG_BACKGROUND = 2;
    constexpr int PALETTE_REG_ITEMS = 10;
    constexpr int PALETTE_REG_TITLE = 12;

    // Store original palette colors for restoration using the existing surface
    if (!surface || !surface->format || !surface->format->palette) {
        std::cerr << "fade_in_paletted_surface: surface has no valid palette\n";
        return false;
    }

    SDL_Color orig_colors[3] = {
        surface->format->palette->colors[PALETTE_REG_BACKGROUND],
        surface->format->palette->colors[PALETTE_REG_ITEMS],
        surface->format->palette->colors[PALETTE_REG_TITLE]
    };

    // Helper: Convert 6-bit EGA color to 8-bit RGB
    auto ega_to_rgb = [](uint8_t ega_6bit) -> uint8_t {
        return (ega_6bit << 2) | (ega_6bit >> 4);
    };

    // Define palette values in 6-bit EGA format, converted to 8-bit
    constexpr uint8_t DARK_GRAY_6BIT = 0x18;  // ~39% gray
    constexpr uint8_t LIGHT_GRAY_6BIT = 0x07; // ~11% gray
    constexpr uint8_t WHITE_6BIT = 0x1f;      // 100% white
    constexpr uint8_t GREEN_6BIT = 0x02;      // dark green
    constexpr uint8_t BRIGHT_GREEN_6BIT = 0x1a;  // 110010 in binary
    constexpr uint8_t BRIGHT_RED_6BIT = 0x1c;    // 011100 in binary

    const uint8_t dark_gray = ega_to_rgb(DARK_GRAY_6BIT);
    const uint8_t light_gray = ega_to_rgb(LIGHT_GRAY_6BIT);
    const uint8_t white = ega_to_rgb(WHITE_6BIT);
    const uint8_t green = ega_to_rgb(GREEN_6BIT);
    const uint8_t bright_green = ega_to_rgb(BRIGHT_GREEN_6BIT);
    const uint8_t bright_red = ega_to_rgb(BRIGHT_RED_6BIT);

    SDL_Event e;
    // Step 1: All three to dark gray
    if (PALETTE_REG_BACKGROUND < pal->ncolors) {
        pal->colors[PALETTE_REG_BACKGROUND] = {dark_gray, dark_gray, dark_gray, 255};
    }
    if (PALETTE_REG_ITEMS < pal->ncolors) {
        pal->colors[PALETTE_REG_ITEMS] = {dark_gray, dark_gray, dark_gray, 255};
    }
    if (PALETTE_REG_TITLE < pal->ncolors) {
        pal->colors[PALETTE_REG_TITLE] = {dark_gray, dark_gray, dark_gray, 255};
    }

    SDL_Texture* tex = surface_to_texture(renderer, surface);
    if (!tex) {
        std::cerr << "Warning: fade_in_paletted_surface: texture creation failed, skipping fade effect\n";
        return true;  // Non-fatal: skip fade, continue sequence
    }

    // Render step 1
    {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, tex, nullptr, &dst);
        SDL_RenderPresent(renderer);
        SDL_DestroyTexture(tex);
    }

    // Process events during wait
    auto wait_with_events = [&](int ms) -> bool {
        uint32_t start = SDL_GetTicks();
        while (SDL_GetTicks() - start < static_cast<uint32_t>(ms)) {
            while (SDL_PollEvent(&e)) {
                if (e.type == SDL_QUIT) return false;
            }
            SDL_Delay(16);
        }
        return true;
    };

    // Wait after step 1
    if (!wait_with_events(FADE_STEP_DELAY_MS)) return false;

    // Step 2: All three to light gray
    if (PALETTE_REG_BACKGROUND < pal->ncolors) {
        pal->colors[PALETTE_REG_BACKGROUND] = {light_gray, light_gray, light_gray, 255};
    }
    if (PALETTE_REG_ITEMS < pal->ncolors) {
        pal->colors[PALETTE_REG_ITEMS] = {light_gray, light_gray, light_gray, 255};
    }
    if (PALETTE_REG_TITLE < pal->ncolors) {
        pal->colors[PALETTE_REG_TITLE] = {light_gray, light_gray, light_gray, 255};
    }

    tex = surface_to_texture(renderer, surface);
    if (!tex) {
        std::cerr << "Warning: fade_in_paletted_surface: texture creation failed at step 2, skipping fade effect\n";
        return true;  // Non-fatal: skip fade, continue sequence
    }

    {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, tex, nullptr, &dst);
        SDL_RenderPresent(renderer);
        SDL_DestroyTexture(tex);
    }

    if (!wait_with_events(FADE_STEP_DELAY_MS)) return false;

    // Step 3: Background stays light gray, items and title go white
    if (PALETTE_REG_BACKGROUND < pal->ncolors) {
        pal->colors[PALETTE_REG_BACKGROUND] = {light_gray, light_gray, light_gray, 255};
    }
    if (PALETTE_REG_ITEMS < pal->ncolors) {
        pal->colors[PALETTE_REG_ITEMS] = {white, white, white, 255};
    }
    if (PALETTE_REG_TITLE < pal->ncolors) {
        pal->colors[PALETTE_REG_TITLE] = {white, white, white, 255};
    }

    tex = surface_to_texture(renderer, surface);
    if (!tex) {
        std::cerr << "Warning: fade_in_paletted_surface: texture creation failed at step 3, skipping fade effect\n";
        return true;  // Non-fatal: skip fade, continue sequence
    }

    {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, tex, nullptr, &dst);
        SDL_RenderPresent(renderer);
        SDL_DestroyTexture(tex);
    }

    if (!wait_with_events(FADE_STEP_DELAY_MS)) return false;

    // Step 4: Background → green, items → bright green, title stays white
    if (PALETTE_REG_BACKGROUND < pal->ncolors) {
        pal->colors[PALETTE_REG_BACKGROUND] = {0, green, 0, 255};
    }
    if (PALETTE_REG_ITEMS < pal->ncolors) {
        pal->colors[PALETTE_REG_ITEMS] = {0, bright_green, 0, 255};
    }
    if (PALETTE_REG_TITLE < pal->ncolors) {
        pal->colors[PALETTE_REG_TITLE] = {white, white, white, 255};
    }

    tex = surface_to_texture(renderer, surface);
    if (!tex) {
        std::cerr << "Warning: fade_in_paletted_surface: texture creation failed at step 4, skipping fade effect\n";
        return true;  // Non-fatal: skip fade, continue sequence
    }

    {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, tex, nullptr, &dst);
        SDL_RenderPresent(renderer);
        SDL_DestroyTexture(tex);
    }

    if (!wait_with_events(FADE_STEP_DELAY_MS)) return false;

    // Step 5: Title goes bright red, restore to original colors
    if (PALETTE_REG_BACKGROUND < pal->ncolors) {
        pal->colors[PALETTE_REG_BACKGROUND] = orig_colors[0];
    }
    if (PALETTE_REG_ITEMS < pal->ncolors) {
        pal->colors[PALETTE_REG_ITEMS] = orig_colors[1];
    }
    if (PALETTE_REG_TITLE < pal->ncolors) {
        pal->colors[PALETTE_REG_TITLE] = {bright_red, 0, 0, 255};
    }

    tex = surface_to_texture(renderer, surface);
    if (!tex) {
        std::cerr << "Warning: fade_in_paletted_surface: texture creation failed at step 5, skipping fade effect\n";
        return true;  // Non-fatal: skip fade, continue sequence
    }

    {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, tex, nullptr, &dst);
        SDL_RenderPresent(renderer);
        SDL_DestroyTexture(tex);
    }

    if (!wait_with_events(FADE_STEP_DELAY_MS)) return false;

    // Step 6: Restore all to original colors
    if (PALETTE_REG_BACKGROUND < pal->ncolors) {
        pal->colors[PALETTE_REG_BACKGROUND] = orig_colors[0];
    }
    if (PALETTE_REG_ITEMS < pal->ncolors) {
        pal->colors[PALETTE_REG_ITEMS] = orig_colors[1];
    }
    if (PALETTE_REG_TITLE < pal->ncolors) {
        pal->colors[PALETTE_REG_TITLE] = orig_colors[2];
    }

    tex = surface_to_texture(renderer, surface);
    if (!tex) {
        std::cerr << "Warning: fade_in_paletted_surface: texture creation failed at step 6, skipping fade effect\n";
        return true;  // Non-fatal: skip fade, continue sequence
    }

    {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, tex, nullptr, &dst);
        SDL_RenderPresent(renderer);
        SDL_DestroyTexture(tex);
    }

    return true;
}

/**
 * Display a texture (no fade) and render one frame.
 */
static void show_texture(SDL_Renderer* renderer, SDL_Texture* texture) {
    SDL_Rect dst = compute_display_rect(renderer);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, &dst);
    SDL_RenderPresent(renderer);
}

/**
 * Wait for a fixed number of milliseconds while processing events.
 * Returns false if SDL_QUIT received.
 */
static bool wait_ms(int duration_ms) {
    uint32_t start = SDL_GetTicks();
    SDL_Event e;
    while (SDL_GetTicks() - start < static_cast<uint32_t>(duration_ms)) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) return false;
        }
        SDL_Delay(16);
    }
    return true;
}

/**
 * Wait until any key is pressed (blocking).
 * Returns false if SDL_QUIT received.
 */
static bool wait_for_keypress(SDL_Renderer* renderer, SDL_Texture* texture) {
    SDL_Rect dst = compute_display_rect(renderer);
    SDL_Event e;

    while (true) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) return false;
            if (e.type == SDL_KEYDOWN) return true;
        }

        // Keep presenting the current screen while waiting
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, &dst);
        SDL_RenderPresent(renderer);

        SDL_Delay(16);
    }
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

bool run_title_sequence(SDL_Renderer* renderer, GraphicsSystem* graphics) {
    // ------------------------------------------------------------------
    // Step 1: Title screen (SYS000.EGA)
    //   - Load image, display with palette fade-in, play title music, wait ~770 ms
    // ------------------------------------------------------------------
    s_title_surface = load_fullscreen_paletted_surface(graphics, TITLE_SCREEN_FILE);
    if (!s_title_surface) {
        std::cerr << "Title sequence aborted: could not load title screen" << std::endl;
        return true;
    }

    // Palette fade-in (darkens registers 2,10,12 then restores with color effects)
    if (!fade_in_paletted_surface(renderer, s_title_surface, graphics, TITLE_SCREEN_FILE)) {
        SDL_FreeSurface(s_title_surface);
        s_title_surface = nullptr;
        return false;
    }

    // Start title music (loops until stopped)
    play_game_music(GameMusic::TITLE);

    // Create texture and display for ~770 ms (original: wait_n_ticks(14))
    SDL_Texture* title_tex = surface_to_texture(renderer, s_title_surface);
    if (!title_tex) {
        SDL_FreeSurface(s_title_surface);
        s_title_surface = nullptr;
        stop_game_music();
        return true;
    }

    show_texture(renderer, title_tex);
    SDL_DestroyTexture(title_tex);

    if (!wait_ms(TITLE_DISPLAY_MS)) {
        SDL_FreeSurface(s_title_surface);
        s_title_surface = nullptr;
        stop_game_music();
        return false;
    }

    SDL_FreeSurface(s_title_surface);
    s_title_surface = nullptr;

    // ------------------------------------------------------------------
    // Step 2: Story screen (SYS001.EGA)
    //   - Load image, display with palette fade-in, wait for keypress
    // ------------------------------------------------------------------
    s_story_surface = load_fullscreen_paletted_surface(graphics, STORY_SCREEN_FILE);
    if (!s_story_surface) {
        std::cerr << "Title sequence aborted: could not load story screen" << std::endl;
        stop_game_music();
        return true;
    }

    if (!fade_in_paletted_surface(renderer, s_story_surface, graphics, STORY_SCREEN_FILE)) {
        SDL_FreeSurface(s_story_surface);
        s_story_surface = nullptr;
        stop_game_music();
        return false;
    }

    // Create texture for keypress waiting
    SDL_Texture* story_tex = surface_to_texture(renderer, s_story_surface);
    if (!story_tex) {
        SDL_FreeSurface(s_story_surface);
        s_story_surface = nullptr;
        stop_game_music();
        return true;
    }

    if (!wait_for_keypress(renderer, story_tex)) {
        SDL_DestroyTexture(story_tex);
        SDL_FreeSurface(s_story_surface);
        s_story_surface = nullptr;
        stop_game_music();
        return false;
    }

    SDL_DestroyTexture(story_tex);
    SDL_FreeSurface(s_story_surface);
    s_story_surface = nullptr;

    // ------------------------------------------------------------------
    // Step 3: Game UI background (SYS003.EGA)
    //   - Load HUD graphic into a persistent texture for gameplay use.
    //   - Original loads into both gameplay buffers; here we keep one
    //     texture that the game loop composites under the playfield.
    // ------------------------------------------------------------------
    SDL_Surface* hud_surface = load_fullscreen_paletted_surface(graphics, GAME_UI_FILE);
    if (hud_surface) {
        SDL_Texture* hud_tex = surface_to_texture(renderer, hud_surface);
        SDL_FreeSurface(hud_surface);

        if (hud_tex) {
            // Retain for gameplay
            if (s_hud_texture) {
                SDL_DestroyTexture(s_hud_texture);
            }
            s_hud_texture = hud_tex;
        }
    } else {
        std::cerr << "Warning: Could not load game UI graphic" << std::endl;
    }

    // ------------------------------------------------------------------
    // Step 4: Items screen (SYS004.EGA)
    //   - Display WITHOUT palette fade effect, wait for keypress
    // ------------------------------------------------------------------
    s_items_surface = load_fullscreen_paletted_surface(graphics, ITEMS_SCREEN_FILE);
    if (!s_items_surface) {
        std::cerr << "Title sequence aborted: could not load items screen" << std::endl;
        stop_game_music();
        return true;
    }

    // Create texture directly (no fade effect for items screen)
    SDL_Texture* items_tex = surface_to_texture(renderer, s_items_surface);
    if (!items_tex) {
        SDL_FreeSurface(s_items_surface);
        s_items_surface = nullptr;
        stop_game_music();
        return true;
    }

    if (!wait_for_keypress(renderer, items_tex)) {
        SDL_DestroyTexture(items_tex);
        SDL_FreeSurface(s_items_surface);
        s_items_surface = nullptr;
        stop_game_music();
        return false;
    }

    SDL_DestroyTexture(items_tex);
    SDL_FreeSurface(s_items_surface);
    s_items_surface = nullptr;

    // ------------------------------------------------------------------
    // Step 5: Display HUD graphic as transition (SYS003.EGA)
    //   - Show the game UI background that will be used during gameplay
    //   - Convert and display the retained HUD texture before returning
    // ------------------------------------------------------------------
    if (s_hud_texture) {
        show_texture(renderer, s_hud_texture);
        // Hold HUD display briefly before transitioning to gameplay
        if (!wait_ms(300)) {
            stop_game_music();
            return false;
        }
    }

    // ------------------------------------------------------------------
    // Step 6: Cleanup and transition
    //   - Stop title music before entering gameplay
    // ------------------------------------------------------------------
    stop_game_music();

    return true;
}

SDL_Texture* get_hud_texture() {
    return s_hud_texture;
}

void cleanup_title_sequence() {
    if (s_title_surface) {
        SDL_FreeSurface(s_title_surface);
        s_title_surface = nullptr;
    }
    if (s_story_surface) {
        SDL_FreeSurface(s_story_surface);
        s_story_surface = nullptr;
    }
    if (s_items_surface) {
        SDL_FreeSurface(s_items_surface);
        s_items_surface = nullptr;
    }
    if (s_hud_texture) {
        SDL_DestroyTexture(s_hud_texture);
        s_hud_texture = nullptr;
    }
}
