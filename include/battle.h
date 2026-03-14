/**
 * battle.h — Turn-based battle system
 */
#ifndef BATTLE_H
#define BATTLE_H

#include "game.h"

/* ── Battle States ────────────────────────────────────── */
typedef enum {
    BATTLE_START,
    BATTLE_COMMAND,
    BATTLE_MAGIC_SELECT,
    BATTLE_ITEM_SELECT,
    BATTLE_SKILL_SELECT,
    BATTLE_EXECUTE,
    BATTLE_PARTY_TURN,
    BATTLE_ENEMY_TURN,
    BATTLE_RESULT,
    BATTLE_WIN,
    BATTLE_LOSE,
} BattleState;

/* ── Battle Commands ──────────────────────────────────── */
typedef enum {
    CMD_ATTACK,
    CMD_MAGIC,
    CMD_DEFEND,
    CMD_ITEM,
    CMD_SKILL,
    CMD_FLEE,
    CMD_COUNT,
} BattleCommand;

/* ── Enemy Data ───────────────────────────────────────── */
typedef struct {
    char    name[MAX_NAME_LEN];
    int16_t hp;
    int16_t max_hp;
    int16_t atk;
    int16_t def;
    int16_t spd;
    uint16_t exp_reward;
    uint16_t gold_reward;
    uint8_t sprite_id;
} EnemyData;

/* ── Spell Data ───────────────────────────────────────── */
#define MAX_SPELLS 4

typedef struct {
    char    name[MAX_NAME_LEN];
    int16_t mp_cost;
    int16_t power;      /* damage multiplier (x10) or heal amount */
    bool    heals;      /* true = heals player, false = damages enemy */
} SpellData;

/* ── Battle Context ───────────────────────────────────── */
typedef struct {
    BattleState   state;
    EnemyData     enemy;
    BattleCommand selected_command;
    uint8_t       cursor_pos;
    uint8_t       spell_cursor;
    uint8_t       item_cursor;
    int16_t       turn_timer;
    int16_t       damage_display;
    char          message[64];
    bool          player_defending;
    Inventory    *inv;   /* pointer to player inventory for battle items */
    Party        *party; /* pointer to full party for display */
    uint8_t       boss_turn_count; /* tracks boss AI turn patterns */
    bool          hero_stunned;    /* true if hero is stunned this turn */
    uint8_t       island;         /* current island for scaling */
    uint8_t       party_turn_idx; /* which party member is acting in PARTY_TURN */
    bool          berserk_active; /* Drake's berserk: next turn DEF=0 */
} BattleContext;

/* ── Functions ────────────────────────────────────────── */
void battle_init(BattleContext *bc, Character *hero, int enemy_id, Inventory *inv, Party *party, uint8_t island);
void battle_update(BattleContext *bc, Character *hero);
void battle_render(BattleContext *bc, Character *hero);
int  battle_calc_damage(int atk, int def);
int  battle_get_random_enemy_for_island(int island_id, uint32_t rng);

#endif /* BATTLE_H */
