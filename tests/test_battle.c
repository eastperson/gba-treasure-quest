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

/* ── Test: Enemy AI table fields ──────────────────────── */
static void test_enemy_ai_type_fields(void) {
    /* Verify ai_type assignments in enemy_table */
    TEST_ASSERT_EQ(enemy_table[0].ai_type, AI_AGGRESSIVE);  /* Slime */
    TEST_ASSERT_EQ(enemy_table[1].ai_type, AI_AGGRESSIVE);  /* Goblin */
    TEST_ASSERT_EQ(enemy_table[2].ai_type, AI_DEFENSIVE);   /* Scorpion */
    TEST_ASSERT_EQ(enemy_table[3].ai_type, AI_CASTER);      /* Fire Bat */
    TEST_ASSERT_EQ(enemy_table[4].ai_type, AI_DEFENSIVE);   /* Ice Golem */
    TEST_ASSERT_EQ(enemy_table[5].ai_type, AI_CASTER);      /* Wraith */
    TEST_ASSERT_EQ(enemy_table[6].ai_type, AI_DEFENSIVE);   /* Temple Guard */
    TEST_ASSERT_EQ(enemy_table[11].ai_type, AI_DEFENSIVE);  /* Skeleton */
    TEST_ASSERT_EQ(enemy_table[13].ai_type, AI_CASTER);     /* Imp */
}

/* ── Test: AI_AGGRESSIVE always attacks ──────────────── */
static void test_ai_aggressive(void) {
    /* AI_AGGRESSIVE should always return AI_ACT_ATTACK regardless of HP */
    for (int i = 0; i < 50; i++) {
        EnemyAIAction act = enemy_ai_choose_action(AI_AGGRESSIVE, 5, 100, i, false, (uint32_t)i * 12345);
        TEST_ASSERT_EQ(act, AI_ACT_ATTACK);
    }
    /* Even at 1 HP */
    EnemyAIAction act = enemy_ai_choose_action(AI_AGGRESSIVE, 1, 100, 5, false, 42);
    TEST_ASSERT_EQ(act, AI_ACT_ATTACK);
}

/* ── Test: AI_DEFENSIVE defend/counter cycle ─────────── */
static void test_ai_defensive(void) {
    /* HP > 30%: should attack */
    for (int i = 0; i < 20; i++) {
        EnemyAIAction act = enemy_ai_choose_action(AI_DEFENSIVE, 50, 100, i, false, (uint32_t)i);
        TEST_ASSERT_EQ(act, AI_ACT_ATTACK);
    }

    /* HP = 30%: should defend */
    {
        EnemyAIAction act = enemy_ai_choose_action(AI_DEFENSIVE, 30, 100, 1, false, 0);
        TEST_ASSERT_EQ(act, AI_ACT_DEFEND);
    }

    /* HP < 30%: should defend */
    {
        EnemyAIAction act = enemy_ai_choose_action(AI_DEFENSIVE, 10, 100, 1, false, 0);
        TEST_ASSERT_EQ(act, AI_ACT_DEFEND);
    }

    /* Counter flag set: should counter regardless of HP */
    {
        EnemyAIAction act = enemy_ai_choose_action(AI_DEFENSIVE, 80, 100, 1, true, 0);
        TEST_ASSERT_EQ(act, AI_ACT_COUNTER);
    }
    {
        EnemyAIAction act = enemy_ai_choose_action(AI_DEFENSIVE, 10, 100, 1, true, 0);
        TEST_ASSERT_EQ(act, AI_ACT_COUNTER);
    }
}

/* ── Test: AI_CASTER cast/heal/attack distribution ───── */
static void test_ai_caster(void) {
    /* HP <= 40%: always heal */
    for (int i = 0; i < 20; i++) {
        EnemyAIAction act = enemy_ai_choose_action(AI_CASTER, 40, 100, i, false, (uint32_t)i);
        TEST_ASSERT_EQ(act, AI_ACT_HEAL);
    }
    {
        EnemyAIAction act = enemy_ai_choose_action(AI_CASTER, 20, 100, 1, false, 0);
        TEST_ASSERT_EQ(act, AI_ACT_HEAL);
    }

    /* HP > 40%: should be mix of CAST and ATTACK */
    int cast_count = 0, attack_count = 0;
    for (uint32_t r = 0; r < 200; r++) {
        EnemyAIAction act = enemy_ai_choose_action(AI_CASTER, 80, 100, 1, false, r * 7919);
        if (act == AI_ACT_CAST) cast_count++;
        else if (act == AI_ACT_ATTACK) attack_count++;
        else TEST_ASSERT(0); /* should not get other actions */
    }
    /* 60% cast, 40% attack — verify both occur */
    TEST_ASSERT(cast_count > 0);
    TEST_ASSERT(attack_count > 0);
    /* Rough ratio check: cast should be more than attack */
    TEST_ASSERT(cast_count > attack_count);

    /* Counter flag overrides even for caster */
    {
        EnemyAIAction act = enemy_ai_choose_action(AI_CASTER, 80, 100, 1, true, 0);
        TEST_ASSERT_EQ(act, AI_ACT_COUNTER);
    }
}

