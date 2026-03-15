/**
 * save.c — Save/Load system implementation
 * Platform-independent: uses platform_save/platform_load for storage.
 */
#include "save.h"
#include "platform.h"
#include <string.h>
#include <stdio.h>

/* ── Static save data cache ──────────────────────────── */
static SaveData g_save_data;

/* ── Initialization ──────────────────────────────────── */
void save_init(void) {
    /* Try to load existing save data from storage */
    if (!platform_load(&g_save_data, sizeof(SaveData))) {
        /* No existing data — clear all slots */
        memset(&g_save_data, 0, sizeof(SaveData));
    }

    /* Validate each slot's magic number */
    for (int i = 0; i < MAX_SAVE_SLOTS; i++) {
        if (g_save_data.slots[i].magic != SAVE_MAGIC) {
            g_save_data.slots[i].valid = false;
        }
    }
}

/* ── Write ───────────────────────────────────────────── */
bool save_write(int slot, GameContext *ctx) {
    if (slot < 0 || slot >= MAX_SAVE_SLOTS) return false;

    SaveSlot *s = &g_save_data.slots[slot];
    s->magic          = SAVE_MAGIC;
    s->valid          = true;
    s->party          = ctx->party;
    s->pos            = ctx->pos;
    s->current_island = ctx->current_island;
    s->treasures_found = ctx->treasures_found;
    s->play_time      = ctx->frame_count;
    s->inventory      = ctx->inventory;
    memcpy(s->quest_flags, ctx->quest_flags, sizeof(s->quest_flags));

    return platform_save(&g_save_data, sizeof(SaveData));
}

/* ── Read ────────────────────────────────────────────── */
bool save_read(int slot, GameContext *ctx) {
    if (slot < 0 || slot >= MAX_SAVE_SLOTS) return false;

    /* Reload from storage to get freshest data */
    if (!platform_load(&g_save_data, sizeof(SaveData))) {
        return false;
    }

    SaveSlot *s = &g_save_data.slots[slot];
    if (s->magic != SAVE_MAGIC || !s->valid) {
        return false;
    }

    ctx->party          = s->party;
    ctx->pos            = s->pos;
    ctx->current_island = s->current_island;
    ctx->treasures_found = s->treasures_found;
    ctx->frame_count    = s->play_time;
    ctx->inventory      = s->inventory;
    memcpy(ctx->quest_flags, s->quest_flags, sizeof(ctx->quest_flags));

    return true;
}

/* ── Check Slot ──────────────────────────────────────── */
bool save_check_slot(int slot) {
    if (slot < 0 || slot >= MAX_SAVE_SLOTS) return false;
    return (g_save_data.slots[slot].magic == SAVE_MAGIC &&
            g_save_data.slots[slot].valid);
}

/* ── Render Slots ────────────────────────────────────── */
void save_render_slots(int cursor) {
    /* Background box */
    platform_draw_rect(20, 20, 200, 120, 0x0000);
    platform_draw_text(72, 28, "- SAVE SLOTS -", 0x7FFF);

    for (int i = 0; i < MAX_SAVE_SLOTS; i++) {
        int y = 50 + i * 28;
        uint16_t color = (i == cursor) ? 0x03FF : 0x5294; /* yellow if selected, gray otherwise */

        /* Draw cursor arrow */
        if (i == cursor) {
            platform_draw_text(28, y, ">", 0x03FF);
        }

        if (save_check_slot(i)) {
            SaveSlot *s = &g_save_data.slots[i];
            Character *ch = &s->party.members[0];
            char line[48];
            snprintf(line, sizeof(line), "Slot %d: Lv.%d HP:%d/%d Island:%d",
                     i + 1, ch->level, ch->hp, ch->max_hp,
                     s->current_island + 1);
            platform_draw_text(40, y, line, color);
        } else {
            char line[16];
            snprintf(line, sizeof(line), "Slot %d: Empty", i + 1);
            platform_draw_text(40, y, line, color);
        }
    }
}
