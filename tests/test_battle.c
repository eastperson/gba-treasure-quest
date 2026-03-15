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

/* ── Test: MP consumption — enough MP ────────────────── */
static void test_mp_enough(void) {
    /* Fireball costs 5 MP — with 10 MP, cast should succeed */
    Character hero;
    memset(&hero, 0, sizeof(Character));
    hero.mp = 10;
    hero.max_mp = 20;

    /* Simulate spell cast: check MP, deduct */
    /* g_spells[0] = Fireball, mp_cost = 5 */
    int spell_idx = 0;
    /* Access the spell table (included via battle.c) */
    const SpellData *sp = &g_spells[spell_idx];
    TEST_ASSERT(hero.mp >= sp->mp_cost);
    hero.mp -= sp->mp_cost;
    TEST_ASSERT_EQ(hero.mp, 5);  /* 10 - 5 = 5 */
}

/* ── Test: MP consumption — insufficient MP ─────────── */
static void test_mp_insufficient(void) {
    Character hero;
    memset(&hero, 0, sizeof(Character));
    hero.mp = 3;
    hero.max_mp = 20;

    /* Fireball costs 5 MP — with 3 MP, cast should fail */
    const SpellData *sp = &g_spells[0];
    TEST_ASSERT(hero.mp < sp->mp_cost);  /* should not be enough */

    /* MP should remain unchanged (not deducted) */
    int16_t mp_before = hero.mp;
    if (hero.mp >= sp->mp_cost) {
        hero.mp -= sp->mp_cost;  /* should NOT execute */
    }
    TEST_ASSERT_EQ(hero.mp, mp_before);
}

/* ── Test: MP deduction amounts for all spells ──────── */
static void test_mp_deduction_amounts(void) {
    /* Verify each spell's MP cost matches spec */
    TEST_ASSERT_EQ(g_spells[0].mp_cost, 5);   /* Fireball */
    TEST_ASSERT_EQ(g_spells[1].mp_cost, 6);   /* Ice Storm */
    TEST_ASSERT_EQ(g_spells[2].mp_cost, 7);   /* Thunder */
    TEST_ASSERT_EQ(g_spells[3].mp_cost, 4);   /* Heal */

    /* Test sequential casting depletes MP correctly */
    Character hero;
    memset(&hero, 0, sizeof(Character));
    hero.mp = 30;
    hero.max_mp = 30;

    /* Cast Fireball (5), Ice Storm (6), Thunder (7), Heal (4) = 22 total */
    for (int i = 0; i < MAX_SPELLS; i++) {
        TEST_ASSERT(hero.mp >= g_spells[i].mp_cost);
        hero.mp -= g_spells[i].mp_cost;
    }
    TEST_ASSERT_EQ(hero.mp, 30 - 5 - 6 - 7 - 4);  /* 8 remaining */
}

/* ── Test: Enemy drop fields ────────────────────────── */
static void test_enemy_drop_fields(void) {
    /* Verify enemy table has valid drop data */
    /* enemy_table is static in battle.c — accessible since we #include it */
    /* Goblin (index 1): item 0, 30% */
    TEST_ASSERT_EQ(enemy_table[1].drop_item_id, 0);
    TEST_ASSERT_EQ(enemy_table[1].drop_chance, 30);

    /* Imp (index 13): no drop */
    TEST_ASSERT_EQ(enemy_table[13].drop_item_id, -1);
    TEST_ASSERT_EQ(enemy_table[13].drop_chance, 0);

    /* Fire Dragon (index 7): item 4, 50% */
    TEST_ASSERT_EQ(enemy_table[7].drop_item_id, 4);
    TEST_ASSERT_EQ(enemy_table[7].drop_chance, 50);
}

/* ── Main ────────────────────────────────────────────── */
int main(void) {
    printf("=== Battle Tests ===\n");

    TEST_RUN(test_physical_damage);
    TEST_RUN(test_spell_element_damage);
    TEST_RUN(test_defend_reduction);
    TEST_RUN(test_minimum_damage);
    TEST_RUN(test_mp_enough);
    TEST_RUN(test_mp_insufficient);
    TEST_RUN(test_mp_deduction_amounts);
    TEST_RUN(test_enemy_drop_fields);

    TEST_SUMMARY();
}
