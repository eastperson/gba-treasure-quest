/**
 * quest.c -- Quest system implementation
 * Platform-independent: no GBA or SDL headers here.
 */
#include "quest.h"
#include "inventory.h"
#include <string.h>

/* ── Quest Database ──────────────────────────────────────── */
static const QuestData g_quests[QUEST_COUNT] = {
    /* Quest 0: Herb Collection — Old Sage wants 3 Potions (item 0) */
    {
        .quest_id          = 0,
        .type              = QUEST_TYPE_COLLECT,
        .giver_map         = 0,
        .giver_dialogue_id = 13,
        .target_id         = 0,       /* item_id 0 = Potion (herbs) */
        .target_count      = 3,
        .target_map        = 0,
        .target_dialogue_id= 0,
        .reward_item_id    = 0,       /* 1 Potion reward */
        .reward_item_count = 1,
        .reward_gold       = 50,
    },
    /* Quest 1: Goblin Hunt — Guard wants 3 Goblins defeated */
    {
        .quest_id          = 1,
        .type              = QUEST_TYPE_HUNT,
        .giver_map         = 1,
        .giver_dialogue_id = 4,
        .target_id         = 1,       /* enemy index 1 = Goblin */
        .target_count      = 3,
        .target_map        = 0,
        .target_dialogue_id= 0,
        .reward_item_id    = 14,      /* Iron Blade */
        .reward_item_count = 1,
        .reward_gold       = 30,
    },
    /* Quest 2: Delivery — Merchant to Oasis Trader */
    {
        .quest_id          = 2,
        .type              = QUEST_TYPE_DELIVER,
        .giver_map         = 0,
        .giver_dialogue_id = 1,
        .target_id         = 0,       /* unused for deliver source */
        .target_count      = 1,
        .target_map        = 2,
        .target_dialogue_id= 20,      /* Oasis Trader dialogue_id */
        .reward_item_id    = 0xFF,    /* no item reward */
        .reward_item_count = 0,
        .reward_gold       = 100,
    },
};

/* ── Initialization ──────────────────────────────────────── */
void quest_init(uint8_t *quest_flags) {
    memset(quest_flags, 0, 8);
}

/* ── State Management (2 bits per quest) ─────────────────── */
/*
 * Layout: 4 quests per byte.
 * Byte i, quest j (within byte): bits [j*2+1 : j*2]
 * quest_id maps to byte = quest_id / 4, shift = (quest_id % 4) * 2
 *
 * Bytes 0-5 hold quest states (up to 24 quests).
 * Bytes 6-7 reserved for kill/delivery counters.
 */

QuestState quest_get_state(const uint8_t *quest_flags, int quest_id) {
    if (quest_id < 0 || quest_id >= MAX_QUESTS) return QUEST_NOT_STARTED;
    int byte_idx = quest_id / 4;
    if (byte_idx > 5) return QUEST_NOT_STARTED;  /* bytes 6-7 are counters */
    int shift = (quest_id % 4) * 2;
    return (QuestState)((quest_flags[byte_idx] >> shift) & 0x03);
}

void quest_set_state(uint8_t *quest_flags, int quest_id, QuestState state) {
    if (quest_id < 0 || quest_id >= MAX_QUESTS) return;
    int byte_idx = quest_id / 4;
    if (byte_idx > 5) return;
    int shift = (quest_id % 4) * 2;
    quest_flags[byte_idx] &= ~(0x03 << shift);       /* clear 2 bits */
    quest_flags[byte_idx] |= ((uint8_t)state << shift); /* set new state */
}

/* ── Item Count Helper ───────────────────────────────────── */
int quest_count_item(const Inventory *inv, int item_id) {
    if (!inv) return 0;
    for (int i = 0; i < inv->count; i++) {
        if (inv->slots[i].item_id == (uint8_t)item_id) {
            return inv->slots[i].count;
        }
    }
    return 0;
}

/* ── Completion Check ────────────────────────────────────── */
bool quest_check_completion(const uint8_t *quest_flags, int quest_id,
                            const Inventory *inv)
{
    if (quest_id < 0 || quest_id >= QUEST_COUNT) return false;
    if (quest_get_state(quest_flags, quest_id) != QUEST_ACTIVE) return false;

    const QuestData *q = &g_quests[quest_id];

    switch (q->type) {
    case QUEST_TYPE_COLLECT:
        return quest_count_item(inv, q->target_id) >= q->target_count;

    case QUEST_TYPE_HUNT:
        return quest_flags[QUEST_KILL_GOBLIN_IDX] >= q->target_count;

    case QUEST_TYPE_DELIVER:
        /* Delivery is "complete" when the player has talked to the
         * target NPC.  We track this via the delivery flags byte. */
        return (quest_flags[QUEST_DELIVER_FLAGS_IDX] & (1 << quest_id)) != 0;

    default:
        return false;
    }
}

