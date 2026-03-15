/**
 * quest.h -- Quest system for NPC-driven missions
 */
#ifndef QUEST_H
#define QUEST_H

#include <stdint.h>
#include <stdbool.h>
#include "game.h"

/* ── Quest States (2 bits per quest) ────────────────────── */
typedef enum {
    QUEST_NOT_STARTED = 0,
    QUEST_ACTIVE      = 1,
    QUEST_COMPLETE    = 2,
    QUEST_REWARDED    = 3,
} QuestState;

/* ── Quest Types ─────────────────────────────────────────── */
typedef enum {
    QUEST_TYPE_COLLECT,   /* gather N items */
    QUEST_TYPE_HUNT,      /* defeat N enemies of a type */
    QUEST_TYPE_DELIVER,   /* talk to target NPC */
} QuestType;

/* ── Quest Data ──────────────────────────────────────────── */
typedef struct {
    uint8_t   quest_id;
    QuestType type;
    uint8_t   giver_map;        /* map where quest giver lives */
    uint8_t   giver_dialogue_id;/* NPC dialogue_id of the quest giver */
    /* target info */
    uint8_t   target_id;        /* item_id for COLLECT, enemy_index for HUNT, dialogue_id for DELIVER */
    uint8_t   target_count;     /* how many to collect/kill (1 for deliver) */
    uint8_t   target_map;       /* map for DELIVER target NPC */
    uint8_t   target_dialogue_id; /* dialogue_id for DELIVER target NPC */
    /* reward info */
    uint8_t   reward_item_id;   /* item to give, 0xFF = none */
    uint8_t   reward_item_count;
    int16_t   reward_gold;
} QuestData;

/* ── Constants ───────────────────────────────────────────── */
#define MAX_QUESTS     32   /* max 32 quests (64 bits = 8 bytes) */
#define QUEST_COUNT     3   /* currently defined quests */

/* ── Kill Tracking ───────────────────────────────────────── */
/* Stored in quest_flags bytes 6-7 as enemy kill counters.
 * quest_flags[0..5] = quest state bits (2 bits x 24 quests max)
 * quest_flags[6]    = goblin kill count (up to 255)
 * quest_flags[7]    = reserved
 *
 * Actually, we store quest states in bits 0-1 of each 2-bit pair:
 * Byte 0 bits: quest0[1:0], quest1[3:2], quest2[5:4], quest3[7:6]
 * So 4 quests per byte, 6 bytes = 24 quests for states.
 * quest_flags[6] = goblin kills, quest_flags[7] = delivery flags.
 */
#define QUEST_KILL_GOBLIN_IDX  6   /* quest_flags index for goblin kill count */
#define QUEST_DELIVER_FLAGS_IDX 7  /* quest_flags index for delivery tracking */

/* ── Functions ───────────────────────────────────────────── */
void       quest_init(uint8_t *quest_flags);
QuestState quest_get_state(const uint8_t *quest_flags, int quest_id);
void       quest_set_state(uint8_t *quest_flags, int quest_id, QuestState state);
bool       quest_check_completion(const uint8_t *quest_flags, int quest_id,
                                  const Inventory *inv);
void       quest_give_reward(uint8_t *quest_flags, int quest_id,
                             Inventory *inv, Party *party);
const QuestData *quest_get_data(int quest_id);
void       quest_on_enemy_defeated(uint8_t *quest_flags, const char *enemy_name);
int        quest_get_npc_dialogue(const uint8_t *quest_flags, int npc_dialogue_id,
                                  uint8_t npc_map);

/* Helper: count items of a given id in inventory */
int        quest_count_item(const Inventory *inv, int item_id);

#endif /* QUEST_H */
