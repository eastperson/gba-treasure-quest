/**
 * menu.c — In-game menu system
 * Platform-independent: uses platform_draw_* for rendering.
 */
#include "menu.h"
#include "save.h"
#include "party.h"
#include "platform.h"
#include <stdio.h>

/* ── Menu item labels ────────────────────────────────── */
static const char *menu_labels[MENU_COUNT] = {
    "Items",
    "Status",
    "Save",
    "Load",
    "Quit"
};

/* ── Status Sub-screen ───────────────────────────────── */
static bool g_show_status = false;

static void status_render(GameContext *ctx) {
    Character *ch = &ctx->party.members[0];

    /* Full-screen dark background */
    platform_draw_rect(0, 0, SCREEN_W, SCREEN_H, 0x0000);

    /* Title */
    platform_draw_text(80, 8, "- STATUS -", 0x7FFF);

    /* Character name and level */
    char line[48];
    snprintf(line, sizeof(line), "%s  Lv.%d", ch->name, ch->level);
    platform_draw_text(16, 28, line, 0x7FFF);

    /* HP / MP */
    snprintf(line, sizeof(line), "HP: %d / %d", ch->hp, ch->max_hp);
    platform_draw_text(16, 44, line, 0x1BE0); /* green */

    snprintf(line, sizeof(line), "MP: %d / %d", ch->mp, ch->max_mp);
    platform_draw_text(16, 56, line, 0x7C1F); /* purple */

    /* Combat stats */
    snprintf(line, sizeof(line), "ATK: %d", ch->atk);
    platform_draw_text(16, 76, line, 0x7FFF);

    snprintf(line, sizeof(line), "DEF: %d", ch->def);
    platform_draw_text(16, 88, line, 0x7FFF);

    snprintf(line, sizeof(line), "SPD: %d", ch->spd);
    platform_draw_text(16, 100, line, 0x7FFF);

    snprintf(line, sizeof(line), "LUK: %d", ch->luk);
    platform_draw_text(16, 112, line, 0x7FFF);

    /* EXP to next level */
    uint16_t needed = party_calc_exp_needed(ch->level);
    snprintf(line, sizeof(line), "EXP: %d / %d", ch->exp, needed);
    platform_draw_text(16, 132, line, 0x03FF); /* yellow */

    /* Gold */
    snprintf(line, sizeof(line), "Gold: %d", ctx->party.gold);
    platform_draw_text(16, 148, line, 0x03FF);

    /* Footer */
    platform_draw_text(72, 152, "[B] Back", 0x5294);
}

/* ── Menu Update ─────────────────────────────────────── */
void menu_update(MenuContext *menu, GameContext *ctx) {
    uint16_t keys = platform_keys_pressed();

    /* Status sub-screen: B to return to menu */
    if (g_show_status) {
        if (keys & KEY_B) {
            g_show_status = false;
        }
        return;
    }

    /* Navigation */
    if (keys & KEY_UP) {
        if (menu->cursor > 0) menu->cursor--;
    }
    if (keys & KEY_DOWN) {
        if (menu->cursor < MENU_COUNT - 1) menu->cursor++;
    }

    /* Close menu */
    if (keys & (KEY_B | KEY_START)) {
        menu->open = false;
        menu->cursor = 0;
        ctx->state = STATE_WORLD;
        return;
    }

    /* Select */
    if (keys & KEY_A) {
        switch ((MenuItem)menu->cursor) {
            case MENU_ITEMS:
                menu->open = false;
                ctx->state = STATE_INVENTORY;
                break;
            case MENU_STATUS:
                g_show_status = true;
                break;
            case MENU_SAVE:
                menu->open = false;
                ctx->state = STATE_SAVE;
                break;
            case MENU_LOAD:
                /* Load uses the same save screen logic;
                   handled via save_cursor in GameContext */
                menu->open = false;
                ctx->state = STATE_SAVE;
                break;
            case MENU_QUIT:
                ctx->running = false;
                break;
            default:
                break;
        }
    }
}

/* ── Menu Render ─────────────────────────────────────── */
void menu_render(MenuContext *menu, GameContext *ctx) {
    /* Render the world underneath */
    state_world_render(ctx);

    /* Status sub-screen overrides menu */
    if (g_show_status) {
        status_render(ctx);
        return;
    }

    /* Semi-transparent dark box on left side */
    platform_draw_rect(0, 0, 80, 120, 0x0000);

    /* Menu items */
    for (int i = 0; i < MENU_COUNT; i++) {
        int y = 8 + i * 20;
        uint16_t color = (i == menu->cursor) ? 0x7FFF : 0x5294;

        /* Cursor arrow */
        if (i == menu->cursor) {
            platform_draw_text(4, y, ">", 0x7FFF);
        }

        platform_draw_text(16, y, menu_labels[i], color);
    }

    /* Mini status on right side */
    Character *ch = &ctx->party.members[0];
    char line[32];

    platform_draw_rect(140, 0, 100, 56, 0x0000);

    platform_draw_text(148, 4, ch->name, 0x7FFF);

    snprintf(line, sizeof(line), "Lv.%d", ch->level);
    platform_draw_text(148, 16, line, 0x7FFF);

    snprintf(line, sizeof(line), "HP:%d/%d", ch->hp, ch->max_hp);
    platform_draw_text(148, 28, line, 0x1BE0);

    snprintf(line, sizeof(line), "MP:%d/%d", ch->mp, ch->max_mp);
    platform_draw_text(148, 40, line, 0x7C1F);
}
