/**
 * platform.h — Hardware Abstraction Layer
 *
 * All platform-specific functions are declared here.
 * Implementations live in platform/gba/ or platform/sdl/.
 */
#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>
#include <stdbool.h>

/* ── Display ───────────────────────────────────────────── */
#define SCREEN_W  240
#define SCREEN_H  160

void platform_init(void);
void platform_shutdown(void);
void platform_frame_start(void);
void platform_frame_end(void);   /* vsync / present */

/* ── Rendering ─────────────────────────────────────────── */
void platform_clear(uint16_t color);
void platform_draw_tile(int x, int y, int tile_id, int palette);
void platform_draw_sprite(int x, int y, int sprite_id, int palette, bool flip_h);
void platform_draw_sprite_scaled(int x, int y, int sprite_id, int palette, bool flip_h, int scale);
void platform_draw_rect(int x, int y, int w, int h, uint16_t color);
void platform_draw_text(int x, int y, const char *text, uint16_t color);

/* ── Input ─────────────────────────────────────────────── */
/*
 * GBA hardware key bits: A=0, B=1, Select=2, Start=3,
 * Right=4, Left=5, Up=6, Down=7, R=8, L=9.
 *
 * On GBA builds, libtonc already #defines KEY_A..KEY_L with
 * these exact values, so we skip our own enum to avoid
 * redefinition conflicts.  On PC/SDL builds we define them.
 */
#ifndef PLATFORM_GBA
typedef enum {
    KEY_A      = (1 << 0),
    KEY_B      = (1 << 1),
    KEY_SELECT = (1 << 2),
    KEY_START  = (1 << 3),
    KEY_RIGHT  = (1 << 4),
    KEY_LEFT   = (1 << 5),
    KEY_UP     = (1 << 6),
    KEY_DOWN   = (1 << 7),
    KEY_R      = (1 << 8),
    KEY_L      = (1 << 9),
} KeyMask;
#endif /* !PLATFORM_GBA */

void     platform_poll_input(void);
uint16_t platform_keys_held(void);     /* currently pressed */
uint16_t platform_keys_pressed(void);  /* just pressed this frame */

/* ── Audio ─────────────────────────────────────────────── */
void platform_play_bgm(int track_id);
void platform_stop_bgm(void);
void platform_play_sfx(int sfx_id);

/* ── Save/Load ─────────────────────────────────────────── */
#define SAVE_SIZE  4096

bool platform_save(const void *data, int size);
bool platform_load(void *data, int size);

#endif /* PLATFORM_H */
