/**
 * treasure.h — Treasure collection system for Seven Islands
 */
#ifndef TREASURE_H
#define TREASURE_H

#include "game.h"

/* ── Constants ────────────────────────────────────────── */
#define TOTAL_TREASURES 7

/* ── Treasure Info ────────────────────────────────────── */
typedef struct {
    char    name[MAX_NAME_LEN];
    uint8_t island_id;
    uint8_t item_id;       /* maps to inventory item */
    char    description[48];
    bool    collected;
} TreasureInfo;

/* ── Functions ────────────────────────────────────────── */
void treasure_init(void);
void treasure_collect(int island_id, GameContext *ctx);
int  treasure_get_count(GameContext *ctx);
bool treasure_check_complete(GameContext *ctx);
void treasure_render_status(GameContext *ctx);

#endif /* TREASURE_H */