/* ── Reward Distribution ─────────────────────────────────── */
void quest_give_reward(uint8_t *quest_flags, int quest_id,
                       Inventory *inv, Party *party)
{
    if (quest_id < 0 || quest_id >= QUEST_COUNT) return;

    const QuestData *q = &g_quests[quest_id];

    /* Give item reward */
    if (q->reward_item_id != 0xFF && inv) {
        inventory_add(inv, q->reward_item_id, q->reward_item_count);
    }

    /* Give gold reward */
    if (party && q->reward_gold > 0) {
        party->gold += q->reward_gold;
    }

    /* For collect quests, remove the collected items */
    if (q->type == QUEST_TYPE_COLLECT && inv) {
        /* Find the item slot and remove the required count */
        for (int i = 0; i < inv->count; i++) {
            if (inv->slots[i].item_id == q->target_id) {
                int to_remove = q->target_count;
                if (inv->slots[i].count <= (uint8_t)to_remove) {
                    to_remove = inv->slots[i].count;
                }
                inventory_remove(inv, i, to_remove);
                break;
            }
        }
    }

    /* Set quest to REWARDED */
    quest_set_state(quest_flags, quest_id, QUEST_REWARDED);
}

/* ── Get Quest Data ──────────────────────────────────────── */
const QuestData *quest_get_data(int quest_id) {
    if (quest_id < 0 || quest_id >= QUEST_COUNT) return NULL;
    return &g_quests[quest_id];
}

/* ── Enemy Kill Tracking ─────────────────────────────────── */
void quest_on_enemy_defeated(uint8_t *quest_flags, const char *enemy_name) {
    if (!quest_flags || !enemy_name) return;

    /* Check if this is a Goblin (simple string compare) */
    if (enemy_name[0] == 'G' && enemy_name[1] == 'o' && enemy_name[2] == 'b') {
        /* Only track if quest 1 (Goblin Hunt) is active */
        if (quest_get_state(quest_flags, 1) == QUEST_ACTIVE) {
            if (quest_flags[QUEST_KILL_GOBLIN_IDX] < 255) {
                quest_flags[QUEST_KILL_GOBLIN_IDX]++;
            }
        }
    }
}

/* ── NPC Dialogue Routing ────────────────────────────────── */
/*
 * Given an NPC's dialogue_id and map, returns the dialogue_id that should
 * actually be shown based on quest state.  Returns -1 if no quest override
 * applies (use the NPC's default dialogue).
 *
 * Quest dialogue IDs (allocated in the unused 16-19 range):
 *   16 = Quest 0 offer (Old Sage: "Bring me 3 herbs")
 *   17 = Quest 0 active reminder
 *   18 = Quest 0 complete ("Thank you! Here's your reward")
 *   19 = Quest 0 rewarded ("Thank you again, hero")
 *
 * For quests 1 and 2, we reuse dialogue lines via the same pattern
 * but we'll handle them directly in game.c with inline dialogue text.
 * Actually, for simplicity we'll return negative quest-specific codes
 * that game.c interprets.
 *
 * Return values:
 *   >= 0 : use this dialogue_id from the dialogue database
 *   -1   : no quest applies, use default NPC dialogue
 *   -100 - quest_id : quest offer (NOT_STARTED)
 *   -200 - quest_id : quest reminder (ACTIVE, not complete)
 *   -300 - quest_id : quest turn-in (COMPLETE)
 *   -400 - quest_id : quest done (REWARDED)
 */
int quest_get_npc_dialogue(const uint8_t *quest_flags, int npc_dialogue_id,
                           uint8_t npc_map)
{
    (void)npc_map; /* may use later for multi-map quest NPCs */

    /* Check each quest to see if this NPC is the giver or deliver target */
    for (int i = 0; i < QUEST_COUNT; i++) {
        const QuestData *q = &g_quests[i];
        QuestState state = quest_get_state(quest_flags, i);

        /* Quest giver NPC */
        if (npc_dialogue_id == q->giver_dialogue_id) {
            switch (state) {
            case QUEST_NOT_STARTED:
                return -100 - i;   /* offer */
            case QUEST_ACTIVE:
                return -200 - i;   /* reminder */
            case QUEST_COMPLETE:
                return -300 - i;   /* turn-in */
            case QUEST_REWARDED:
                return -400 - i;   /* done */
            }
        }

        /* Delivery target NPC (only relevant when quest is active) */
        if (q->type == QUEST_TYPE_DELIVER &&
            npc_dialogue_id == q->target_dialogue_id &&
            state == QUEST_ACTIVE)
        {
            /* Mark delivery as done */
            return -500 - i;  /* deliver action */
        }
    }

    return -1; /* no quest override */
}
