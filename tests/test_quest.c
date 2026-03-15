/**
 * test_quest.c -- Unit tests for the quest system
 */
#ifndef PLATFORM_TEST
#define PLATFORM_TEST
#endif
#include "test_framework.h"
#include "platform_stub.h"
#include "game.h"
#include "quest.h"
#include "inventory.h"

/* Pull in implementations */
#include "../src/core/inventory.c"
#include "../src/core/quest.c"

/* ── Test: Quest state get/set (2-bit manipulation) ──────── */
static void test_quest_state_getset(void) {
    uint8_t flags[8];
    quest_init(flags);

    /* All quests should start as NOT_STARTED */
    TEST_ASSERT_EQ(quest_get_state(flags, 0), QUEST_NOT_STARTED);
    TEST_ASSERT_EQ(quest_get_state(flags, 1), QUEST_NOT_STARTED);
    TEST_ASSERT_EQ(quest_get_state(flags, 2), QUEST_NOT_STARTED);

    /* Set quest 0 to ACTIVE */
    quest_set_state(flags, 0, QUEST_ACTIVE);
    TEST_ASSERT_EQ(quest_get_state(flags, 0), QUEST_ACTIVE);
    TEST_ASSERT_EQ(quest_get_state(flags, 1), QUEST_NOT_STARTED); /* others unaffected */
    TEST_ASSERT_EQ(quest_get_state(flags, 2), QUEST_NOT_STARTED);

    /* Set quest 1 to COMPLETE */
    quest_set_state(flags, 1, QUEST_COMPLETE);
    TEST_ASSERT_EQ(quest_get_state(flags, 0), QUEST_ACTIVE);
    TEST_ASSERT_EQ(quest_get_state(flags, 1), QUEST_COMPLETE);

    /* Set quest 2 to REWARDED */
    quest_set_state(flags, 2, QUEST_REWARDED);
    TEST_ASSERT_EQ(quest_get_state(flags, 2), QUEST_REWARDED);

    /* Test quest in second byte (quest 4) */
    quest_set_state(flags, 4, QUEST_ACTIVE);
    TEST_ASSERT_EQ(quest_get_state(flags, 4), QUEST_ACTIVE);
    TEST_ASSERT_EQ(quest_get_state(flags, 0), QUEST_ACTIVE); /* byte 0 unaffected */

    /* Test all 4 values in a single byte */
    quest_set_state(flags, 0, QUEST_NOT_STARTED);
    quest_set_state(flags, 1, QUEST_ACTIVE);
    quest_set_state(flags, 2, QUEST_COMPLETE);
    quest_set_state(flags, 3, QUEST_REWARDED);
    TEST_ASSERT_EQ(quest_get_state(flags, 0), QUEST_NOT_STARTED);
    TEST_ASSERT_EQ(quest_get_state(flags, 1), QUEST_ACTIVE);
    TEST_ASSERT_EQ(quest_get_state(flags, 2), QUEST_COMPLETE);
    TEST_ASSERT_EQ(quest_get_state(flags, 3), QUEST_REWARDED);

    /* Boundary: invalid quest_id */
    TEST_ASSERT_EQ(quest_get_state(flags, -1), QUEST_NOT_STARTED);
    TEST_ASSERT_EQ(quest_get_state(flags, 32), QUEST_NOT_STARTED);
    /* quest_id 24+ would be in byte 6+ which is reserved for counters */
    TEST_ASSERT_EQ(quest_get_state(flags, 24), QUEST_NOT_STARTED);
}

