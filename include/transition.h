/**
 * transition.h — Screen Transitions
 */
#ifndef TRANSITION_H
#define TRANSITION_H

#include <stdint.h>
#include <stdbool.h>
#include "game.h"

/* ── Transition Types ─────────────────────────────────── */
typedef enum {
    TRANS_NONE,
    TRANS_FADE_OUT,
    TRANS_FADE_IN,
} TransType;

/* ── Transition State ─────────────────────────────────── */
typedef struct {
    TransType  type;
    int16_t    timer;
    int16_t    duration;
    bool       active;
    GameState  callback_state;  /* state to switch to after transition */
} Transition;

/* ── Functions ────────────────────────────────────────── */
void transition_start(TransType type, int duration, GameState callback_state);
bool transition_update(GameContext *ctx);  /* returns true if transitioning */
void transition_render(void);

#endif /* TRANSITION_H */
