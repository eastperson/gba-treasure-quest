/**
 * platform_web.c — Web-specific platform functions
 *
 * Provides:
 *   - Save/Load via browser localStorage (through Emscripten)
 *   - Audio stubs (SDL2_mixer not available in Emscripten builds)
 *   - Touch input injection for mobile devices
 *
 * Compiled with PLATFORM_SDL + PLATFORM_WEB.
 * This file replaces sdl/sound.c (which bundles audio+save).
 */
#include "platform.h"

#ifdef PLATFORM_WEB

#include <SDL2/SDL.h>
#include <string.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

/* ── Audio (Web Audio API via JavaScript) ────────────── */

void platform_play_bgm(int track_id) {
#ifdef __EMSCRIPTEN__
    EM_ASM({
        if (typeof TQ_Audio !== 'undefined') TQ_Audio.playBgm($0);
    }, track_id);
#else
    (void)track_id;
#endif
}

void platform_stop_bgm(void) {
#ifdef __EMSCRIPTEN__
    EM_ASM({
        if (typeof TQ_Audio !== 'undefined') TQ_Audio.stopBgm();
    });
#endif
}

void platform_play_sfx(int sfx_id) {
#ifdef __EMSCRIPTEN__
    EM_ASM({
        if (typeof TQ_Audio !== 'undefined') TQ_Audio.playSfx($0);
    }, sfx_id);
#else
    (void)sfx_id;
#endif
}

/* ── Save/Load via localStorage ──────────────────────── */

/*
 * We use Emscripten's EM_ASM to call localStorage directly.
 * Save data is base64-encoded for safe string storage.
 */

static const char BASE64_CHARS[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static int base64_encode(const uint8_t *src, int len, char *dst, int dst_size) {
    int i = 0, j = 0;
    while (i < len && j < dst_size - 4) {
        uint32_t a = (i < len) ? src[i++] : 0;
        uint32_t b = (i < len) ? src[i++] : 0;
        uint32_t c = (i < len) ? src[i++] : 0;
        uint32_t triple = (a << 16) | (b << 8) | c;

        int pad = (i - 1 >= len) ? ((i - 2 >= len) ? 2 : 1) : 0;
        dst[j++] = BASE64_CHARS[(triple >> 18) & 0x3F];
        dst[j++] = BASE64_CHARS[(triple >> 12) & 0x3F];
        dst[j++] = (pad >= 2) ? '=' : BASE64_CHARS[(triple >> 6) & 0x3F];
        dst[j++] = (pad >= 1) ? '=' : BASE64_CHARS[triple & 0x3F];
    }
    dst[j] = '\0';
    return j;
}

static int base64_decode_char(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

static int base64_decode(const char *src, uint8_t *dst, int dst_size) {
    int len = strlen(src);
    int i = 0, j = 0;
    while (i < len && j < dst_size) {
        int a = (i < len) ? base64_decode_char(src[i++]) : 0;
        int b = (i < len) ? base64_decode_char(src[i++]) : 0;
        int c = (i < len) ? base64_decode_char(src[i++]) : 0;
        int d = (i < len) ? base64_decode_char(src[i++]) : 0;
        if (a < 0) a = 0; if (b < 0) b = 0;
        if (c < 0) c = 0; if (d < 0) d = 0;

        uint32_t triple = (a << 18) | (b << 12) | (c << 6) | d;
        if (j < dst_size) dst[j++] = (triple >> 16) & 0xFF;
        if (j < dst_size && src[i-1] != '=') dst[j++] = (triple >> 8) & 0xFF;
        if (j < dst_size && src[i-1] != '=' && (i < 2 || src[i-2] != '='))
            dst[j++] = triple & 0xFF;
    }
    return j;
}

bool platform_save(const void *data, int size) {
#ifdef __EMSCRIPTEN__
    /* Encode to base64 */
    int b64_size = ((size + 2) / 3) * 4 + 1;
    char *b64 = (char *)malloc(b64_size);
    if (!b64) return false;
    base64_encode((const uint8_t *)data, size, b64, b64_size);

    EM_ASM({
        try {
            localStorage.setItem('treasure_quest_save', UTF8ToString($0));
        } catch(e) {}
    }, b64);

    free(b64);
    return true;
#else
    (void)data; (void)size;
    return false;
#endif
}

bool platform_load(void *data, int size) {
#ifdef __EMSCRIPTEN__
    char *b64 = (char *)EM_ASM_PTR({
        var s = localStorage.getItem('treasure_quest_save');
        if (!s) return 0;
        var len = lengthBytesUTF8(s) + 1;
        var buf = _malloc(len);
        stringToUTF8(s, buf, len);
        return buf;
    });

    if (!b64) return false;

    int decoded = base64_decode(b64, (uint8_t *)data, size);
    free(b64);
    return decoded >= size;
#else
    (void)data; (void)size;
    return false;
#endif
}

#endif /* PLATFORM_WEB */
