/**
 * sound.c — GBA audio (DirectSound) + SRAM save/load
 */
#include "platform.h"

#ifdef PLATFORM_GBA

#include <tonc.h>
#include <string.h>

void platform_play_bgm(int track_id) {
    (void)track_id;
    /* TODO: DirectSound DMA playback */
}

void platform_stop_bgm(void) {
    /* TODO */
}

void platform_play_sfx(int sfx_id) {
    (void)sfx_id;
    /* TODO */
}

/* ── Save/Load (SRAM) ──────────────────────────────────── */
#define SRAM_BASE  ((volatile uint8_t *)0x0E000000)

bool platform_save(const void *data, int size) {
    const uint8_t *src = (const uint8_t *)data;
    for (int i = 0; i < size && i < SAVE_SIZE; i++)
        SRAM_BASE[i] = src[i];
    return true;
}

bool platform_load(void *data, int size) {
    uint8_t *dst = (uint8_t *)data;
    for (int i = 0; i < size && i < SAVE_SIZE; i++)
        dst[i] = SRAM_BASE[i];
    return true;
}

#endif /* PLATFORM_GBA */
