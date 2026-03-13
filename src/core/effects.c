/**
 * effects.c — Battle visual effects implementation
 * Platform-independent: uses platform.h for rendering.
 */
#include "effects.h"
#include "platform.h"
#include <string.h>
#include <stdio.h>

/* ── Effect Pool ──────────────────────────────────────── */
static Effect s_effects[MAX_EFFECTS];

/* ── Initialization ───────────────────────────────────── */
void fx_init(void) {
    memset(s_effects, 0, sizeof(s_effects));
}

/* ── Spawn Effect ─────────────────────────────────────── */
void fx_spawn(EffectType type, int x, int y, int value, int duration) {
    for (int i = 0; i < MAX_EFFECTS; i++) {
        if (!s_effects[i].active) {
            s_effects[i].type     = type;
            s_effects[i].x        = (int16_t)x;
            s_effects[i].y        = (int16_t)y;
            s_effects[i].value    = (int16_t)value;
            s_effects[i].timer    = 0;
            s_effects[i].duration = (int16_t)duration;
            s_effects[i].active   = true;
            return;
        }
    }
    /* No free slot — effect is silently dropped */
}

/* ── Update ───────────────────────────────────────────── */
void fx_update(void) {
    for (int i = 0; i < MAX_EFFECTS; i++) {
        if (!s_effects[i].active) continue;

        s_effects[i].timer++;
        if (s_effects[i].timer >= s_effects[i].duration) {
            s_effects[i].active = false;
        }
    }
}

/* ── Render ───────────────────────────────────────────── */
void fx_render(void) {
    for (int i = 0; i < MAX_EFFECTS; i++) {
        if (!s_effects[i].active) continue;

        Effect *fx = &s_effects[i];

        switch (fx->type) {

        /* ── Damage Number: floats upward ────────────── */
        case FX_DAMAGE_NUM: {
            char num_text[12];
            snprintf(num_text, sizeof(num_text), "%d", fx->value);
            /* Float upward: y decreases over time */
            int draw_y = fx->y - (fx->timer / 2);
            /* White (0x7FFF) for damage, green (0x03E0) for heal */
            uint16_t color = (fx->value > 0) ? 0x7FFF : 0x03E0;
            platform_draw_text(fx->x, draw_y, num_text, color);
            break;
        }

        /* ── Hit Flash: full-screen white overlay ────── */
        case FX_HIT_FLASH: {
            /* Flash white for 3 frames */
            if (fx->timer < 3) {
                platform_draw_rect(0, 0, SCREEN_W, SCREEN_H, 0x7FFF);
            }
            break;
        }

        /* ── Heal: green sparkle rects around target ─── */
        case FX_HEAL: {
            /* Draw small green rectangles around target position */
            int phase = fx->timer % 8;
            int offsets[4][2] = {
                { -6, -4 }, {  6, -4 },
                { -4,  6 }, {  4,  6 },
            };
            for (int j = 0; j < 4; j++) {
                /* Stagger sparkles: each appears at different phase */
                if ((fx->timer + j * 2) % 8 < 4) {
                    int sx = fx->x + offsets[j][0] + (phase < 4 ? -1 : 1);
                    int sy = fx->y + offsets[j][1] - (fx->timer / 3);
                    platform_draw_rect(sx, sy, 3, 3, 0x03E0); /* green */
                }
            }
            (void)phase;
            break;
        }

        /* ── Level Up: gold text ─────────────────────── */
        case FX_LEVEL_UP: {
            /* "LEVEL UP!" in gold (0x03FF = yellow-ish on GBA) */
            /* Text rises slightly then holds */
            int draw_y = fx->y;
            if (fx->timer < 10) {
                draw_y = fx->y - fx->timer;
            } else {
                draw_y = fx->y - 10;
            }
            /* Fade: only show while timer < duration (blink near end) */
            if (fx->timer > fx->duration - 10 && (fx->timer % 4 < 2)) {
                break; /* blink off */
            }
            /* Gold color: 0x03FF (GBA BGR555) */
            platform_draw_text(fx->x, draw_y, "LEVEL UP!", 0x03FF);
            break;
        }

        /* ── Fade: progressive black overlay ─────────── */
        case FX_FADE: {
            /* Progressively cover screen with dark rectangles */
            /* Use horizontal bands that grow over time */
            int progress = (fx->timer * SCREEN_H) / fx->duration;
            if (progress > SCREEN_H) progress = SCREEN_H;
            platform_draw_rect(0, 0, SCREEN_W, progress, 0x0000);
            break;
        }

        } /* switch */
    }
}
