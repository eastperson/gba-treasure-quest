/**
 * tiledata.h — 8x8 pixel art patterns for tiles and sprites
 *
 * Each tile/sprite is stored as 8 bytes (8 rows x 8 cols, 1 bit per pixel).
 * Bit=1 draws foreground color, Bit=0 draws background color.
 * MSB = leftmost pixel.
 *
 * Shared across GBA and SDL renderers.
 */
#ifndef TILEDATA_H
#define TILEDATA_H

#include <stdint.h>

/* ── Tile Patterns (TILE_GRASS..TILE_PORT = 9 tiles) ─── */
static const uint8_t TILE_PATTERNS[9][8] = {
    /* TILE_GRASS — scattered dots */
    { 0x00, 0x20, 0x04, 0x00, 0x40, 0x02, 0x10, 0x00 },
    /* TILE_WATER — wavy lines */
    { 0x00, 0x49, 0x92, 0x00, 0x00, 0x92, 0x49, 0x00 },
    /* TILE_SAND — fine dots */
    { 0x44, 0x00, 0x11, 0x00, 0x44, 0x00, 0x11, 0x00 },
    /* TILE_STONE — brick pattern */
    { 0xFF, 0x82, 0x82, 0xFF, 0xFF, 0x28, 0x28, 0xFF },
    /* TILE_TREE — tree crown + trunk */
    { 0x18, 0x3C, 0x7E, 0xFF, 0x7E, 0x3C, 0x18, 0x18 },
    /* TILE_WALL — solid with edge */
    { 0xFF, 0xC3, 0xA5, 0x99, 0x99, 0xA5, 0xC3, 0xFF },
    /* TILE_DOOR — door shape */
    { 0x7E, 0x42, 0x42, 0x42, 0x42, 0x4A, 0x42, 0x7E },
    /* TILE_CHEST — chest shape */
    { 0x00, 0x7E, 0xFF, 0xFF, 0x7E, 0x7E, 0x7E, 0x00 },
    /* TILE_PORT — anchor/dock */
    { 0x18, 0x18, 0x7E, 0x18, 0x18, 0x24, 0x42, 0x00 },
};

/* BGR555 color pairs [bg, fg] for each tile type */
static const uint16_t TILE_COLORS[9][2] = {
    /* GRASS  */ { 0x1AA0, 0x2360 },  /* dark green bg, light green fg */
    /* WATER  */ { 0x5800, 0x7C00 },  /* dark blue bg, light blue fg */
    /* SAND   */ { 0x1EF7, 0x2F7B },  /* tan bg, light tan fg */
    /* STONE  */ { 0x294A, 0x4210 },  /* dark gray bg, light gray fg */
    /* TREE   */ { 0x1AA0, 0x0160 },  /* green bg, dark green fg (trunk) */
    /* WALL   */ { 0x2529, 0x3DEF },  /* brown bg, light brown fg */
    /* DOOR   */ { 0x1126, 0x2DAD },  /* dark brown bg, wood fg */
    /* CHEST  */ { 0x1AA0, 0x02BF },  /* green bg (grass), gold fg */
    /* PORT   */ { 0x1EF7, 0x5800 },  /* sand bg, blue fg */
};

/* ── Sprite Patterns (8x8 pixel art, improved with fill) ── */
/* Player directions: 0=down, 1=up, 2=left, 3=right */
/* 2 frames per direction for walking animation */
/* Sprites have more filled area for multi-color rendering */
static const uint8_t SPRITE_PLAYER[4][2][8] = {
    /* DOWN frame 0, frame 1 */
    {
        { 0x3C, 0x7E, 0x5A, 0x7E, 0x3C, 0x7E, 0x24, 0x24 },
        { 0x3C, 0x7E, 0x5A, 0x7E, 0x3C, 0x7E, 0x14, 0x28 },
    },
    /* UP frame 0, frame 1 */
    {
        { 0x3C, 0x7E, 0x7E, 0x7E, 0x3C, 0x7E, 0x24, 0x24 },
        { 0x3C, 0x7E, 0x7E, 0x7E, 0x3C, 0x7E, 0x28, 0x14 },
    },
    /* LEFT frame 0, frame 1 */
    {
        { 0x3C, 0x7C, 0x5C, 0x7C, 0x3C, 0x7E, 0x24, 0x24 },
        { 0x3C, 0x7C, 0x5C, 0x7C, 0x3C, 0x7E, 0x0C, 0x30 },
    },
    /* RIGHT frame 0, frame 1 */
    {
        { 0x3C, 0x3E, 0x3A, 0x3E, 0x3C, 0x7E, 0x24, 0x24 },
        { 0x3C, 0x3E, 0x3A, 0x3E, 0x3C, 0x7E, 0x30, 0x0C },
    },
};