/* ── Test: State transitions ─────────────────────────────── */
static void test_quest_transitions(void) {
    uint8_t flags[8];
    quest_init(flags);

    /* NOT_STARTED -> ACTIVE -> COMPLETE -> REWARDED */
    TEST_ASSERT_EQ(quest_get_state(flags, 0), QUEST_NOT_STARTED);

    quest_set_state(flags, 0, QUEST_ACTIVE);
    TEST_ASSERT_EQ(quest_get_state(flags, 0), QUEST_ACTIVE);

    quest_set_state(flags, 0, QUEST_COMPLETE);
    TEST_ASSERT_EQ(quest_get_state(flags, 0), QUEST_COMPLETE);

    quest_set_state(flags, 0, QUEST_REWARDED);
    TEST_ASSERT_EQ(quest_get_state(flags, 0), QUEST_REWARDED);
}

/* ── Test: Collection quest completion check ─────────────── */
static void test_collect_quest_completion(void) {
    uint8_t flags[8];
    quest_init(flags);

    Inventory inv;
    inventory_init(&inv);

    /* Quest 0 = collect 3 Potions (item_id 0) */
    quest_set_state(flags, 0, QUEST_ACTIVE);

    /* Not enough items yet */
    inventory_add(&inv, 0, 2);
    TEST_ASSERT(!quest_check_completion(flags, 0, &inv));

    /* Add one more -- should now complete */
    inventory_add(&inv, 0, 1);
    TEST_ASSERT(quest_check_completion(flags, 0, &inv));

    /* More than required should still pass */
    inventory_add(&inv, 0, 5);
    TEST_ASSERT(quest_check_completion(flags, 0, &inv));
}

/* ── Test: Hunt quest completion check (kill count) ──────── */
static void test_hunt_quest_completion(void) {
    uint8_t flags[8];
    quest_init(flags);

    Inventory inv;
    inventory_init(&inv);

    /* Quest 1 = defeat 3 Goblins */
    quest_set_state(flags, 1, QUEST_ACTIVE);

    /* 0 kills */
    TEST_ASSERT(!quest_check_completion(flags, 1, &inv));

    /* Simulate 2 goblin kills */
    quest_on_enemy_defeated(flags, "Goblin");
    quest_on_enemy_defeated(flags, "Goblin");
    TEST_ASSERT_EQ(flags[QUEST_KILL_GOBLIN_IDX], 2);
    TEST_ASSERT(!quest_check_completion(flags, 1, &inv));

    /* 3rd kill -- should complete */
    quest_on_enemy_defeated(flags, "Goblin");
    TEST_ASSERT_EQ(flags[QUEST_KILL_GOBLIN_IDX], 3);
    TEST_ASSERT(quest_check_completion(flags, 1, &inv));

    /* Non-goblin enemies don't count */
    flags[QUEST_KILL_GOBLIN_IDX] = 0;
    quest_on_enemy_defeated(flags, "Slime");
    TEST_ASSERT_EQ(flags[QUEST_KILL_GOBLIN_IDX], 0);

    /* Kills don't track if quest not active */
    quest_set_state(flags, 1, QUEST_NOT_STARTED);
    quest_on_enemy_defeated(flags, "Goblin");
    TEST_ASSERT_EQ(flags[QUEST_KILL_GOBLIN_IDX], 0);
}

/* ── Test: Delivery quest completion ─────────────────────── */
static void test_deliver_quest_completion(void) {
    uint8_t flags[8];
    quest_init(flags);

    Inventory inv;
    inventory_init(&inv);

    /* Quest 2 = deliver to Oasis Trader */
    quest_set_state(flags, 2, QUEST_ACTIVE);

    /* Not delivered yet */
    TEST_ASSERT(!quest_check_completion(flags, 2, &inv));

    /* Mark delivery flag */
    flags[QUEST_DELIVER_FLAGS_IDX] |= (1 << 2);
    TEST_ASSERT(quest_check_completion(flags, 2, &inv));
}

