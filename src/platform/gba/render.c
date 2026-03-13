/**
 * render.c — GBA Mode 3 bitmap rendering (libtonc)
 *
 * Uses the same 4x6 bitmap font as the SDL version so text
 * looks identical on both targets.
 */
#include "platform.h"

#ifdef PLATFORM_GBA

#include <tonc.h>
#include <string.h>
#include "tiledata.h"

/*
 * 4x6 bitmap font for ASCII 32-127.
 * Each character is stored as 3 bytes (24 bits) = 6 rows x 4 cols.
 * Bit layout: row0[col0 col1 col2 col3] row1[...] ... row5[...]
 * MSB = top-left pixel.
 *
 * Shared with the SDL version — keep both copies in sync.
 */
static const u8 g_font_data[96][3] = {
    /* 32 ' ' */ {0x00, 0x00, 0x00},
    /* 33 '!' */ {0x44, 0x40, 0x40},
    /* 34 '"' */ {0xAA, 0x00, 0x00},
    /* 35 '#' */ {0x5F, 0x5F, 0x50},
    /* 36 '$' */ {0x6E, 0xC7, 0x60},
    /* 37 '%' */ {0x93, 0x64, 0x90},
    /* 38 '&' */ {0x4A, 0x4A, 0xD0},
    /* 39 ''' */ {0x44, 0x00, 0x00},
    /* 40 '(' */ {0x24, 0x44, 0x20},
    /* 41 ')' */ {0x42, 0x22, 0x40},
    /* 42 '*' */ {0x0A, 0x4A, 0x00},
    /* 43 '+' */ {0x04, 0xE4, 0x00},
    /* 44 ',' */ {0x00, 0x00, 0x24},
    /* 45 '-' */ {0x00, 0xE0, 0x00},
    /* 46 '.' */ {0x00, 0x00, 0x40},
    /* 47 '/' */ {0x12, 0x44, 0x80},
    /* 48 '0' */ {0x69, 0xBD, 0x96},
    /* 49 '1' */ {0x26, 0x22, 0x27},
    /* 50 '2' */ {0x69, 0x24, 0x8F},
    /* 51 '3' */ {0xE1, 0x61, 0x1E},
    /* 52 '4' */ {0x99, 0xF1, 0x11},
    /* 53 '5' */ {0xF8, 0xE1, 0x1E},
    /* 54 '6' */ {0x68, 0xE9, 0x96},
    /* 55 '7' */ {0xF1, 0x24, 0x44},
    /* 56 '8' */ {0x69, 0x69, 0x96},
    /* 57 '9' */ {0x69, 0x97, 0x16},
    /* 58 ':' */ {0x04, 0x04, 0x00},
    /* 59 ';' */ {0x04, 0x04, 0x24},
    /* 60 '<' */ {0x12, 0x48, 0x42},
    /* 61 '=' */ {0x0E, 0x0E, 0x00},
    /* 62 '>' */ {0x42, 0x12, 0x48},
    /* 63 '?' */ {0x69, 0x20, 0x20},
    /* 64 '@' */ {0x6B, 0xDB, 0x87},
    /* 65 'A' */ {0x69, 0x9F, 0x99},
    /* 66 'B' */ {0xE9, 0xE9, 0x9E},
    /* 67 'C' */ {0x69, 0x88, 0x96},
    /* 68 'D' */ {0xE9, 0x99, 0x9E},
    /* 69 'E' */ {0xF8, 0xE8, 0x8F},
    /* 70 'F' */ {0xF8, 0xE8, 0x88},
    /* 71 'G' */ {0x69, 0x8B, 0x96},
    /* 72 'H' */ {0x99, 0xF9, 0x99},
    /* 73 'I' */ {0xE4, 0x44, 0x4E},
    /* 74 'J' */ {0x71, 0x11, 0x96},
    /* 75 'K' */ {0x9A, 0xCA, 0x99},
    /* 76 'L' */ {0x88, 0x88, 0x8F},
    /* 77 'M' */ {0x9F, 0xF9, 0x99},
    /* 78 'N' */ {0x9D, 0xBB, 0x99},
    /* 79 'O' */ {0x69, 0x99, 0x96},
    /* 80 'P' */ {0xE9, 0x9E, 0x88},
    /* 81 'Q' */ {0x69, 0x9B, 0x95},
    /* 82 'R' */ {0xE9, 0x9E, 0xA9},
    /* 83 'S' */ {0x69, 0x86, 0x1E},
    /* 84 'T' */ {0xE4, 0x44, 0x44},
    /* 85 'U' */ {0x99, 0x99, 0x96},
    /* 86 'V' */ {0x99, 0x99, 0x66},
    /* 87 'W' */ {0x99, 0x9F, 0xF9},
    /* 88 'X' */ {0x99, 0x69, 0x99},
    /* 89 'Y' */ {0x99, 0x64, 0x44},
    /* 90 'Z' */ {0xF1, 0x24, 0x8F},
    /* 91 '[' */ {0x64, 0x44, 0x46},
    /* 92 '\' */ {0x84, 0x42, 0x21},
    /* 93 ']' */ {0x62, 0x22, 0x26},
    /* 94 '^' */ {0x69, 0x00, 0x00},
    /* 95 '_' */ {0x00, 0x00, 0x0F},
    /* 96 '`' */ {0x42, 0x00, 0x00},
    /* 97  'a' (=A) */ {0x69, 0x9F, 0x99},
    /* 98  'b' (=B) */ {0xE9, 0xE9, 0x9E},
    /* 99  'c' (=C) */ {0x69, 0x88, 0x96},
    /* 100 'd' (=D) */ {0xE9, 0x99, 0x9E},
    /* 101 'e' (=E) */ {0xF8, 0xE8, 0x8F},
    /* 102 'f' (=F) */ {0xF8, 0xE8, 0x88},
    /* 103 'g' (=G) */ {0x69, 0x8B, 0x96},
    /* 104 'h' (=H) */ {0x99, 0xF9, 0x99},
    /* 105 'i' (=I) */ {0xE4, 0x44, 0x4E},
    /* 106 'j' (=J) */ {0x71, 0x11, 0x96},
    /* 107 'k' (=K) */ {0x9A, 0xCA, 0x99},
    /* 108 'l' (=L) */ {0x88, 0x88, 0x8F},
    /* 109 'm' (=M) */ {0x9F, 0xF9, 0x99},
    /* 110 'n' (=N) */ {0x9D, 0xBB, 0x99},
    /* 111 'o' (=O) */ {0x69, 0x99, 0x96},
    /* 112 'p' (=P) */ {0xE9, 0x9E, 0x88},
    /* 113 'q' (=Q) */ {0x69, 0x9B, 0x95},
    /* 114 'r' (=R) */ {0xE9, 0x9E, 0xA9},
    /* 115 's' (=S) */ {0x69, 0x86, 0x1E},
    /* 116 't' (=T) */ {0xE4, 0x44, 0x44},
    /* 117 'u' (=U) */ {0x99, 0x99, 0x96},
    /* 118 'v' (=V) */ {0x99, 0x99, 0x66},
    /* 119 'w' (=W) */ {0x99, 0x9F, 0xF9},
    /* 120 'x' (=X) */ {0x99, 0x69, 0x99},
    /* 121 'y' (=Y) */ {0x99, 0x64, 0x44},
    /* 122 'z' (=Z) */ {0xF1, 0x24, 0x8F},
    /* 123 '{' */ {0x24, 0x84, 0x42},
    /* 124 '|' */ {0x44, 0x44, 0x44},
    /* 125 '}' */ {0x42, 0x14, 0x24},
    /* 126 '~' */ {0x05, 0xA0, 0x00},
};

