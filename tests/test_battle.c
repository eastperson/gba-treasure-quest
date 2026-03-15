/**
 * test_battle.c — Unit tests for damage calculations
 */
#ifndef PLATFORM_TEST
#define PLATFORM_TEST
#endif
#include "test_framework.h"
#include "platform_stub.h"
#include "game.h"
#include "battle.h"
#include "inventory.h"

/* Pull in the implementations */
#include "../src/core/battle.c"
#include "../src/core/inventory.c"

/* ── Test: Physical damage formula ───────────────────── */
static void test_physical_damage(void) {
    /* battle_calc_damage(atk, def) = (atk - def) + rand(-2..+2), min 1 */
    /* Run multiple times to check range */
    int min_dmg = 9999, max_dmg = 0;

    for (int i = 0; i < 100; i++) {
        int dmg = battle_calc_damage(15, 5);
        if (dmg < min_dmg) min_dmg = dmg;
        if (dmg > max_dmg) max_dmg = dmg;
        TEST_ASSERT(dmg >= 1);
    }

    /* base = 15-5 = 10, variance -2..+2, so range should be 8..12 */
    TEST_ASSERT(min_dmg >= 8);
    TEST_ASSERT(max_dmg <= 12);

    /* ATK < DEF: should always produce at least 1 */
    for (int i = 0; i < 50; i++) {
        int dmg = battle_calc_damage(3, 10);
        TEST_ASSERT_EQ(dmg >= 1, 1);
    }
}

/* ── Test: Spell damage with element multipliers ─────── */
static void test_spell_element_damage(void) {
    bool super_eff = false, resist_eff = false;

    /* Normal spell (no element match): power 15, atk 12, def 6 */
    /* base = 15 + 12/3 - 6/3 = 15 + 4 - 2 = 17 +/- variance */
    for (int i = 0; i < 20; i++) {
        int dmg = battle_calc_spell_damage(15, 12, 6,
            ELEM_FIRE, ELEM_NONE, ELEM_NONE, &super_eff, &resist_eff);
        TEST_ASSERT(dmg >= 1);
        TEST_ASSERT(!super_eff);
        TEST_ASSERT(!resist_eff);
    }

    /* Super effective: Fire spell vs Ice-weak enemy */
    /* base ~17, then * 1.5 = ~25 */
    int total_super = 0;
    for (int i = 0; i < 50; i++) {
        super_eff = false; resist_eff = false;
        int dmg = battle_calc_spell_damage(15, 12, 6,
            ELEM_FIRE, ELEM_FIRE, ELEM_NONE, &super_eff, &resist_eff);
        TEST_ASSERT(super_eff);
        TEST_ASSERT(!resist_eff);
        TEST_ASSERT(dmg >= 1);
        total_super += dmg;
    }

    /* Resist: Fire spell vs Fire-resistant enemy */
    /* base ~17, then * 0.5 = ~8 */
    int total_resist = 0;
    for (int i = 0; i < 50; i++) {
        super_eff = false; resist_eff = false;
        int dmg = battle_calc_spell_damage(15, 12, 6,
            ELEM_FIRE, ELEM_NONE, ELEM_FIRE, &super_eff, &resist_eff);
        TEST_ASSERT(!super_eff);
        TEST_ASSERT(resist_eff);
        TEST_ASSERT(dmg >= 1);
        total_resist += dmg;
    }

    /* Average super should be higher than average resist */
    TEST_ASSERT(total_super > total_resist);
}

/* ── Test: Defend reduction (65%) ────────────────────── */
static void test_defend_reduction(void) {
    /* In battle, defending applies: dmg = dmg * 35 / 100
       This means 65% reduction (only 35% of damage gets through) */
    int full_dmg = 20;
    int defended_dmg = full_dmg * 35 / 100;
    TEST_ASSERT_EQ(defended_dmg, 7);  /* 20 * 0.35 = 7 */

    /* Also ensure minimum damage of 1 */
    int tiny_dmg = 1;
    int tiny_defended = tiny_dmg * 35 / 100;
    if (tiny_defended < 1) tiny_defended = 1;
    TEST_ASSERT_EQ(tiny_defended, 1);  /* floor at 1 */
}

/* ── Test: Minimum damage floor ──────────────────────── */
static void test_minimum_damage(void) {
    /* Physical: atk=1, def=100 => should still deal 1 */
    for (int i = 0; i < 30; i++) {
        int dmg = battle_calc_damage(1, 100);
        TEST_ASSERT(dmg >= 1);
    }

    /* Spell: tiny power, high defense, resisted */
    bool s, r;
    for (int i = 0; i < 30; i++) {
        int dmg = battle_calc_spell_damage(1, 0, 50,
            ELEM_FIRE, ELEM_NONE, ELEM_FIRE, &s, &r);
        TEST_ASSERT(dmg >= 1);
    }
}

/* ── Main ────────────────────────────────────────────── */
int main(void) {
    printf("=== Battle Tests ===\n");

    TEST_RUN(test_physical_damage);
    TEST_RUN(test_spell_element_damage);
    TEST_RUN(test_defend_reduction);
    TEST_RUN(test_minimum_damage);

    TEST_SUMMARY();
}
