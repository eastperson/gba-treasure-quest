/**
 * main.c — Emscripten (WebAssembly) entry point
 *
 * Uses emscripten_set_main_loop for browser-friendly frame scheduling.
 * SDL2 rendering and input are handled by the shared SDL platform code
 * (compiled with PLATFORM_SDL).
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

static void main_loop_iteration(void) {
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

    platform_init();
    game_init(&g_ctx);

#ifdef __EMSCRIPTEN__
    /* Let the browser drive the frame rate (requestAnimationFrame).
     * 0 = use browser's refresh rate, 1 = simulate infinite loop */
    emscripten_set_main_loop(main_loop_iteration, 0, 1);
#else
    /* Fallback for non-emscripten builds (shouldn't happen) */
    while (g_ctx.running && !g_quit_requested) {
        main_loop_iteration();
    }
    platform_shutdown();
#endif

    return 0;
}