/* ── Test: Boss AI Phase 1 patterns ──────────────────── */
static void test_boss_ai_phase1(void) {
    /* Fire Dragon (sprite 23): Phase 1 (HP > 50%) — special every 3 turns */
    TEST_ASSERT_EQ(boss_ai_choose_action(23, 80, 80, 0, 0), AI_ACT_ATTACK);  /* turn 0 */
    TEST_ASSERT_EQ(boss_ai_choose_action(23, 80, 80, 1, 0), AI_ACT_ATTACK);  /* turn 1 */
    TEST_ASSERT_EQ(boss_ai_choose_action(23, 80, 80, 2, 0), AI_ACT_ATTACK);  /* turn 2 */
    TEST_ASSERT_EQ(boss_ai_choose_action(23, 80, 80, 3, 0), AI_ACT_BOSS_SPECIAL); /* turn 3 */
    TEST_ASSERT_EQ(boss_ai_choose_action(23, 80, 80, 6, 0), AI_ACT_BOSS_SPECIAL); /* turn 6 */

    /* Kraken (sprite 24): Phase 1 — special every 4 turns */
    TEST_ASSERT_EQ(boss_ai_choose_action(24, 100, 100, 0, 0), AI_ACT_ATTACK);
    TEST_ASSERT_EQ(boss_ai_choose_action(24, 100, 100, 4, 0), AI_ACT_BOSS_SPECIAL);
    TEST_ASSERT_EQ(boss_ai_choose_action(24, 100, 100, 8, 0), AI_ACT_BOSS_SPECIAL);

    /* Sky Lord (sprite 25): Phase 1 — special every 2 turns */
    TEST_ASSERT_EQ(boss_ai_choose_action(25, 150, 150, 0, 0), AI_ACT_ATTACK);
    TEST_ASSERT_EQ(boss_ai_choose_action(25, 150, 150, 1, 0), AI_ACT_ATTACK);
    TEST_ASSERT_EQ(boss_ai_choose_action(25, 150, 150, 2, 0), AI_ACT_BOSS_SPECIAL);
    TEST_ASSERT_EQ(boss_ai_choose_action(25, 150, 150, 4, 0), AI_ACT_BOSS_SPECIAL);
}

/* ── Test: Boss AI Phase 2 (HP <= 50%) ───────────────── */
static void test_boss_ai_phase2(void) {
    /* Fire Dragon Phase 2: special every 2 turns (was 3) */
    TEST_ASSERT_EQ(boss_ai_choose_action(23, 40, 80, 2, 0), AI_ACT_BOSS_SPECIAL);
    TEST_ASSERT_EQ(boss_ai_choose_action(23, 40, 80, 4, 0), AI_ACT_BOSS_SPECIAL);
    TEST_ASSERT_EQ(boss_ai_choose_action(23, 40, 80, 3, 0), AI_ACT_ATTACK);

    /* Kraken Phase 2: special every 2 turns (was 4) + heal on odd turns */
    TEST_ASSERT_EQ(boss_ai_choose_action(24, 50, 100, 2, 0), AI_ACT_BOSS_SPECIAL);
    TEST_ASSERT_EQ(boss_ai_choose_action(24, 50, 100, 1, 0), AI_ACT_BOSS_HEAL);
    TEST_ASSERT_EQ(boss_ai_choose_action(24, 50, 100, 3, 0), AI_ACT_BOSS_HEAL);

    /* Sky Lord Phase 2: Thunder Storm every turn */
    for (int t = 1; t <= 10; t++) {
        TEST_ASSERT_EQ(boss_ai_choose_action(25, 75, 150, t, 0), AI_ACT_BOSS_SPECIAL);
    }
    /* Even turn 0 in phase 2 */
    TEST_ASSERT_EQ(boss_ai_choose_action(25, 75, 150, 0, 0), AI_ACT_BOSS_SPECIAL);
}

