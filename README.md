# Treasure Quest: Seven Islands (仮題)

GBA-style turn-based RPG adventure game with dual-target build system.

## Overview

An original IP turn-based RPG inspired by classic GBA adventure games.
Explore seven unique islands, recruit party members, engage in strategic
turn-based combat, and collect legendary treasures.

## Architecture

```
src/
├── core/           # Platform-independent game logic (pure C)
│   ├── game.c      # Game loop, state machine
│   ├── battle.c    # Turn-based combat system
│   ├── map.c       # Tilemap rendering, world navigation
│   ├── party.c     # Party/character management
│   ├── inventory.c # Item/treasure system
│   ├── dialogue.c  # Dialogue/event system
│   └── save.c      # Save/load (SRAM for GBA, file for PC)
├── platform/
│   ├── gba/        # GBA-specific (devkitARM, libtonc)
│   │   ├── main.c  # GBA entry point
│   │   ├── render.c# Mode 0 tile rendering
│   │   ├── input.c # GBA key input
│   │   └── sound.c # GBA sound (DirectSound)
│   └── sdl/        # PC/Steam (SDL2)
│       ├── main.c  # SDL2 entry point
│       ├── render.c# SDL2 rendering with scaling
│       ├── input.c # Keyboard/gamepad input
│       └── sound.c # SDL2_mixer audio
├── data/
│   ├── maps/       # Tilemap data (Tiled JSON → binary)
│   ├── sprites/    # Sprite sheets (Aseprite → GBA format)
│   ├── scripts/    # Event/dialogue scripts
│   └── sound/      # Music & SFX (tracker format)
└── tools/          # Asset pipeline tools
    ├── map2bin.py   # Tiled → binary converter
    └── sprite2gba.py# Sprite sheet → GBA OAM format
include/            # Shared headers
```

## Build Targets

### GBA ROM
```bash
make gba        # Produces treasure_quest.gba
```
Requires: devkitPro (devkitARM), libtonc

### PC/Steam (SDL2)
```bash
make pc          # Produces treasure_quest executable
```
Requires: SDL2, SDL2_mixer, SDL2_image, CMake

## Technical Specs

### GBA Target
- Resolution: 240×160
- Colors: 15 per palette, 16 palettes (backgrounds), 16 palettes (sprites)
- RAM: 32KB IWRAM + 256KB EWRAM
- ROM: up to 32MB
- Save: 64KB SRAM

### PC/Steam Target
- Resolution: 240×160 base, pixel-perfect scaling (2x-6x)
- Fullscreen support with letterboxing
- Keyboard + gamepad input (SDL2 GameController API)
- Steam overlay integration via Steamworks SDK

## Tools

- **devkitPro** — GBA cross-compiler toolchain
- **libtonc** — GBA hardware abstraction library
- **SDL2** — Cross-platform multimedia library
- **Aseprite** — Pixel art editor
- **Tiled** — Tilemap editor
- **mGBA** — GBA emulator for testing

## License

Proprietary — Epic Community
