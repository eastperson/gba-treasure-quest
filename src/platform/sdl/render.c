/**
 * render.c — SDL2 rendering with pixel-perfect scaling
 */
#include "platform.h"

#ifdef PLATFORM_SDL

#include <SDL2/SDL.h>
#include <string.h>
#include <stdio.h>
#include "tiledata.h"

#ifdef PLATFORM_WEB
#include <emscripten.h>
#endif

/* ── Internal State ────────────────────────────────────── */
static SDL_Window   *g_window;
static SDL_Renderer *g_renderer;
static SDL_Texture  *g_framebuffer;
static uint16_t      g_pixels[SCREEN_W * SCREEN_H];
static uint32_t      g_pixels32[SCREEN_W * SCREEN_H]; /* 32-bit conversion buffer */
static bool          g_use_32bit = false; /* true if ABGR1555 texture failed */
static int           g_scale = 3;
static bool          g_fullscreen = false;

/*
 * 4x6 bitmap font for ASCII 32-127.
 * Each character is stored as 3 bytes (24 bits) = 6 rows x 4 cols.
 * Bit layout: row0[col0 col1 col2 col3] row1[...] ... row5[...]
 * MSB = top-left pixel.
 */
static const uint8_t g_font_data[96][3] = {
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
    /* Init video first — audio may fail on mobile without user gesture */
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "FATAL: SDL_Init(VIDEO) failed: %s\n", SDL_GetError());
        return;
    }
    /* Try audio separately — non-fatal if it fails */
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        printf("SDL audio init skipped (non-fatal)\n");
    }

    g_window = SDL_CreateWindow(
        "Treasure Quest: Seven Islands",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_W * g_scale, SCREEN_H * g_scale,
        SDL_WINDOW_RESIZABLE
    );
    if (!g_window) {
        fprintf(stderr, "FATAL: SDL_CreateWindow failed: %s\n", SDL_GetError());
        return;
    }

#ifdef PLATFORM_WEB
    /* Web: create renderer (emscripten_set_main_loop handles frame timing) */
    g_renderer = SDL_CreateRenderer(g_window, -1, 0);
    if (!g_renderer) {
        fprintf(stderr, "FATAL: SDL_CreateRenderer failed: %s\n", SDL_GetError());
        return;
    }
    /* Force requestAnimationFrame timing mode */
    SDL_GL_SetSwapInterval(1);
    emscripten_set_main_loop_timing(EM_TIMING_RAF, 0);
    /* Web: always use ARGB8888 — avoids 16-bit format issues with WebGL */
    g_use_32bit = true;
    g_framebuffer = SDL_CreateTexture(g_renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        SCREEN_W, SCREEN_H);
    if (!g_framebuffer) {
        fprintf(stderr, "FATAL: SDL_CreateTexture failed: %s\n", SDL_GetError());
    }
#else
    g_renderer = SDL_CreateRenderer(g_window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    g_framebuffer = SDL_CreateTexture(g_renderer,
        SDL_PIXELFORMAT_ABGR1555,
        SDL_TEXTUREACCESS_STREAMING,
        SCREEN_W, SCREEN_H);
#endif

    if (g_renderer) {
        SDL_RenderSetLogicalSize(g_renderer, SCREEN_W, SCREEN_H);
    }
}

void platform_shutdown(void) {
    if (g_framebuffer) SDL_DestroyTexture(g_framebuffer);
    if (g_renderer)    SDL_DestroyRenderer(g_renderer);
    if (g_window)      SDL_DestroyWindow(g_window);
    SDL_Quit();
}

