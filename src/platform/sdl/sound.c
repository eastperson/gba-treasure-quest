/**
 * sound.c — SDL2_mixer audio implementation
 */
#include "platform.h"

#ifdef PLATFORM_SDL

#include <SDL2/SDL.h>
/* #include <SDL2/SDL_mixer.h> — enable when audio assets exist */

void platform_play_bgm(int track_id) {
    (void)track_id;
    /* TODO: load and play music from data/sound/ */
}

void platform_stop_bgm(void) {
    /* TODO */
}

void platform_play_sfx(int sfx_id) {
    (void)sfx_id;
    /* TODO: load and play SFX from data/sound/ */
}

/* ── Save/Load (file-based) ────────────────────────────── */
#include <stdio.h>

static const char *SAVE_PATH = "treasure_quest.sav";

bool platform_save(const void *data, int size) {
    FILE *f = fopen(SAVE_PATH, "wb");
    if (!f) return false;
    bool ok = (int)fwrite(data, 1, size, f) == size;
    fclose(f);
    return ok;
}

bool platform_load(void *data, int size) {
    FILE *f = fopen(SAVE_PATH, "rb");
    if (!f) return false;
    bool ok = (int)fread(data, 1, size, f) == size;
    fclose(f);
    return ok;
}

#endif /* PLATFORM_SDL */
