# Changelog

## [Unreleased]

- Implement enemy AI behaviors: bounce, leap, roll, seek, shy (per-pixel stepping to avoid tunneling).
- Add and register tests with CTest and provide a top-level `check` target to run them.
- Fix build warnings in `src/main.cpp` and `src/audio/Audio.cpp` (silence unused parameter/variable warnings).
- Update `README_BUILD.md` to mark enemy AI implementation complete.
- Widen position types for `Enemy`, `Fireball`, `Item`, and `Door` to `int16_t` to avoid overflow/truncation on larger maps; clamp enemy positions within world bounds.
