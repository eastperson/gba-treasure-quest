/**
 * main.c — Emscripten (WebAssembly) entry point
 *
 * Uses emscripten_set_main_loop for browser-friendly frame scheduling.
 * SDL2 rendering and input are handled by the shared SDL platform code
 * (compiled with PLATFORM_SDL).
 *
 * Frame rate is capped at 60fps via time accumulator to prevent
 * double-speed on 120Hz+ monitors (RAF fires at display refresh rate).
 */
#include "game.h"
#include "platform.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include <SDL2/SDL.h>

/* Defined in sdl/input.c */
extern bool g_quit_requested;

/* Global game context for the main loop callback */
static GameContext g_ctx;
static bool g_initialized = false;

/* Frame rate limiter: cap at 60fps regardless of display refresh rate */
static double g_last_time = 0.0;
static double g_accum = 0.0;
#define TARGET_FRAME_MS (1000.0 / 60.0)  /* 16.667ms per frame */

static void main_loop_iteration(void) {
    /* Deferred init: SDL_CreateRenderer calls emscripten_set_main_loop_timing
     * internally, so emscripten_set_main_loop must already exist first. */
    if (!g_initialized) {
        platform_init();
        game_init(&g_ctx);
        g_initialized = true;
        g_last_time = emscripten_get_now();
        return;
    }

    /* Time accumulator: only run a game tick when enough time has passed */
    double now = emscripten_get_now();
    double dt = now - g_last_time;
    g_last_time = now;
    g_accum += dt;

    if (g_accum < TARGET_FRAME_MS) {
        return;  /* Not enough time for a frame yet — skip */
    }
    g_accum -= TARGET_FRAME_MS;
    /* Prevent death spiral: if we fall behind, don't try to catch up */
    if (g_accum > TARGET_FRAME_MS * 3.0) {
        g_accum = 0.0;
    }

    if (g_quit_requested || !g_ctx.running) {
#ifdef __EMSCRIPTEN__
        emscripten_cancel_main_loop();
#endif
        platform_shutdown();
        return;
    }

    game_update(&g_ctx);
    game_render(&g_ctx);
}

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

#ifdef __EMSCRIPTEN__
    /* Establish main loop FIRST — SDL_CreateRenderer needs it to exist
     * before it can call emscripten_set_main_loop_timing internally.
     * Actual init happens on first iteration via deferred init above. */
    emscripten_set_main_loop(main_loop_iteration, 0, 1);
#else
    platform_init();
    game_init(&g_ctx);
    while (g_ctx.running && !g_quit_requested) {
        game_update(&g_ctx);
        game_render(&g_ctx);
    }
    platform_shutdown();
#endif

    return 0;
}
