# Title Sequence Function Overview

This document explains the **`title_sequence()`** function from the original C reference implementation  
in [`jsandas/comic-c`](https://github.com/jsandas/comic-c/blob/master/src/game_main.c).
It outlines every step that this routine performs so another AI (or developer) can reproduce the behaviour in **C++ using SDL**.
Focus is on the flow and intent rather than exact DOS/BIOS calls.

---

## 🎯 Purpose

Sets up and displays the game's introductory screens, plays music, then initializes and starts the actual gameplay.
The sequence is executed once during program startup when the player elects to continue from the startup notice.

## 🔁 High‑level flow

1. **Switch to EGA graphics mode** (320×200, 16‑color).
2. **Initialize graphics controller and palette.**

3. **Title screen**
   - Load `SYS000.EGA` into a temporary graphics buffer.
   - Display it by switching the active buffer.
   - Perform a palette darken/fade‑in effect to create a smooth visual transition when the image appears.
   - Start title music.
   - Wait ~770 ms (14 game ticks).

4. **Story screen**
   - Load `SYS001.EGA` into another temp buffer.
   - Flip buffers to show story image.
   - Perform the same palette darken/fade‑in effect so the story screen transitions in smoothly.
   - Pause until any key is pressed.

5. **Game UI background**
   - Load `SYS003.EGA` (the static in‑game UI graphic) into **both** gameplay buffers (A and B).
   - Copy the contents of buffer A to buffer B for a static background.
   - This buffer now contains the HUD elements: life icons, score digits, HP meter, fireball meter and inventory slots.  During gameplay the engine will blit sprites over this UI on the active buffer.

6. **Items screen**
   - Load `SYS004.EGA` into a title buffer and display it.
   - Wait for a keystroke.

7. **Cleanup and transition**
   - Stop title music.
   - Switch the active video buffer to gameplay‑buffer B containing the UI.
   - Call `initialize_lives_sequence()` to show the extra‑lives animation and set starting lives.
   - Load the first level (`load_new_level()`), then the first stage (`load_new_stage()`).
   - Enter `game_loop()` to begin normal play.

---

## 🔧 Detailed Operations

- **Graphics mode setup**
  ```c
  regs.h.ah = 0x00; regs.h.al = 0x0D;  /* 320×200 16‑color EGA */
  int86(0x10, &regs, &regs);
  init_ega_graphics();
  init_default_palette();
  ```

- **Loading and displaying full‑screen graphics**
  The helper `load_fullscreen_graphic(filename, buffer)` returns `0` on success.
  If loading fails at any step:
  - the title sequence is aborted,
  - title music is stopped if already playing,
  - and the function returns immediately.

- **Palette effects**
  `palette_darken()` followed by `palette_fade_in()` produce a smooth fade‑in from black.

- **Timing**
  `wait_n_ticks(n)` waits `n` ticks (~55 ms each) by polling the BIOS timer.
  Used for fixed delays (title display) and during the lives‑animation.

- **Input polling**
  Keypresses are retrieved via BIOS `INT 16h`, blocking until the user presses any key.

- **Music control**
  - `play_title_music()` starts background music before the title screen delay.
  - `stop_music()` halts it before moving to gameplay.

- **Buffer management**
  - `switch_video_buffer(buffer)` points the hardware to whichever 64 KB buffer holds the desired image.
  - The UI graphic (`SYS003.EGA`) is pre‑loaded into both gameplay buffers so the HUD stays fixed while gameplay sprites are drawn on the active buffer during game_loop.
  - The HUD includes:
    * a row of life icons (bright/dark) reflecting `comic_num_lives`
    * three score digits (24‑bit score) at the top of the screen
    * an HP meter showing current health
    * a fireball charge meter
    * inventory indicators for items (door key, teleport wand, corkscrew, lantern, shield, etc.)
  - `copy_ega_plane(src, dst, size)` duplicates 8 KB planes – used to replicate the UI.

- **Post‑title initialization**
  Lives animation and level loading are part of the sequence to seamlessly hand off to the main game loop.

---

## 📝 SDL/C++ Reproduction Notes

To replicate this behaviour in SDL:

- **Graphics mode** → create a 320×200 surface/texture with a 16‑color palette.
- **Buffers** → maintain two SDL textures for double‑buffering; additional temporary textures for title/story/items screens.
- **Palette effects** → modulate palette entries or use SDL render‑target fade routines for darken/fade‑in.
- **Asset loading** → read EGA files (raw 320×200 planar data) into textures; SDL can blit them directly.
- **Timing** → SDL’s `SDL_GetTicks()` or a fixed 55 ms delay loop.
- **Input** → SDL event polling for keydown events.
- **Audio** → SDL_mixer or SDL_audio to start/stop title music.
- **Sequence logic** remains identical; just replace DMA/INT calls with SDL equivalents.

---

By following this outline, the AI agent can reconstruct the complete title sequence in C++/SDL while preserving timing, visual transitions, input handling, music playback and state transitions into gameplay.