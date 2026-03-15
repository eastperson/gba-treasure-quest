/**
 * save.h — Save/Load system with slot management
 */
#ifndef SAVE_H
#define SAVE_H

#include "game.h"

/* ── Constants ────────────────────────────────────────── */
#define MAX_SAVE_SLOTS  3
#define SAVE_MAGIC      0x5452  /* "TR" — v2: added inventory+quest_flags */

/* ── Save Slot ────────────────────────────────────────── */
typedef struct {
    uint16_t    magic;          /* SAVE_MAGIC if valid */
    bool        valid;
    Party       party;
    MapPosition pos;
    uint8_t     current_island;
    uint8_t     treasures_found;
    uint32_t    play_time;      /* frame_count */
    Inventory   inventory;
    uint8_t     quest_flags[8];
} SaveSlot;

/* ── Save Data (all slots packed together) ────────────── */
typedef struct {
    SaveSlot slots[MAX_SAVE_SLOTS];
} SaveData;

/* ── Functions ────────────────────────────────────────── */
void save_init(void);
bool save_write(int slot, GameContext *ctx);
bool save_read(int slot, GameContext *ctx);
bool save_check_slot(int slot);
void save_render_slots(int cursor);

#endif /* SAVE_H */