void platform_init(void) {
    /* Mode 3 bitmap — 240x160, 15-bit direct color */
    REG_DISPCNT = DCNT_MODE3 | DCNT_BG2;
}

void platform_shutdown(void) {
    /* GBA doesn't shut down */
}

void platform_frame_start(void) {
    /* Nothing needed before render */
}

void platform_frame_end(void) {
    vid_vsync();
}

/* ── Rendering (Mode 3 bitmap) ─────────────────────────── */
void platform_clear(u16 color) {
    memset16(vid_mem, color, SCREEN_W * SCREEN_H);
}

static inline void m3_plot(int x, int y, u16 color) {
    if ((unsigned)x < SCREEN_W && (unsigned)y < SCREEN_H)
        vid_mem[y * SCREEN_W + x] = color;
}

void platform_draw_tile(int x, int y, int tile_id, int palette) {
    (void)palette;
    if (tile_id < 0 || tile_id >= 9) return;
    u16 bg = TILE_COLORS[tile_id][0];
    u16 fg = TILE_COLORS[tile_id][1];
    const u8 *pat = TILE_PATTERNS[tile_id];
    for (int dy = 0; dy < 8; dy++) {
        u8 row = pat[dy];
        for (int dx = 0; dx < 8; dx++) {
            u16 c = (row & (0x80 >> dx)) ? fg : bg;
            m3_plot(x + dx, y + dy, c);
        }
    }
}

