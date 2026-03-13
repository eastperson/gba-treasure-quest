/**
 * party.h — Character growth and party management
 */
#ifndef PARTY_H
#define PARTY_H

#include "game.h"

/* ── Level-Up Bonus ──────────────────────────────────── */
typedef struct {
    int16_t hp_bonus;
    int16_t mp_bonus;
    int16_t atk_bonus;
    int16_t def_bonus;
    int16_t spd_bonus;
    int16_t luk_bonus;
} LevelUpBonus;

/* ── Functions ───────────────────────────────────────── */
uint16_t party_calc_exp_needed(uint8_t level);
bool     party_try_level_up(Character *ch);
void     party_apply_level_up(Character *ch);
void     party_heal_full(Character *ch);
bool     party_is_alive(Character *ch);
void     party_add_member(Party *party, const char *name,
                          int16_t hp, int16_t mp,
                          int16_t atk, int16_t def,
                          int16_t spd, uint8_t sprite_id);

#endif /* PARTY_H */