/* Per-row colors for player sprite (BGR555):
 * row 0: hair, row 1-3: skin/face, row 4-5: shirt, row 6-7: pants/boots */
static const uint16_t PLAYER_ROW_COLORS[8] = {
    0x0948,  /* hair — dark brown */
    0x1EFF,  /* face — skin tone */
    0x1EFF,  /* eyes — skin (eye gaps show background) */
    0x1EFF,  /* face — skin tone */
    0x7C00,  /* shirt — blue */
    0x7C00,  /* shirt — blue */
    0x294A,  /* pants — dark gray */
    0x1126,  /* boots — brown */
};

/* NPC sprite (generic townsperson — more filled) */
static const uint8_t SPRITE_NPC[8] = {
    0x3C, 0x7E, 0x5A, 0x66, 0x3C, 0x7E, 0x24, 0x24
};

/* Per-row colors for NPC sprite */
static const uint16_t NPC_ROW_COLORS[8] = {
    0x03E0,  /* hat — green */
    0x1EFF,  /* face — skin */
    0x1EFF,  /* eyes — skin */
    0x1EFF,  /* mouth — skin */
    0x2D6B,  /* body — olive */
    0x2D6B,  /* body — olive */
    0x294A,  /* legs — gray */
    0x294A,  /* feet — gray */
};

/* Enemy sprites for battle screen (8x8 with more detail) */
static const uint8_t SPRITE_ENEMIES[5][8] = {
    /* Slime — blobby */
    { 0x00, 0x18, 0x3C, 0x7E, 0xFF, 0xDB, 0xFF, 0x7E },
    /* Bat — wings spread */
    { 0x81, 0xC3, 0xE7, 0xFF, 0x7E, 0x3C, 0x18, 0x18 },
    /* Skeleton — skull and bones */
    { 0x3C, 0x7E, 0xDB, 0x7E, 0x3C, 0x5A, 0x24, 0x24 },
    /* Golem — bulky */
    { 0x7E, 0xFF, 0xDB, 0xFF, 0xFF, 0x7E, 0x7E, 0xFF },
    /* Dragon (boss) — horned */
    { 0xA5, 0xDB, 0xFF, 0xFF, 0x7E, 0xFF, 0xDB, 0xC3 },
};

/* Per-row colors for enemies */
static const uint16_t ENEMY_ROW_COLORS[5][8] = {
    /* Slime — green body, dark bottom */
    { 0x1FE0, 0x1FE0, 0x2FE0, 0x2FE0, 0x1FE0, 0x0CC0, 0x1FE0, 0x0CC0 },
    /* Bat — purple wings, dark body */
    { 0x6010, 0x5010, 0x4010, 0x3010, 0x4010, 0x5010, 0x6010, 0x6010 },
    /* Skeleton — white bones, gray */
    { 0x7FFF, 0x6B5A, 0x7FFF, 0x6B5A, 0x7FFF, 0x5294, 0x5294, 0x5294 },
    /* Golem — gray/brown rock */
    { 0x35AD, 0x4210, 0x294A, 0x4210, 0x35AD, 0x294A, 0x294A, 0x35AD },
    /* Dragon — red/orange fire */
    { 0x001F, 0x02BF, 0x001F, 0x02BF, 0x001F, 0x02BF, 0x001F, 0x001F },
};

/* Legacy defines for compatibility */
#define COLOR_PLAYER_BODY  0x7C00  /* blue */
#define COLOR_PLAYER_SKIN  0x1EFF  /* skin tone */
#define COLOR_NPC_BODY     0x03E0  /* green */
#define COLOR_ENEMY_COLORS_COUNT 5

static const uint16_t COLOR_ENEMIES[5] = {
    0x1FE0,  /* Slime — lime green */
    0x4010,  /* Bat — purple */
    0x7FFF,  /* Skeleton — white */
    0x294A,  /* Golem — gray */
    0x001F,  /* Dragon — red */
};

#endif /* TILEDATA_H */
