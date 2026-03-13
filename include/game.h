/**
 * game.h — Core game state and logic
 */
#ifndef GAME_H
#define GAME_H

#include <stdint.h>
#include <stdbool.h>

/* ── Game States ───────────────────────────────────────── */
typedef enum {
    STATE_TITLE,
    STATE_WORLD,
    STATE_BATTLE,
    STATE_DIALOGUE,
    STATE_MENU,
    STATE_INVENTORY,
    STATE_SAVE,
    STATE_SHOP,
    STATE_GAME_OVER,
} GameState;

/* ── Character ─────────────────────────────────────────── */
#define MAX_PARTY_SIZE  4
#define MAX_NAME_LEN   16

typedef struct {
    char     name[MAX_NAME_LEN];
    int16_t  hp;
    int16_t  max_hp;
    int16_t  mp;
    int16_t  max_mp;
    int16_t  atk;
    int16_t  def;
    int16_t  spd;
    int16_t  luk;
    uint8_t  level;
    uint16_t exp;
    uint8_t  sprite_id;
    int8_t   weapon_id;  /* -1 = none, 13-15 = weapons */
    int8_t   armor_id;   /* -1 = none, 16-18 = armors */
} Character;

/* ── Party ─────────────────────────────────────────────── */
typedef struct {
    Character members[MAX_PARTY_SIZE];
    uint8_t   count;
    int16_t   gold;
} Party;

/* ── Map Position ──────────────────────────────────────── */
typedef struct {
    int16_t  x;
    int16_t  y;
    uint8_t  map_id;
    uint8_t  direction;  /* 0=down, 1=up, 2=left, 3=right */
} MapPosition;

/* ── Forward Declarations ─────────────────────────────── */
/* Inventory is defined in inventory.h; forward-declare the struct here
   so GameContext can embed it without circular includes. */
#define MAX_INVENTORY  20

typedef struct {
    uint8_t item_id;
    uint8_t count;
} InventorySlot;

typedef struct {
    InventorySlot slots[MAX_INVENTORY];
    uint8_t       count;
} Inventory;

/* ── Menu Context (forward-declared here for GameContext) ── */
typedef struct {
    uint8_t cursor;
    bool    open;
} MenuContext;

/* ── Game Context ──────────────────────────────────────── */
typedef struct {
    GameState   state;
    Party       party;
    Inventory   inventory;
    MapPosition pos;
    MenuContext menu;
    uint32_t    frame_count;
    uint8_t     current_island;   /* 0-6 for seven islands */
    uint8_t     treasures_found;  /* bitmask */
    uint8_t     inv_cursor;       /* inventory screen cursor */
    uint8_t     save_cursor;      /* save slot selection cursor */
    uint8_t     shop_cursor;      /* shop screen cursor */
    bool        running;
    bool        victory;          /* true = all treasures, false = defeat */
} GameContext;

/* ── Core Functions ────────────────────────────────────── */
void game_init(GameContext *ctx);
void game_update(GameContext *ctx);
void game_render(GameContext *ctx);

/* ── State Handlers ────────────────────────────────────── */
void state_title_update(GameContext *ctx);
void state_title_render(GameContext *ctx);
void state_world_update(GameContext *ctx);
void state_world_render(GameContext *ctx);
void state_battle_update(GameContext *ctx);
void state_battle_render(GameContext *ctx);
void state_inventory_update(GameContext *ctx);
void state_inventory_render(GameContext *ctx);
void state_save_update(GameContext *ctx);
void state_save_render(GameContext *ctx);
void state_shop_update(GameContext *ctx);
void state_shop_render(GameContext *ctx);
void state_game_over_update(GameContext *ctx);
void state_game_over_render(GameContext *ctx);

#endif /* GAME_H */