void platform_draw_sprite(int x, int y, int sprite_id,
                          int palette, bool flip_h) {
    (void)palette;
    const u8 *pat;
    u16 color;

    if (sprite_id <= 7) {
        int dir = sprite_id / 2;
        int frame = sprite_id % 2;
        if (dir > 3) dir = 0;
        pat = SPRITE_PLAYER[dir][frame];
        color = COLOR_PLAYER_BODY;
    } else if (sprite_id == 8) {
        pat = SPRITE_NPC;
        color = COLOR_NPC_BODY;
    } else if (sprite_id >= 10 && sprite_id < 10 + COLOR_ENEMY_COLORS_COUNT) {
        pat = SPRITE_ENEMIES[sprite_id - 10];
        color = COLOR_ENEMIES[sprite_id - 10];
    } else {
        pat = SPRITE_NPC;
        color = 0x7FFF;
    }

    for (int dy = 0; dy < 8; dy++) {
        u8 row = pat[dy];
        for (int dx = 0; dx < 8; dx++) {
            int sx = flip_h ? (7 - dx) : dx;
            if (row & (0x80 >> sx)) {
                m3_plot(x + dx, y + dy, color);
            }
        }
    }
}

void platform_draw_rect(int x, int y, int w, int h, u16 color) {
    for (int dy = 0; dy < h; dy++)
        for (int dx = 0; dx < w; dx++)
            m3_plot(x + dx, y + dy, color);
}

void platform_draw_text(int x, int y, const char *text, u16 color) {
    int cx = x;
    while (*text) {
        int ch = (unsigned char)*text - 32;
        if (ch >= 0 && ch < 96) {
            /*
             * Each glyph is 3 bytes = 24 bits for a 4-wide x 6-tall bitmap.
             * Bits packed MSB-first: bit23 = row0,col0 ... bit0 = row5,col3.
             */
            const u8 *gdata = g_font_data[ch];
            u32 glyph = ((u32)gdata[0] << 16)
                       | ((u32)gdata[1] << 8)
                       |  (u32)gdata[2];
            for (int row = 0; row < 6; row++) {
                for (int col = 0; col < 4; col++) {
                    if (glyph & (1u << (23 - row * 4 - col))) {
                        m3_plot(cx + col, y + row, color);
                    }
                }
            }
        }
        cx += 5; /* 4px glyph + 1px spacing */
        text++;
    }
}

#endif /* PLATFORM_GBA */
