/**
 * main.c — SDL2 entry point for PC/Steam build
 */
#include "game.h"
#include "platform.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Defined in input.c */
extern bool g_quit_requested;
extern bool g_fullscreen_toggle;

/* Defined in render.c */
extern void platform_set_scale(int scale);
extern void platform_toggle_fullscreen(void);

/* Defined in settings.c */
extern void settings_load(int *scale, bool *fullscreen, int *volume);
extern void settings_save(int scale, bool fullscreen, int volume);

/* Target frame time: 60fps = ~16.67ms per frame */
#define TARGET_FPS      60
#define FRAME_TIME_MS   (1000 / TARGET_FPS)

static void print_usage(const char *prog) {
    printf("Usage: %s [options]\n", prog);
    printf("  --fullscreen     Start in fullscreen mode\n");
    printf("  --scale N        Window scale factor (2-6, default: 3)\n");
    printf("  --help           Show this help message\n");
}

int main(int argc, char *argv[]) {
    int  arg_scale      = -1;  /* -1 = use settings file default */
    bool arg_fullscreen = false;
    bool has_fullscreen_arg = false;

    /* Parse command-line arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--fullscreen") == 0) {
            arg_fullscreen = true;
            has_fullscreen_arg = true;
        } else if (strcmp(argv[i], "--scale") == 0 && i + 1 < argc) {
            int s = atoi(argv[++i]);
            if (s >= 2 && s <= 6) {
                arg_scale = s;
            } else {
                fprintf(stderr, "Error: --scale must be 2-6\n");
                return 1;
            }
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    /* Load saved settings */
    int  cfg_scale = 3;
    bool cfg_fullscreen = false;
    int  cfg_volume = 80;
    settings_load(&cfg_scale, &cfg_fullscreen, &cfg_volume);

    /* Command-line args override saved settings */
    int  use_scale      = (arg_scale > 0) ? arg_scale : cfg_scale;
    bool use_fullscreen = has_fullscreen_arg ? arg_fullscreen : cfg_fullscreen;

    platform_init();
    platform_set_scale(use_scale);
    if (use_fullscreen) {
        platform_toggle_fullscreen();
    }

    GameContext ctx;
    game_init(&ctx);

    while (ctx.running) {
        Uint32 frame_start = SDL_GetTicks();

        /* Check for quit from SDL events */
        if (g_quit_requested) {
            ctx.running = false;
            break;
        }

        /* Handle fullscreen toggle (F11) */
        if (g_fullscreen_toggle) {
            platform_toggle_fullscreen();
        }

        game_update(&ctx);
        game_render(&ctx);

        /* Frame rate limiter */
        Uint32 frame_time = SDL_GetTicks() - frame_start;
        if (frame_time < FRAME_TIME_MS) {
            SDL_Delay(FRAME_TIME_MS - frame_time);
        }
    }

    /* Save current settings on exit */
    settings_save(use_scale, use_fullscreen, cfg_volume);

    platform_shutdown();
    return 0;
}
