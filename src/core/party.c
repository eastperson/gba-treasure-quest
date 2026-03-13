/**
 * party.c — Character growth and party management
 * Platform-independent: no GBA or SDL headers here.
 */
#include "party.h"
#include <string.h>

/* ── Level-Up Bonus Tables ───────────────────────────── */
/* Bracket 0: levels 1-5  (fast growth)   */
/* Bracket 1: levels 6-10 (moderate)      */
/* Bracket 2: levels 11+  (slow growth)   */
static const LevelUpBonus g_level_bonuses[3] = {
    /* fast:     HP  MP  ATK DEF SPD LUK */
    {            7,  4,   3,  2,  2,  1 },
    /* moderate: */
    {            5,  3,   2,  1,  1,  1 },
    /* slow:     */
    {            3,  2,   1,  1,  1,  0 },
};

/* ── Helpers ─────────────────────────────────────────── */

static const LevelUpBonus *get_bonus_for_level(uint8_t level) {
    if (level <= 5)  return &g_level_bonuses[0];
    if (level <= 10) return &g_level_bonuses[1];
    return &g_level_bonuses[2];
}

/* ── Public API ──────────────────────────────────────── */

uint16_t party_calc_exp_needed(uint8_t level) {
    return (uint16_t)(level * 50);
}

bool party_try_level_up(Character *ch) {
    uint16_t needed = party_calc_exp_needed(ch->level);
    if (ch->exp >= needed) {
        ch->exp -= needed;
        party_apply_level_up(ch);
        return true;
    }
    return false;
}

void party_apply_level_up(Character *ch) {
    const LevelUpBonus *bonus = get_bonus_for_level(ch->level);

    ch->level++;
    ch->max_hp += bonus->hp_bonus;
    ch->max_mp += bonus->mp_bonus;
    ch->atk    += bonus->atk_bonus;
    ch->def    += bonus->def_bonus;
    ch->spd    += bonus->spd_bonus;
    ch->luk    += bonus->luk_bonus;

    /* Restore HP/MP to new max */
    ch->hp = ch->max_hp;
    ch->mp = ch->max_mp;
}

void party_heal_full(Character *ch) {
    ch->hp = ch->max_hp;
    ch->mp = ch->max_mp;
}

bool party_is_alive(Character *ch) {
    return ch->hp > 0;
}

void party_add_member(Party *party, const char *name,
                      int16_t hp, int16_t mp,
                      int16_t atk, int16_t def,
                      int16_t spd, uint8_t sprite_id) {
    if (party->count >= MAX_PARTY_SIZE) return;

    Character *ch = &party->members[party->count];
    memset(ch, 0, sizeof(Character));
    strncpy(ch->name, name, MAX_NAME_LEN - 1);
    ch->name[MAX_NAME_LEN - 1] = '\0';
    ch->hp      = hp;
    ch->max_hp  = hp;
    ch->mp      = mp;
    ch->max_mp  = mp;
    ch->atk     = atk;
    ch->def     = def;
    ch->spd     = spd;
    ch->luk     = 3;   /* default luck */
    ch->level   = 1;
    ch->exp     = 0;
    ch->sprite_id = sprite_id;

    party->count++;
}
