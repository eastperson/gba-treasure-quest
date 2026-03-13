/**
 * inventory.h — Inventory and item system
 */
#ifndef INVENTORY_H
#define INVENTORY_H

#include "game.h"

/* ── Item Types ──────────────────────────────────────── */
typedef enum {
    ITEM_POTION,
    ITEM_HI_POTION,
    ITEM_ETHER,
    ITEM_ANTIDOTE,
    ITEM_BOMB,
    ITEM_KEY,
    ITEM_TREASURE,
    ITEM_EQUIPMENT,
} ItemType;

/* ── Item Data ───────────────────────────────────────── */
typedef struct {
    uint8_t  id;
    char     name[MAX_NAME_LEN];
    ItemType type;
    int16_t  value;       /* heal amount, damage, etc. */
    uint16_t price;
    char     description[32];
} ItemData;

/* Inventory and InventorySlot structs are defined in game.h */

/* ── Functions ───────────────────────────────────────── */
void            inventory_init(Inventory *inv);
bool            inventory_add(Inventory *inv, int item_id, int count);
void            inventory_remove(Inventory *inv, int slot_index, int count);
bool            inventory_use(Inventory *inv, int slot_index, Character *target);
const ItemData *inventory_get_item_data(int item_id);
void            inventory_render(Inventory *inv, int cursor_pos);

#endif /* INVENTORY_H */
