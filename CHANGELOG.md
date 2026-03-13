# Treasure Quest: Seven Islands — Changelog

## v0.1.0 — Initial Release

### Engine
- Platform-independent core engine with HAL pattern
- Dual-target: GBA ROM (devkitARM) + PC/Steam (SDL2)
- State machine: Title, World, Battle, Dialogue, Menu, Inventory, Save, Game Over

### Gameplay
- 7 unique island maps (Town, Forest, Desert, Volcano, Frozen, Ruins, Sky Temple)
- Turn-based combat with 14 enemy types + 3 bosses
- Character growth system (3-tier stat progression)
- 13 items + 7 legendary treasures
- NPC system with 20+ NPCs across all islands
- Dialogue system with 10+ conversation scripts
- Save/Load with 3 slots
- Random encounters scaled by island difficulty

### Visual
- 4-direction sprite animation
- Battle effects (damage numbers, hit flash, heal sparkle)
- Screen transitions (venetian blind fade)
- UI framework (windows, cursors, HP bars)

### Audio
- BGM/SFX ID system with per-island track mapping
- 7 BGM tracks + 8 SFX types (abstract, ready for assets)

### PC/Steam
- 60fps frame limiter
- Window scaling (2x-6x) + fullscreen (F11)
- Keyboard (WASD/arrows + ZX) + gamepad support
- Settings persistence (treasure_quest.cfg)
- Command-line options (--scale, --fullscreen)