/* ── Test: Reward giving ─────────────────────────────────── */
static void test_quest_reward(void) {
    uint8_t flags[8];
    quest_init(flags);

    Inventory inv;
    inventory_init(&inv);
    Party party;
    memset(&party, 0, sizeof(Party));
    party.count = 1;
    party.gold = 0;

    /* Quest 0 reward: 1 Potion + 50 Gold, consumes 3 Potions */
    quest_set_state(flags, 0, QUEST_COMPLETE);
    inventory_add(&inv, 0, 5); /* 5 Potions */

    quest_give_reward(flags, 0, &inv, &party);

    /* Should be REWARDED now */
    TEST_ASSERT_EQ(quest_get_state(flags, 0), QUEST_REWARDED);
    /* Gold rewarded */
    TEST_ASSERT_EQ(party.gold, 50);
    /* 5 Potions - 3 consumed + 1 reward = 3 Potions */
    TEST_ASSERT_EQ(quest_count_item(&inv, 0), 3);

    /* Quest 1 reward: Iron Blade (id 14) + 30 Gold */
    quest_set_state(flags, 1, QUEST_COMPLETE);
    party.gold = 0;
    quest_give_reward(flags, 1, &inv, &party);
    TEST_ASSERT_EQ(quest_get_state(flags, 1), QUEST_REWARDED);
    TEST_ASSERT_EQ(party.gold, 30);
    TEST_ASSERT(quest_count_item(&inv, 14) >= 1);

    /* Quest 2 reward: 100 Gold, no item */
    quest_set_state(flags, 2, QUEST_COMPLETE);
    party.gold = 0;
    quest_give_reward(flags, 2, &inv, &party);
    TEST_ASSERT_EQ(quest_get_state(flags, 2), QUEST_REWARDED);
    TEST_ASSERT_EQ(party.gold, 100);
}

/* ── Test: Quest flags save/load round-trip ──────────────── */
static void test_quest_save_load(void) {
    uint8_t flags_orig[8];
    uint8_t flags_copy[8];
    quest_init(flags_orig);

    /* Set up various states */
    quest_set_state(flags_orig, 0, QUEST_REWARDED);
    quest_set_state(flags_orig, 1, QUEST_ACTIVE);
    quest_set_state(flags_orig, 2, QUEST_COMPLETE);
    flags_orig[QUEST_KILL_GOBLIN_IDX] = 5;
    flags_orig[QUEST_DELIVER_FLAGS_IDX] = 0x04; /* quest 2 delivered */

    /* Copy (simulates save/load) */
    memcpy(flags_copy, flags_orig, 8);

    /* Verify all states preserved */
    TEST_ASSERT_EQ(quest_get_state(flags_copy, 0), QUEST_REWARDED);
    TEST_ASSERT_EQ(quest_get_state(flags_copy, 1), QUEST_ACTIVE);
    TEST_ASSERT_EQ(quest_get_state(flags_copy, 2), QUEST_COMPLETE);
    TEST_ASSERT_EQ(flags_copy[QUEST_KILL_GOBLIN_IDX], 5);
    TEST_ASSERT_EQ(flags_copy[QUEST_DELIVER_FLAGS_IDX], 0x04);

    /* Test with actual save struct simulation */
    {
        uint8_t save_buf[8];
        memcpy(save_buf, flags_orig, 8);

        uint8_t flags_loaded[8];
        memcpy(flags_loaded, save_buf, 8);

        TEST_ASSERT_EQ(quest_get_state(flags_loaded, 0), QUEST_REWARDED);
        TEST_ASSERT_EQ(quest_get_state(flags_loaded, 1), QUEST_ACTIVE);
        TEST_ASSERT_EQ(quest_get_state(flags_loaded, 2), QUEST_COMPLETE);
        TEST_ASSERT_EQ(flags_loaded[QUEST_KILL_GOBLIN_IDX], 5);
    }
}

