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

/* ── Shake Offset Query (Sprint 23) ──────────────────── */
int fx_get_shake_offset(void) {
    for (int i = 0; i < MAX_EFFECTS; i++) {
        if (s_effects[i].active && s_effects[i].type == FX_ENEMY_SHAKE) {
            /* Oscillate: -3, +3, -2, +2, -1, +1, 0... */
            int t = s_effects[i].timer;
            int amp = 3 - t / 2;
            if (amp < 0) amp = 0;
            return (t % 2 == 0) ? amp : -amp;
        }
    }
    return 0;
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

        /* ── Sprint 23: Fireball — expanding orange/red circle ── */
        case FX_FIREBALL: {
            int radius = 2 + fx->timer / 2;
            if (radius > 12) radius = 12;
            int cx = fx->x;
            int cy = fx->y;
            /* Draw concentric circles: outer orange, inner yellow */
            for (int dy = -radius; dy <= radius; dy++) {
                for (int dx = -radius; dx <= radius; dx++) {
                    int dist2 = dx*dx + dy*dy;
                    if (dist2 <= radius * radius) {
                        uint16_t c;
                        if (dist2 < (radius/2) * (radius/2))
                            c = 0x03FF; /* bright yellow center */
                        else if (dist2 < (radius*3/4) * (radius*3/4))
                            c = 0x02BF; /* orange mid */
                        else
                            c = 0x001F; /* red outer */
                        int px = cx + dx;
                        int py = cy + dy;
                        if (px >= 0 && px < SCREEN_W && py >= 0 && py < SCREEN_H)
                            platform_draw_rect(px, py, 1, 1, c);
                    }
                }
            }
            /* Trailing sparks */
            if (fx->timer < 10) {
                for (int s = 0; s < 3; s++) {
                    int sx = cx - 5 + (int)((fx->timer * 3 + s * 7) % 10);
                    int sy = cy + 3 + s * 2;
                    platform_draw_rect(sx, sy, 1, 1, 0x03FF);
                }
            }
            break;
        }

        /* ── Sprint 23: Ice Shards — blue crystal fragments ── */
        case FX_ICE_SHARDS: {
            int num_shards = 5;
            for (int s = 0; s < num_shards; s++) {
                /* Each shard expands outward from center */
                int angle_phase = (s * 72 + fx->timer * 8) % 360;
                int dist = 2 + fx->timer;
                if (dist > 14) dist = 14;
                /* Approximate sine/cosine with simple lookup */
                int dx_table[5] = { 0, 3, 2, -2, -3 };
                int dy_table[5] = { -3, -1, 2, 2, -1 };
                int sx = fx->x + dx_table[s] * dist / 3;
                int sy = fx->y + dy_table[s] * dist / 3;
                /* Draw diamond-shaped shard */
                uint16_t ice_color = ((s + fx->timer) % 2 == 0) ? 0x7E10 : 0x7C00;
                platform_draw_rect(sx, sy - 1, 1, 3, ice_color);
                platform_draw_rect(sx - 1, sy, 3, 1, ice_color);
                /* Center bright pixel */
                platform_draw_rect(sx, sy, 1, 1, 0x7FFF);
                (void)angle_phase;
            }
            break;
        }

        /* ── Sprint 23: Lightning — vertical zigzag bolts ── */
        case FX_LIGHTNING: {
            if (fx->timer < fx->duration / 2) {
                /* Flash background white briefly */
                if (fx->timer < 3) {
                    platform_draw_rect(0, 0, SCREEN_W, SCREEN_H, 0x7FFF);
                }
                /* Draw zigzag bolt */
                int bx = fx->x;
                int by = 0;
                uint16_t bolt_color = 0x03FF; /* bright yellow */
                for (int seg = 0; seg < 8; seg++) {
                    int nx = bx + ((seg % 2 == 0) ? 4 : -4);
                    int ny = by + 9;
                    /* Draw thick line segment */
                    int dx = (nx > bx) ? 1 : -1;
                    int steps = (nx > bx) ? (nx - bx) : (bx - nx);
                    for (int step = 0; step <= steps; step++) {
                        int px = bx + step * dx;
                        int py = by + (ny - by) * step / (steps > 0 ? steps : 1);
                        platform_draw_rect(px, py, 2, 2, bolt_color);
                    }
                    bx = nx;
                    by = ny;
                }
            }
            break;
        }

        /* ── Sprint 23: Enemy Shake — screen offset effect ── */
        case FX_ENEMY_SHAKE: {
            /* This effect is checked during battle render for offset.
             * Here we draw a subtle red flash on the enemy area. */
            if (fx->timer < 6 && (fx->timer % 2 == 0)) {
                /* Red tint flash over enemy area */
                platform_draw_rect(80, 2, 80, 70, 0x001F);
            }
            break;
        }

        /* ── Sprint 23: Victory — sparkles and stars rising ── */
        case FX_VICTORY: {
            /* Rising golden sparkles across screen */
            for (int s = 0; s < 8; s++) {
                int sx = 20 + s * 28 + ((fx->timer + s * 3) % 12);
                int sy = SCREEN_H - (fx->timer * 2 + s * 8) % (SCREEN_H + 20);
                if (sy < 0 || sy >= SCREEN_H) continue;
                /* Star shape: + pattern */
                uint16_t star_c = ((s + fx->timer / 3) % 2 == 0) ? 0x03FF : 0x7FFF;
                platform_draw_rect(sx, sy - 1, 1, 3, star_c);
                platform_draw_rect(sx - 1, sy, 3, 1, star_c);
            }
            break;
        }

        } /* switch */
    }
}
