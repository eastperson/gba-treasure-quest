/**
 * test_party.c — Unit tests for party management and level-up logic
 */
#ifndef PLATFORM_TEST
#define PLATFORM_TEST
#endif
#include "test_framework.h"
#include "platform_stub.h"
#include "game.h"
#include "party.h"

/* Pull in the implementation */
#include "../src/core/party.c"

/* ── Test: Level-up HP increase ─────────────────────────── */
static void test_level_up_hp(void) {
    Character ch;
    memset(&ch, 0, sizeof(Character));
    ch.level  = 1;
    ch.hp     = 30;
    ch.max_hp = 30;
    ch.mp     = 10;
    ch.max_mp = 10;
    ch.atk    = 8;
    ch.def    = 5;
    ch.spd    = 6;
    ch.luk    = 4;

    int16_t old_max_hp = ch.max_hp;
    party_apply_level_up(&ch);

    /* Level should have increased */
    TEST_ASSERT_EQ(ch.level, 2);
    /* HP should have increased (bracket 0: +7 HP) */
    TEST_ASSERT(ch.max_hp > old_max_hp);
    TEST_ASSERT_EQ(ch.max_hp, old_max_hp + 7);
    /* HP should be fully restored after level up */
    TEST_ASSERT_EQ(ch.hp, ch.max_hp);
}

/* ── Test: Level-up MP increase ─────────────────────────── */
static void test_level_up_mp(void) {
    Character ch;
    memset(&ch, 0, sizeof(Character));
    ch.level  = 1;
    ch.hp     = 30;
    ch.max_hp = 30;
    ch.mp     = 5;   /* partially depleted */
    ch.max_mp = 10;
    ch.atk    = 8;
    ch.def    = 5;
    ch.spd    = 6;
    ch.luk    = 4;

    int16_t old_max_mp = ch.max_mp;
    party_apply_level_up(&ch);

    /* MP max should have increased (bracket 0: +4 MP) */
    TEST_ASSERT(ch.max_mp > old_max_mp);
    TEST_ASSERT_EQ(ch.max_mp, old_max_mp + 4);
    /* MP should be fully restored after level up */
    TEST_ASSERT_EQ(ch.mp, ch.max_mp);
}

/* ── Test: Stat growth across level brackets ────────────── */
static void test_stat_growth_brackets(void) {
    Character ch;
    memset(&ch, 0, sizeof(Character));
    ch.level  = 1;
    ch.hp     = 30;
    ch.max_hp = 30;
    ch.mp     = 10;
    ch.max_mp = 10;
    ch.atk    = 8;
    ch.def    = 5;
    ch.spd    = 6;
    ch.luk    = 4;

    /* Level up through bracket 0 (levels 1-5: fast growth) */
    for (int i = 0; i < 4; i++) {
        party_apply_level_up(&ch);
    }
    TEST_ASSERT_EQ(ch.level, 5);
    /* 4 level-ups with +3 ATK each = 8 + 12 = 20 */
    TEST_ASSERT_EQ(ch.atk, 8 + 4 * 3);
    /* 4 level-ups with +2 DEF each = 5 + 8 = 13 */
    TEST_ASSERT_EQ(ch.def, 5 + 4 * 2);

    /* Level up from 5->6: bonus is looked up for level 5 (bracket 0: +3 ATK) */
    int16_t atk_before = ch.atk;
    party_apply_level_up(&ch);
    TEST_ASSERT_EQ(ch.level, 6);
    TEST_ASSERT_EQ(ch.atk, atk_before + 3);

    /* Now at level 6, bracket 1 applies: +2 ATK per level */
    atk_before = ch.atk;
    party_apply_level_up(&ch);
    TEST_ASSERT_EQ(ch.level, 7);
    TEST_ASSERT_EQ(ch.atk, atk_before + 2);

    /* Level up to bracket 2 (level 11+: slow growth) */
    while (ch.level < 11) {
        party_apply_level_up(&ch);
    }
    /* Now at level 11, bracket 2 applies: +1 ATK per level */
    atk_before = ch.atk;
    party_apply_level_up(&ch);
    TEST_ASSERT_EQ(ch.level, 12);
    TEST_ASSERT_EQ(ch.atk, atk_before + 1);
}

/* ── Test: party_try_level_up EXP logic ─────────────────── */
static void test_try_level_up(void) {
    Character ch;
    memset(&ch, 0, sizeof(Character));
    ch.level  = 1;
    ch.hp     = 30;
    ch.max_hp = 30;
    ch.mp     = 10;
    ch.max_mp = 10;
    ch.atk    = 8;
    ch.def    = 5;
    ch.spd    = 6;
    ch.luk    = 4;

    /* EXP needed at level 1 = 1*50 = 50 */
    ch.exp = 30;
    TEST_ASSERT(!party_try_level_up(&ch));
    TEST_ASSERT_EQ(ch.level, 1);

    ch.exp = 50;
    TEST_ASSERT(party_try_level_up(&ch));
    TEST_ASSERT_EQ(ch.level, 2);
    TEST_ASSERT_EQ(ch.exp, 0);  /* 50 - 50 = 0 */

    /* EXP needed at level 2 = 2*50 = 100 */
    ch.exp = 120;
    TEST_ASSERT(party_try_level_up(&ch));
    TEST_ASSERT_EQ(ch.level, 3);
    TEST_ASSERT_EQ(ch.exp, 20);  /* 120 - 100 = 20 */
}

/* ── Test: party_heal_full restores HP and MP ──────────── */
static void test_heal_full(void) {
    Character ch;
    memset(&ch, 0, sizeof(Character));
    ch.hp     = 10;
    ch.max_hp = 50;
    ch.mp     = 3;
    ch.max_mp = 20;

    party_heal_full(&ch);
    TEST_ASSERT_EQ(ch.hp, 50);
    TEST_ASSERT_EQ(ch.mp, 20);
}

/* ── Test: party_add_member MP initialization ──────────── */
static void test_add_member_mp(void) {
    Party party;
    memset(&party, 0, sizeof(Party));

    party_add_member(&party, "Mage", 25, 20, 6, 4, 5, 1);
    TEST_ASSERT_EQ(party.count, 1);
    TEST_ASSERT_EQ(party.members[0].mp, 20);
    TEST_ASSERT_EQ(party.members[0].max_mp, 20);
    TEST_ASSERT_EQ(party.members[0].hp, 25);
    TEST_ASSERT_EQ(party.members[0].max_hp, 25);
}

/* ── Main ────────────────────────────────────────────────── */
int main(void) {
    printf("=== Party Tests ===\n");

    TEST_RUN(test_level_up_hp);
    TEST_RUN(test_level_up_mp);
    TEST_RUN(test_stat_growth_brackets);
    TEST_RUN(test_try_level_up);
    TEST_RUN(test_heal_full);
    TEST_RUN(test_add_member_mp);

    TEST_SUMMARY();
}
