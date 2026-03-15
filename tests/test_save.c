/**
 * test_save.c — Unit tests for save/load roundtrip
 */
#ifndef PLATFORM_TEST
#define PLATFORM_TEST
#endif
#include "test_framework.h"
#include "platform_stub.h"
#include "game.h"
#include "save.h"
#include "inventory.h"

/* Pull in the implementations */
#include "../src/core/save.c"
#include "../src/core/inventory.c"

/* ── Test: Save and load roundtrip ───────────────────── */
static void test_save_load_roundtrip(void) {
    /* Reset stub save buffer */
    g_stub_save_size = 0;

    /* Create a GameContext with known data */
    GameContext ctx;
    memset(&ctx, 0, sizeof(GameContext));

    /* Set up party */
    ctx.party.count = 2;
    strncpy(ctx.party.members[0].name, "Hero", MAX_NAME_LEN);
    ctx.party.members[0].hp = 45;
    ctx.party.members[0].max_hp = 50;
    ctx.party.members[0].mp = 10;
    ctx.party.members[0].max_mp = 20;
    ctx.party.members[0].atk = 15;
    ctx.party.members[0].def = 8;
    ctx.party.members[0].spd = 7;
    ctx.party.members[0].level = 5;
    ctx.party.members[0].exp = 120;
    ctx.party.members[0].weapon_id = 13;
    ctx.party.members[0].armor_id = 16;

    strncpy(ctx.party.members[1].name, "Elara", MAX_NAME_LEN);
    ctx.party.members[1].hp = 30;
    ctx.party.members[1].max_hp = 35;
    ctx.party.members[1].level = 4;

    ctx.party.gold = 500;

    /* Set up inventory */
    inventory_init(&ctx.inventory);
    inventory_add(&ctx.inventory, 0, 5);   /* 5 Potions */
    inventory_add(&ctx.inventory, 4, 3);   /* 3 Bombs */
    inventory_add(&ctx.inventory, 14, 1);  /* 1 Iron Blade */

    /* Set up position and state */
    ctx.pos.x = 10;
    ctx.pos.y = 20;
    ctx.pos.map_id = 3;
    ctx.pos.direction = 2;
    ctx.current_island = 3;
    ctx.treasures_found = 0x07;  /* first 3 treasures */
    ctx.frame_count = 54321;
    ctx.quest_flags[0] = 0xFF;
    ctx.quest_flags[3] = 0x42;

    /* Initialize save system and write */
    save_init();
    bool ok = save_write(0, &ctx);
    TEST_ASSERT(ok);

    /* Verify slot is valid */
    TEST_ASSERT(save_check_slot(0));
    TEST_ASSERT(!save_check_slot(1));  /* slot 1 should be empty */

    /* Load into a fresh context */
    GameContext loaded;
    memset(&loaded, 0, sizeof(GameContext));

    ok = save_read(0, &loaded);
    TEST_ASSERT(ok);

    /* Verify party */
    TEST_ASSERT_EQ(loaded.party.count, 2);
    TEST_ASSERT(strcmp(loaded.party.members[0].name, "Hero") == 0);
    TEST_ASSERT_EQ(loaded.party.members[0].hp, 45);
    TEST_ASSERT_EQ(loaded.party.members[0].max_hp, 50);
    TEST_ASSERT_EQ(loaded.party.members[0].mp, 10);
    TEST_ASSERT_EQ(loaded.party.members[0].max_mp, 20);
    TEST_ASSERT_EQ(loaded.party.members[0].atk, 15);
    TEST_ASSERT_EQ(loaded.party.members[0].def, 8);
    TEST_ASSERT_EQ(loaded.party.members[0].spd, 7);
    TEST_ASSERT_EQ(loaded.party.members[0].level, 5);
    TEST_ASSERT_EQ(loaded.party.members[0].exp, 120);
    TEST_ASSERT_EQ(loaded.party.members[0].weapon_id, 13);
    TEST_ASSERT_EQ(loaded.party.members[0].armor_id, 16);

    TEST_ASSERT(strcmp(loaded.party.members[1].name, "Elara") == 0);
    TEST_ASSERT_EQ(loaded.party.members[1].hp, 30);
    TEST_ASSERT_EQ(loaded.party.members[1].max_hp, 35);
    TEST_ASSERT_EQ(loaded.party.members[1].level, 4);

    TEST_ASSERT_EQ(loaded.party.gold, 500);

    /* Verify inventory */
    TEST_ASSERT_EQ(loaded.inventory.count, 3);
    TEST_ASSERT_EQ(loaded.inventory.slots[0].item_id, 0);
    TEST_ASSERT_EQ(loaded.inventory.slots[0].count, 5);
    TEST_ASSERT_EQ(loaded.inventory.slots[1].item_id, 4);
    TEST_ASSERT_EQ(loaded.inventory.slots[1].count, 3);
    TEST_ASSERT_EQ(loaded.inventory.slots[2].item_id, 14);
    TEST_ASSERT_EQ(loaded.inventory.slots[2].count, 1);

    /* Verify position and state */
    TEST_ASSERT_EQ(loaded.pos.x, 10);
    TEST_ASSERT_EQ(loaded.pos.y, 20);
    TEST_ASSERT_EQ(loaded.pos.map_id, 3);
    TEST_ASSERT_EQ(loaded.pos.direction, 2);
    TEST_ASSERT_EQ(loaded.current_island, 3);
    TEST_ASSERT_EQ(loaded.treasures_found, 0x07);
    TEST_ASSERT_EQ(loaded.frame_count, 54321);
    TEST_ASSERT_EQ(loaded.quest_flags[0], 0xFF);
    TEST_ASSERT_EQ(loaded.quest_flags[3], 0x42);
}

/* ── Test: Invalid slot handling ─────────────────────── */
static void test_invalid_slot(void) {
    g_stub_save_size = 0;

    GameContext ctx;
    memset(&ctx, 0, sizeof(GameContext));

    /* Out-of-range slot */
    TEST_ASSERT(!save_write(-1, &ctx));
    TEST_ASSERT(!save_write(3, &ctx));
    TEST_ASSERT(!save_read(-1, &ctx));
    TEST_ASSERT(!save_read(3, &ctx));

    /* Reading from uninitialized slot */
    save_init();
    TEST_ASSERT(!save_check_slot(0));
    TEST_ASSERT(!save_read(0, &ctx));
}

/* ── Main ────────────────────────────────────────────── */
int main(void) {
    printf("=== Save Tests ===\n");

    TEST_RUN(test_save_load_roundtrip);
    TEST_RUN(test_invalid_slot);

    TEST_SUMMARY();
}
