/**
 * input.c — SDL2 keyboard and gamepad input
 */
#include "platform.h"

#ifdef PLATFORM_SDL

#include <SDL2/SDL.h>

#ifdef PLATFORM_WEB
#include <emscripten.h>
#endif

/* Global quit flag — set by SDL_QUIT or other exit triggers */
bool g_quit_requested = false;

#ifdef PLATFORM_WEB
/* Touch input from JavaScript — Emscripten's keyboard hooks don't see
 * programmatically dispatched KeyboardEvents, so JS calls these directly. */
static uint16_t g_touch_keys = 0;

EMSCRIPTEN_KEEPALIVE
void web_key_down(int key_mask) {
    g_touch_keys |= (uint16_t)key_mask;
}

EMSCRIPTEN_KEEPALIVE
void web_key_up(int key_mask) {
    g_touch_keys &= ~(uint16_t)key_mask;
}
#endif

/* Fullscreen toggle flag — set by F11 key */
bool g_fullscreen_toggle = false;

static uint16_t g_keys_current;
static uint16_t g_keys_previous;

void platform_poll_input(void) {
    SDL_Event ev;
    g_keys_previous = g_keys_current;
    g_fullscreen_toggle = false;

    while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_QUIT) {
            g_quit_requested = true;
        }
        if (ev.type == SDL_KEYDOWN && ev.key.keysym.scancode == SDL_SCANCODE_F11) {
            g_fullscreen_toggle = true;
        }
    }

    const uint8_t *kb = SDL_GetKeyboardState(NULL);
    g_keys_current = 0;

    if (kb[SDL_SCANCODE_Z] || kb[SDL_SCANCODE_J])
        g_keys_current |= KEY_A;
    if (kb[SDL_SCANCODE_X] || kb[SDL_SCANCODE_K])
        g_keys_current |= KEY_B;
    if (kb[SDL_SCANCODE_RSHIFT] || kb[SDL_SCANCODE_BACKSPACE])
        g_keys_current |= KEY_SELECT;
    if (kb[SDL_SCANCODE_RETURN] || kb[SDL_SCANCODE_ESCAPE])
        g_keys_current |= KEY_START;
    if (kb[SDL_SCANCODE_RIGHT] || kb[SDL_SCANCODE_D])
        g_keys_current |= KEY_RIGHT;
    if (kb[SDL_SCANCODE_LEFT] || kb[SDL_SCANCODE_A])
        g_keys_current |= KEY_LEFT;
    if (kb[SDL_SCANCODE_UP] || kb[SDL_SCANCODE_W])
        g_keys_current |= KEY_UP;
    if (kb[SDL_SCANCODE_DOWN] || kb[SDL_SCANCODE_S])
        g_keys_current |= KEY_DOWN;
    if (kb[SDL_SCANCODE_E])
        g_keys_current |= KEY_R;
    if (kb[SDL_SCANCODE_Q])
        g_keys_current |= KEY_L;

#ifdef PLATFORM_WEB
    /* Merge touch input from JavaScript */
    g_keys_current |= g_touch_keys;
#endif

    /* Gamepad support */
    SDL_GameController *pad = NULL;
    for (int i = 0; i < SDL_NumJoysticks(); i++) {
        if (SDL_IsGameController(i)) {
            pad = SDL_GameControllerOpen(i);
            break;
        }
    }
    if (pad) {
        if (SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_A))
            g_keys_current |= KEY_A;
        if (SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_B))
            g_keys_current |= KEY_B;
        if (SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_BACK))
            g_keys_current |= KEY_SELECT;
        if (SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_START))
            g_keys_current |= KEY_START;
        if (SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_UP))
            g_keys_current |= KEY_UP;
        if (SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_DOWN))
            g_keys_current |= KEY_DOWN;
        if (SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_LEFT))
            g_keys_current |= KEY_LEFT;
        if (SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_RIGHT))
            g_keys_current |= KEY_RIGHT;
        if (SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER))
            g_keys_current |= KEY_R;
        if (SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_LEFTSHOULDER))
            g_keys_current |= KEY_L;
        SDL_GameControllerClose(pad);
    }
}

uint16_t platform_keys_held(void) {
    return g_keys_current;
}

uint16_t platform_keys_pressed(void) {
    return g_keys_current & ~g_keys_previous;
}

#endif /* PLATFORM_SDL */