/* ── Test: Quest data access ─────────────────────────────── */
static void test_quest_data(void) {
    const QuestData *q0 = quest_get_data(0);
    TEST_ASSERT(q0 != NULL);
    TEST_ASSERT_EQ(q0->type, QUEST_TYPE_COLLECT);
    TEST_ASSERT_EQ(q0->target_count, 3);
    TEST_ASSERT_EQ(q0->reward_gold, 50);

    const QuestData *q1 = quest_get_data(1);
    TEST_ASSERT(q1 != NULL);
    TEST_ASSERT_EQ(q1->type, QUEST_TYPE_HUNT);
    TEST_ASSERT_EQ(q1->reward_gold, 30);

    const QuestData *q2 = quest_get_data(2);
    TEST_ASSERT(q2 != NULL);
    TEST_ASSERT_EQ(q2->type, QUEST_TYPE_DELIVER);
    TEST_ASSERT_EQ(q2->reward_gold, 100);

    /* Invalid quest */
    TEST_ASSERT(quest_get_data(-1) == NULL);
    TEST_ASSERT(quest_get_data(3) == NULL);
}

/* ── Test: NPC dialogue routing ──────────────────────────── */
static void test_npc_dialogue_routing(void) {
    uint8_t flags[8];
    quest_init(flags);

    /* Old Sage (dialogue_id 13) -- quest 0 not started -> offer */
    int d = quest_get_npc_dialogue(flags, 13, 0);
    TEST_ASSERT_EQ(d, -100); /* -100 - 0 = offer quest 0 */

    /* Set quest 0 to ACTIVE -> reminder */
    quest_set_state(flags, 0, QUEST_ACTIVE);
    d = quest_get_npc_dialogue(flags, 13, 0);
    TEST_ASSERT_EQ(d, -200); /* reminder quest 0 */

    /* Set quest 0 to COMPLETE -> turn-in */
    quest_set_state(flags, 0, QUEST_COMPLETE);
    d = quest_get_npc_dialogue(flags, 13, 0);
    TEST_ASSERT_EQ(d, -300); /* turn-in quest 0 */

    /* Set quest 0 to REWARDED -> done */
    quest_set_state(flags, 0, QUEST_REWARDED);
    d = quest_get_npc_dialogue(flags, 13, 0);
    TEST_ASSERT_EQ(d, -400); /* done quest 0 */

    /* Unrelated NPC should return -1 */
    d = quest_get_npc_dialogue(flags, 99, 0);
    TEST_ASSERT_EQ(d, -1);

    /* Delivery target NPC (Oasis Trader, dialogue_id 20) when quest 2 active */
    quest_set_state(flags, 2, QUEST_ACTIVE);
    d = quest_get_npc_dialogue(flags, 20, 2);
    TEST_ASSERT_EQ(d, -502); /* deliver action quest 2 */
}

/* ── Test: quest_count_item helper ───────────────────────── */
static void test_count_item(void) {
    Inventory inv;
    inventory_init(&inv);

    TEST_ASSERT_EQ(quest_count_item(&inv, 0), 0);

    inventory_add(&inv, 0, 3);
    TEST_ASSERT_EQ(quest_count_item(&inv, 0), 3);

    inventory_add(&inv, 4, 2);
    TEST_ASSERT_EQ(quest_count_item(&inv, 4), 2);
    TEST_ASSERT_EQ(quest_count_item(&inv, 0), 3); /* unchanged */

    TEST_ASSERT_EQ(quest_count_item(&inv, 99), 0); /* non-existent */
    TEST_ASSERT_EQ(quest_count_item(NULL, 0), 0);  /* null inv */
}

/* ── Main ────────────────────────────────────────────────── */
int main(void) {
    printf("=== Quest Tests ===\n");

    TEST_RUN(test_quest_state_getset);
    TEST_RUN(test_quest_transitions);
    TEST_RUN(test_collect_quest_completion);
    TEST_RUN(test_hunt_quest_completion);
    TEST_RUN(test_deliver_quest_completion);
    TEST_RUN(test_quest_reward);
    TEST_RUN(test_quest_save_load);
    TEST_RUN(test_quest_data);
    TEST_RUN(test_npc_dialogue_routing);
    TEST_RUN(test_count_item);

    TEST_SUMMARY();
}
