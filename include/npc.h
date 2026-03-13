/**
 * npc.h — NPC system: placement, behavior, and interaction
 */
#ifndef NPC_H
#define NPC_H

#include <stdint.h>
#include <stdbool.h>
#include "game.h"

/* ── NPC Types ─────────────────────────────────────────── */
typedef enum {
    NPC_VILLAGER,
    NPC_SHOPKEEPER,
    NPC_GUARD,
    NPC_SAGE,
    NPC_ENEMY_OVERWORLD,
    NPC_TYPE_COUNT
} NPCType;

/* ── NPC Data ──────────────────────────────────────────── */
#define MAX_NPCS 16

typedef struct {
    NPCType  type;
    int16_t  x;
    int16_t  y;
    uint8_t  direction;   /* 0=down, 1=up, 2=left, 3=right */
    uint8_t  sprite_id;
    uint8_t  dialogue_id;
    bool     active;
    char     name[MAX_NAME_LEN];
} NPCData;

/* ── Functions ─────────────────────────────────────────── */
void     npc_init_map(uint8_t map_id);
void     npc_update(uint32_t frame_count);
void     npc_draw(int16_t cam_x, int16_t cam_y);
NPCData *npc_check_interaction(int16_t player_x, int16_t player_y, uint8_t direction);
bool     npc_is_at(int16_t x, int16_t y);

#endif /* NPC_H */
