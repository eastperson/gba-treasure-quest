/**
 * treasure.c — Treasure collection system implementation
 * Platform-independent: no GBA or SDL headers here.
 */
#include "treasure.h"
#include "inventory.h"
#include "platform.h"
#include <string.h>
#include <stdio.h>

/* ── Treasure Table ──────────────────────────────────── */
static TreasureInfo g_treasures[TOTAL_TREASURES] = {
    /* name              island  item_id  description                         collected */
    { "Coral Crown",        0,      6,    "Crown of the sea king",            false },
    { "Emerald Blade",      1,      7,    "Sword of the forest guardian",     false },
    { "Sun Amulet",         2,      8,    "Amulet glowing with desert heat",  false },
    { "Flame Ruby",         3,      9,    "Gem from the volcano's heart",     false },
    { "Frost Crystal",      4,     10,    "Crystal of eternal ice",           false },
    { "Abyssal Pearl",      5,     11,    "Pearl from the deep ruins",        false },
    { "Sky Scepter",        6,     12,    "Scepter of the heavens",           false },
};

/* ── Initialization ──────────────────────────────────── */
void treasure_init(void) {
    for (int i = 0; i < TOTAL_TREASURES; i++) {
        g_treasures[i].collected = false;
    }
}

/* ── Collect Treasure ────────────────────────────────── */
void treasure_collect(int island_id, GameContext *ctx) {
    if (island_id < 0 || island_id >= TOTAL_TREASURES) return;

    TreasureInfo *t = &g_treasures[island_id];

    /* Already collected */
    if (t->collected) return;

    /* Set collected flag and bitmask */
    t->collected = true;
    ctx->treasures_found |= (1 << island_id);

    /* Add treasure item to inventory */
    inventory_add(&ctx->inventory, t->item_id, 1);
}

/* ── Get Count ───────────────────────────────────────── */
int treasure_get_count(GameContext *ctx) {
    int count = 0;
    for (int i = 0; i < TOTAL_TREASURES; i++) {
        if (ctx->treasures_found & (1 << i)) {
            count++;
        }
    }
    return count;
}

/* ── Check Complete ──────────────────────────────────── */
bool treasure_check_complete(GameContext *ctx) {
    return (ctx->treasures_found & 0x7F) == 0x7F;
}

/* ── Render Treasure Status ──────────────────────────── */
void treasure_render_status(GameContext *ctx) {
    /* Draw 7 treasure icons: filled square if collected, empty if not */
    int base_x = 160;
    int base_y = 2;
    int icon_size = 6;
    int spacing = 8;

    for (int i = 0; i < TOTAL_TREASURES; i++) {
        int x = base_x + i * spacing;
        bool collected = (ctx->treasures_found & (1 << i)) != 0;

        if (collected) {
            /* Filled icon — gold */
            platform_draw_rect(x, base_y, icon_size, icon_size, 0x03FF);
        } else {
            /* Empty icon — dark outline */
            platform_draw_rect(x, base_y, icon_size, icon_size, 0x294A);
            /* Inner empty space */
            platform_draw_rect(x + 1, base_y + 1, icon_size - 2, icon_size - 2, 0x0000);
        }
    }
}
