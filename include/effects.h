/**
 * effects.h — Battle visual effects system
 */
#ifndef EFFECTS_H
#define EFFECTS_H

#include <stdint.h>
#include <stdbool.h>

/* ── Effect Types ─────────────────────────────────────── */
typedef enum {
    FX_DAMAGE_NUM,
    FX_HIT_FLASH,
    FX_HEAL,
    FX_LEVEL_UP,
    FX_FADE,
} EffectType;

/* ── Effect Instance ──────────────────────────────────── */
typedef struct {
    EffectType type;
    int16_t    x;
    int16_t    y;
    int16_t    value;     /* damage/heal number */
    int16_t    timer;
    int16_t    duration;
    bool       active;
} Effect;

#define MAX_EFFECTS 8

/* ── Functions ────────────────────────────────────────── */
void fx_init(void);
void fx_spawn(EffectType type, int x, int y, int value, int duration);
void fx_update(void);
void fx_render(void);

#endif /* EFFECTS_H */