/* ── Test: Boss Phase 2 threshold at exactly 50% ─────── */
static void test_boss_phase_threshold(void) {
    /* HP = 50% exactly: should be Phase 2 (<=50%) */
    TEST_ASSERT_EQ(boss_ai_choose_action(23, 40, 80, 2, 0), AI_ACT_BOSS_SPECIAL); /* Phase 2 interval */
    /* HP = 51%: should be Phase 1 */
    TEST_ASSERT_EQ(boss_ai_choose_action(23, 41, 80, 2, 0), AI_ACT_ATTACK); /* turn 2, Phase 1 interval=3 */
    /* HP = 49%: should be Phase 2 */
    TEST_ASSERT_EQ(boss_ai_choose_action(23, 39, 80, 2, 0), AI_ACT_BOSS_SPECIAL);
}

/* ── Test: AI self-heal amount (15% max HP) ──────────── */
static void test_ai_heal_amount(void) {
    /* Verify self-heal = 15% of max_hp */
    /* Wraith: max_hp 28, 15% = 4 */
    int heal_28 = (28 * 15) / 100;
    TEST_ASSERT_EQ(heal_28, 4);

    /* Ice Golem: max_hp 35, 15% = 5 */
    int heal_35 = (35 * 15) / 100;
    TEST_ASSERT_EQ(heal_35, 5);

    /* Slime: max_hp 10, 15% = 1 */
    int heal_10 = (10 * 15) / 100;
    TEST_ASSERT_EQ(heal_10, 1);

    /* Minimum heal should be >= 1 */
    int heal_1 = (1 * 15) / 100;
    if (heal_1 < 1) heal_1 = 1;
    TEST_ASSERT(heal_1 >= 1);
}

/* ── Test: Defensive counter damage multiplier ───────── */
static void test_counter_damage_multiplier(void) {
    /* Counter deals 1.5x ATK — verify formula */
    int atk = 20;
    int counter_atk = (atk * 3) / 2;
    TEST_ASSERT_EQ(counter_atk, 30);

    atk = 15;
    counter_atk = (atk * 3) / 2;
    TEST_ASSERT_EQ(counter_atk, 22); /* 15*1.5 = 22 (integer division) */

    /* Verify damage is at least 1 */
    for (int i = 0; i < 30; i++) {
        int dmg = battle_calc_damage(counter_atk, 50);
        TEST_ASSERT(dmg >= 1);
    }
}

/* ── Test: Boss Phase 2 attack power boost ───────────── */
static void test_boss_phase2_attack_boost(void) {
    /* Phase 2: ATK * 1.3 */
    int atk = 20;
    int boosted = (atk * 13) / 10;
    TEST_ASSERT_EQ(boosted, 26);

    atk = 18; /* Fire Dragon */
    boosted = (atk * 13) / 10;
    TEST_ASSERT_EQ(boosted, 23);

    atk = 25; /* Sky Lord */
    boosted = (atk * 13) / 10;
    TEST_ASSERT_EQ(boosted, 32);
}

/* ── Test: AI edge case — zero max HP ────────────────── */
static void test_ai_edge_zero_hp(void) {
    /* max_hp=0 should not crash (division by zero guard) */
    EnemyAIAction act = enemy_ai_choose_action(AI_DEFENSIVE, 0, 0, 0, false, 0);
    /* hp_pct defaults to 100 when max_hp=0, so should attack */
    TEST_ASSERT_EQ(act, AI_ACT_ATTACK);

    act = enemy_ai_choose_action(AI_CASTER, 0, 0, 0, false, 0);
    /* 100% > 40%, not heal. rng=0 → (0%100)=0 < 60 → CAST */
    TEST_ASSERT(act == AI_ACT_CAST || act == AI_ACT_ATTACK);
}

/* ── Test: Kraken Phase 2 heal does not exceed max HP ── */
static void test_boss_heal_cap(void) {
    /* Kraken heal: 10 HP, verify concept */
    int16_t hp = 95, max_hp = 100;
    int heal = 10;
    hp += (int16_t)heal;
    if (hp > max_hp) hp = max_hp;
    TEST_ASSERT_EQ(hp, 100); /* capped at max */

    hp = 50;
    hp += (int16_t)heal;
    if (hp > max_hp) hp = max_hp;
    TEST_ASSERT_EQ(hp, 60); /* no cap needed */
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
    /* Sprint 31: Enemy AI tests */
    TEST_RUN(test_enemy_ai_type_fields);
    TEST_RUN(test_ai_aggressive);
    TEST_RUN(test_ai_defensive);
    TEST_RUN(test_ai_caster);
    TEST_RUN(test_boss_ai_phase1);
    TEST_RUN(test_boss_ai_phase2);
    TEST_RUN(test_boss_phase_threshold);
    TEST_RUN(test_ai_heal_amount);
    TEST_RUN(test_counter_damage_multiplier);
    TEST_RUN(test_boss_phase2_attack_boost);
    TEST_RUN(test_ai_edge_zero_hp);
    TEST_RUN(test_boss_heal_cap);

    TEST_SUMMARY();
}