void platform_set_scale(int scale) {
    if (scale < 2) scale = 2;
    if (scale > 6) scale = 6;
    g_scale = scale;
    if (g_window) {
        SDL_SetWindowSize(g_window, SCREEN_W * g_scale, SCREEN_H * g_scale);
        SDL_SetWindowPosition(g_window,
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    }
}

void platform_toggle_fullscreen(void) {
    g_fullscreen = !g_fullscreen;
    if (g_window) {
        SDL_SetWindowFullscreen(g_window,
            g_fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
    }
}

void platform_frame_start(void) {
    /* pixels cleared by platform_clear */
}

void platform_frame_end(void) {
    if (!g_renderer || !g_framebuffer) return;

    if (g_use_32bit) {
        /* Convert 15-bit GBA colors to 32-bit ARGB8888 */
        for (int i = 0; i < SCREEN_W * SCREEN_H; i++) {
            uint16_t c = g_pixels[i];
            uint8_t r = (c & 0x001F) << 3;
            uint8_t g = ((c >> 5) & 0x1F) << 3;
            uint8_t b = ((c >> 10) & 0x1F) << 3;
            g_pixels32[i] = 0xFF000000u | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
        }
        SDL_UpdateTexture(g_framebuffer, NULL, g_pixels32,
                          SCREEN_W * sizeof(uint32_t));
    } else {
        SDL_UpdateTexture(g_framebuffer, NULL, g_pixels,
                          SCREEN_W * sizeof(uint16_t));
    }
    SDL_RenderClear(g_renderer);
    SDL_RenderCopy(g_renderer, g_framebuffer, NULL, NULL);
    SDL_RenderPresent(g_renderer);
}

/* ── Rendering ─────────────────────────────────────────── */
void platform_clear(uint16_t color) {
    color |= 0x8000; /* set alpha bit opaque */
    for (int i = 0; i < SCREEN_W * SCREEN_H; i++)
        g_pixels[i] = color;
}

static inline void put_pixel(int x, int y, uint16_t color) {
    if (x >= 0 && x < SCREEN_W && y >= 0 && y < SCREEN_H)
        g_pixels[y * SCREEN_W + x] = color | 0x8000; /* set alpha bit opaque */
}

void platform_draw_tile(int x, int y, int tile_id, int palette) {
    (void)palette;
    if (tile_id < 0 || tile_id >= 9) return;
    uint16_t bg = TILE_COLORS[tile_id][0];
    uint16_t fg = TILE_COLORS[tile_id][1];
    const uint8_t *pat = TILE_PATTERNS[tile_id];
    for (int dy = 0; dy < 8; dy++) {
        uint8_t row = pat[dy];
        for (int dx = 0; dx < 8; dx++) {
            uint16_t c = (row & (0x80 >> dx)) ? fg : bg;
            put_pixel(x + dx, y + dy, c);
        }
    }
}

/* ── 16x16 2bpp sprite helper ────────────────────────── */
static void draw_sprite_16x16(int x, int y, const uint8_t *data,
                               const uint16_t colors[][3], int scale,
                               bool flip_h) {
    for (int dy = 0; dy < 16; dy++) {
        const uint8_t *row_bytes = &data[dy * 4];
        uint16_t outline = colors[dy][0];
        uint16_t main_c  = colors[dy][1];
        uint16_t accent  = colors[dy][2];
        for (int dx = 0; dx < 16; dx++) {
            int sx = flip_h ? (15 - dx) : dx;
            int byte_idx = sx / 4;
            int bit_shift = 6 - (sx % 4) * 2;
            int pixel = (row_bytes[byte_idx] >> bit_shift) & 0x03;
            if (pixel == 0) continue; /* transparent */
            uint16_t c;
            if      (pixel == 1) c = outline;
            else if (pixel == 2) c = main_c;
            else                 c = accent;
            for (int sy = 0; sy < scale; sy++)
                for (int sxx = 0; sxx < scale; sxx++)
                    put_pixel(x + dx * scale + sxx, y + dy * scale + sy, c);
        }
    }
}

void platform_draw_sprite(int x, int y, int sprite_id,
                          int palette, bool flip_h) {
    (void)palette;

    /* Use 16x16 2bpp sprites — offset to align feet with tile */
    if (sprite_id <= 7) {
        int dir = sprite_id / 2;
        int frame = sprite_id % 2;
        if (dir > 3) dir = 0;
        draw_sprite_16x16(x - 4, y - 8,
                          SPRITE16_PLAYER[dir][frame],
                          PLAYER16_ROW_COLORS, 1, flip_h);
        return;
    }
    if (sprite_id == 8) {
        draw_sprite_16x16(x - 4, y - 8,
                          SPRITE16_NPC,
                          NPC16_ROW_COLORS, 1, flip_h);
        return;
    }
    if (sprite_id >= 10 && sprite_id < 10 + COLOR_ENEMY_COLORS_COUNT) {
        int idx = sprite_id - 10;
        draw_sprite_16x16(x - 4, y - 8,
                          SPRITE16_ENEMIES[idx],
                          ENEMY16_ROW_COLORS[idx], 1, flip_h);
        return;
    }

    /* Fallback: old 8x8 1bpp rendering */
    {
        const uint8_t *pat = SPRITE_NPC;
        const uint16_t *row_colors = NPC_ROW_COLORS;
        for (int dy = 0; dy < 8; dy++) {
            uint8_t row = pat[dy];
            uint16_t color = row_colors[dy];
            for (int dx = 0; dx < 8; dx++) {
                int sx = flip_h ? (7 - dx) : dx;
                if (row & (0x80 >> sx)) {
                    put_pixel(x + dx, y + dy, color);
                }
            }
        }
    }
}

void platform_draw_sprite_scaled(int x, int y, int sprite_id,
                                  int palette, bool flip_h, int scale) {
    (void)palette;
    if (scale < 1) scale = 1;

    if (sprite_id <= 7) {
        int dir = sprite_id / 2;
        int frame = sprite_id % 2;
        if (dir > 3) dir = 0;
        draw_sprite_16x16(x, y,
                          SPRITE16_PLAYER[dir][frame],
                          PLAYER16_ROW_COLORS, scale, flip_h);
        return;
    }
    if (sprite_id == 8) {
        draw_sprite_16x16(x, y, SPRITE16_NPC,
                          NPC16_ROW_COLORS, scale, flip_h);
        return;
    }
    if (sprite_id >= 10 && sprite_id < 10 + COLOR_ENEMY_COLORS_COUNT) {
        int idx = sprite_id - 10;
        draw_sprite_16x16(x, y, SPRITE16_ENEMIES[idx],
                          ENEMY16_ROW_COLORS[idx], scale, flip_h);
        return;
    }
    /* Fallback: draw old 8x8 at scale */
    {
        const uint8_t *pat = SPRITE_NPC;
        const uint16_t *row_colors = NPC_ROW_COLORS;
        for (int dy = 0; dy < 8; dy++) {
            uint8_t row = pat[dy];
            uint16_t color = row_colors[dy];
            for (int dx = 0; dx < 8; dx++) {
                int sx = flip_h ? (7 - dx) : dx;
                if (row & (0x80 >> sx)) {
                    for (int sy = 0; sy < scale; sy++)
                        for (int sxx = 0; sxx < scale; sxx++)
                            put_pixel(x + dx*scale + sxx, y + dy*scale + sy, color);
                }
            }
        }
    }
}

void platform_draw_rect(int x, int y, int w, int h, uint16_t color) {
    for (int dy = 0; dy < h; dy++)
        for (int dx = 0; dx < w; dx++)
            put_pixel(x + dx, y + dy, color);
}

/*
 * Korean text rendering via Emscripten EM_JS.
 * Renders Unicode text using browser's Canvas 2D API directly into
 * the game's pixel buffer for pixel-art style Korean text.
 */
#ifdef PLATFORM_WEB
EM_JS(void, js_draw_korean_char, (int x, int y, int codepoint,
                                   int cr, int cg, int cb,
                                   int buf_ptr, int sw, int sh), {
    /* Reuse or create a small offscreen canvas for text rendering */
    if (!Module._krCanvas) {
        Module._krCanvas = document.createElement('canvas');
        Module._krCanvas.width = 12;
        Module._krCanvas.height = 12;
        Module._krCtx = Module._krCanvas.getContext('2d');
    }
    var cvs = Module._krCanvas;
    var ctx = Module._krCtx;
    ctx.clearRect(0, 0, 12, 12);

    ctx.font = 'bold 10px sans-serif';
    ctx.fillStyle = 'white';
    ctx.textBaseline = 'top';
    ctx.fillText(String.fromCodePoint(codepoint), 0, 1);

    var imgData = ctx.getImageData(0, 0, 10, 11);
    var data = imgData.data;

    /* Pack the target color as BGR555 with alpha bit */
    var bgr = ((cb >> 3) << 10) | ((cg >> 3) << 5) | (cr >> 3) | 0x8000;

    for (var py = 0; py < 11; py++) {
        for (var px = 0; px < 10; px++) {
            var idx = (py * 10 + px) * 4;
            if (data[idx + 3] > 80) {
                var sx = x + px;
                var sy = y + py;
                if (sx >= 0 && sx < sw && sy >= 0 && sy < sh) {
                    HEAP16[(buf_ptr >> 1) + sy * sw + sx] = bgr;
                }
            }
        }
    }
});
#endif /* PLATFORM_WEB */

void platform_draw_text(int x, int y, const char *text, uint16_t color) {
    int cx = x;
    while (*text) {
        unsigned char c = (unsigned char)*text;

        if (c < 0x80) {
            /* ASCII — use existing 4x6 bitmap font */
            if (c == '\n') {
                cx = x;
                y += 8;
                text++;
                continue;
            }
            int ch = c - 32;
            if (ch >= 0 && ch < 96) {
                const uint8_t *gdata = g_font_data[ch];
                uint32_t glyph = ((uint32_t)gdata[0] << 16)
                               | ((uint32_t)gdata[1] << 8)
                               |  (uint32_t)gdata[2];
                for (int row = 0; row < 6; row++) {
                    for (int col = 0; col < 4; col++) {
                        if (glyph & (1u << (23 - row * 4 - col))) {
                            put_pixel(cx + col, y + row, color);
                        }
                    }
                }
            }
            cx += 5;
            text++;
        }
#ifdef PLATFORM_WEB
        else if (c >= 0xE0 && c <= 0xEF && text[1] && text[2]) {
            /* 3-byte UTF-8: covers Korean (U+AC00-U+D7A3), punctuation, etc. */
            int cp = ((c & 0x0F) << 12)
                   | (((unsigned char)text[1] & 0x3F) << 6)
                   | ((unsigned char)text[2] & 0x3F);
            /* Convert BGR555 to RGB components */
            uint8_t r = (color & 0x001F) << 3;
            uint8_t g = ((color >> 5) & 0x1F) << 3;
            uint8_t b = ((color >> 10) & 0x1F) << 3;
            js_draw_korean_char(cx, y, cp, r, g, b,
                                (int)(intptr_t)g_pixels, SCREEN_W, SCREEN_H);
            cx += 11; /* Korean char advance */
            text += 3;
        }
#endif
        else {
            /* Skip unsupported multi-byte UTF-8 */
            if (c >= 0xF0) text += 4;
            else if (c >= 0xE0) text += 3;
            else if (c >= 0xC0) text += 2;
            else text++;
        }
    }
}

#endif /* PLATFORM_SDL */
