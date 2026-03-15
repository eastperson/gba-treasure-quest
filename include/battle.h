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

/* ── Element Types ────────────────────────────────────── */
typedef enum {
    ELEM_NONE = 0,
    ELEM_FIRE,
    ELEM_ICE,
    ELEM_THUNDER,
} Element;

/* ── Enemy AI Types ──────────────────────────────────── */
typedef enum {
    AI_AGGRESSIVE = 0,  /* always physical attack */
    AI_DEFENSIVE  = 1,  /* defend when HP low, counter next turn */
    AI_CASTER     = 2,  /* elemental magic attack, self-heal when low */
} EnemyAIType;

/* ── Enemy AI Action (returned by pure AI functions) ── */
typedef enum {
    AI_ACT_ATTACK,      /* normal physical attack */
    AI_ACT_DEFEND,      /* defend (take 50% damage, counter next turn) */
    AI_ACT_COUNTER,     /* counter-attack at 1.5x damage */
    AI_ACT_CAST,        /* elemental magic attack */
    AI_ACT_HEAL,        /* self-heal (15% max HP) */
    /* Boss-specific actions */
    AI_ACT_BOSS_SPECIAL,  /* boss special attack (Fire Breath, Tentacle, etc.) */
    AI_ACT_BOSS_HEAL,     /* boss self-heal (Kraken Phase 2) */
} EnemyAIAction;

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
    uint8_t weakness;    /* Element enum — takes 1.5x damage from this */
    uint8_t resistance;  /* Element enum — takes 0.5x damage from this */
    int8_t  drop_item_id;  /* item id to drop, -1 = no drop */
    uint8_t drop_chance;   /* drop probability 0-100 percent */
    uint8_t ai_type;       /* EnemyAIType — AI behavior pattern */
} EnemyData;

/* ── Spell Data ───────────────────────────────────────── */
#define MAX_SPELLS 4

typedef struct {
    char    name[MAX_NAME_LEN];
    int16_t mp_cost;
    int16_t power;      /* damage multiplier (x10) or heal amount */
    bool    heals;      /* true = heals player, false = damages enemy */
    uint8_t element;    /* Element enum for weakness/resistance checks */
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
    bool          enemy_defending; /* true if enemy is defending this turn */
    bool          enemy_counter;   /* true if enemy will counter next turn */
    bool          boss_phase2;     /* true if boss HP <= 50% */
} BattleContext;

/* ── Functions ────────────────────────────────────────── */
void battle_init(BattleContext *bc, Character *hero, int enemy_id, Inventory *inv, Party *party, uint8_t island);
void battle_update(BattleContext *bc, Character *hero);
void battle_render(BattleContext *bc, Character *hero);
int  battle_calc_damage(int atk, int def);
int  battle_calc_spell_damage(int power, int caster_atk, int target_def,
                              uint8_t spell_element, uint8_t enemy_weakness,
                              uint8_t enemy_resistance, bool *out_super, bool *out_resist);
int  battle_get_random_enemy_for_island(int island_id, uint32_t rng);

/* ── AI Functions (pure, testable) ────────────────────── */
EnemyAIAction enemy_ai_choose_action(uint8_t ai_type, int16_t hp, int16_t max_hp,
                                      uint8_t turn_count, bool is_countering, uint32_t rng);
EnemyAIAction boss_ai_choose_action(uint8_t sprite_id, int16_t hp, int16_t max_hp,
                                     uint8_t turn_count, uint32_t rng);

#endif /* BATTLE_H */
