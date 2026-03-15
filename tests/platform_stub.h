/**
 * platform_stub.h — Stub implementations for platform_* functions
 * Used by unit tests compiled with -DPLATFORM_TEST
 */
#ifndef PLATFORM_STUB_H
#define PLATFORM_STUB_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* ── Rendering stubs ──────────────────────────────────── */
void platform_init(void) {}
void platform_shutdown(void) {}
void platform_frame_start(void) {}
void platform_frame_end(void) {}
void platform_clear(uint16_t color) { (void)color; }
void platform_draw_tile(int x, int y, int tile_id, int palette) {
    (void)x; (void)y; (void)tile_id; (void)palette;
}
void platform_draw_sprite(int x, int y, int sprite_id, int palette, bool flip_h) {
    (void)x; (void)y; (void)sprite_id; (void)palette; (void)flip_h;
}
void platform_draw_sprite_scaled(int x, int y, int sprite_id, int palette, bool flip_h, int scale) {
    (void)x; (void)y; (void)sprite_id; (void)palette; (void)flip_h; (void)scale;
}
void platform_draw_rect(int x, int y, int w, int h, uint16_t color) {
    (void)x; (void)y; (void)w; (void)h; (void)color;
}
void platform_draw_text(int x, int y, const char *text, uint16_t color) {
    (void)x; (void)y; (void)text; (void)color;
}

/* ── Input stubs ──────────────────────────────────────── */
void     platform_poll_input(void) {}
uint16_t platform_keys_held(void) { return 0; }
uint16_t platform_keys_pressed(void) { return 0; }

/* ── Audio stubs ──────────────────────────────────────── */
void platform_play_bgm(int track_id) { (void)track_id; }
void platform_stop_bgm(void) {}
void platform_play_sfx(int sfx_id) { (void)sfx_id; }

/* ── Save/Load stubs ─────────────────────────────────── */
static uint8_t g_stub_save_buf[4096];
static int     g_stub_save_size = 0;

bool platform_save(const void *data, int size) {
    if (size > (int)sizeof(g_stub_save_buf)) return false;
    memcpy(g_stub_save_buf, data, size);
    g_stub_save_size = size;
    return true;
}

bool platform_load(void *data, int size) {
    if (g_stub_save_size == 0) return false;
    if (size > g_stub_save_size) size = g_stub_save_size;
    memcpy(data, g_stub_save_buf, size);
    return true;
}

/* ── Effects stubs ───────────────────────────────────── */
#include "effects.h"
void fx_init(void) {}
void fx_spawn(EffectType type, int x, int y, int value, int duration) {
    (void)type; (void)x; (void)y; (void)value; (void)duration;
}
void fx_update(void) {}
void fx_render(void) {}
int  fx_get_shake_offset(void) { return 0; }

#endif /* PLATFORM_STUB_H */
