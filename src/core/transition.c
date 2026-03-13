/**
 * transition.c — Screen Transitions
 * Platform-independent: uses platform_draw_rect for fade overlay.
 */
#include "transition.h"
#include "platform.h"

/* ── Global Transition State ──────────────────────────── */
static Transition g_trans = { TRANS_NONE, 0, 0, false, STATE_TITLE };

/* ── Start a Transition ───────────────────────────────── */
void transition_start(TransType type, int duration, GameState callback_state) {
    g_trans.type           = type;
    g_trans.timer          = (int16_t)duration;
    g_trans.duration       = (int16_t)duration;
    g_trans.active         = true;
    g_trans.callback_state = callback_state;
}

/* ── Update Transition ────────────────────────────────── */
bool transition_update(GameContext *ctx) {
    if (!g_trans.active) {
        return false;
    }

    g_trans.timer--;

    if (g_trans.timer <= 0) {
        if (g_trans.type == TRANS_FADE_OUT) {
            /* Fade out complete: switch state, then start fade in */
            ctx->state = g_trans.callback_state;
            transition_start(TRANS_FADE_IN, g_trans.duration, g_trans.callback_state);
        } else {
            /* Fade in complete or TRANS_NONE: done */
            g_trans.active = false;
            g_trans.type   = TRANS_NONE;
        }
    }

    return true;
}

/* ── Render Transition Overlay ────────────────────────── */
void transition_render(void) {
    if (!g_trans.active || g_trans.duration <= 0) {
        return;
    }

    /*
     * Draw black overlay strips to simulate fade.
     * TRANS_FADE_OUT: progress goes from 0 to 1 (getting darker)
     * TRANS_FADE_IN:  progress goes from 1 to 0 (getting lighter)
     */
    int elapsed = g_trans.duration - g_trans.timer;

    int coverage;  /* how many scanlines out of SCREEN_H to cover with black */
    if (g_trans.type == TRANS_FADE_OUT) {
        /* As elapsed increases, cover more of the screen */
        coverage = (elapsed * SCREEN_H) / g_trans.duration;
    } else {
        /* TRANS_FADE_IN: as elapsed increases, cover less of the screen */
        coverage = ((g_trans.duration - elapsed) * SCREEN_H) / g_trans.duration;
    }

    if (coverage <= 0) {
        return;
    }
    if (coverage > SCREEN_H) {
        coverage = SCREEN_H;
    }

    /*
     * Draw horizontal strips evenly distributed across the screen.
     * Use 8 strips to simulate a venetian-blind fade effect.
     */
    int num_strips = 8;
    int strip_max_h = SCREEN_H / num_strips;  /* max height per strip */
    int strip_h = (coverage * strip_max_h) / SCREEN_H;

    if (strip_h < 1 && coverage > 0) {
        strip_h = 1;
    }

    for (int i = 0; i < num_strips; i++) {
        int sy = i * strip_max_h;
        platform_draw_rect(0, sy, SCREEN_W, strip_h, 0x0000);
    }
}
