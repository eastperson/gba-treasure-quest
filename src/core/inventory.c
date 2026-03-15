/**
 * inventory.c — Inventory and item system
 * Platform-independent: no GBA or SDL headers here.
 */
#include "inventory.h"
#include "platform.h"
#include <string.h>
#include <stdio.h>

/* ── Item Database ───────────────────────────────────── */
/* IDs: 0-5 consumables/key, 6-12 treasure items (one per island) */
#define ITEM_DB_COUNT  19

static const ItemData g_item_db[ITEM_DB_COUNT] = {
    /* id  name             type            value  price  description          */
    {  0, "Potion",         ITEM_POTION,      15,    50, "Restores 15 HP"     },
    {  1, "Hi-Potion",      ITEM_HI_POTION,   50,   150, "Restores 50 HP"     },
    {  2, "Ether",          ITEM_ETHER,       10,    80, "Restores 10 MP"     },
    {  3, "Antidote",       ITEM_ANTIDOTE,     0,    30, "Cures poison"       },
    {  4, "Bomb",           ITEM_BOMB,        20,    50, "Deals 20 damage"    },
    {  5, "Skeleton Key",   ITEM_KEY,          0,   200, "Unlocks locked doors"},
    {  6, "Coral Crown",    ITEM_TREASURE,     0,     0, "Crown of the sea king"  },
    {  7, "Emerald Blade",  ITEM_TREASURE,     0,     0, "Sword of forest guardian"},
    {  8, "Sun Amulet",     ITEM_TREASURE,     0,     0, "Glows with desert heat" },
    {  9, "Flame Ruby",     ITEM_TREASURE,     0,     0, "Gem from volcano's heart"},
    { 10, "Frost Crystal",  ITEM_TREASURE,     0,     0, "Crystal of eternal ice" },
    { 11, "Abyssal Pearl",  ITEM_TREASURE,     0,     0, "Pearl from deep ruins"  },
    { 12, "Sky Scepter",    ITEM_TREASURE,     0,     0, "Scepter of the heavens" },
    /* Equipment — weapons (id 13-15) */
    { 13, "Short Sword",    ITEM_EQUIPMENT,    3,   100, "ATK +3"               },
    { 14, "Iron Blade",     ITEM_EQUIPMENT,    6,   300, "ATK +6"               },
    { 15, "Flame Sword",    ITEM_EQUIPMENT,   10,   600, "ATK +10"              },
    /* Equipment — armor (id 16-18) */
    { 16, "Leather Armor",  ITEM_EQUIPMENT,    3,    80, "DEF +3"               },
    { 17, "Chain Mail",     ITEM_EQUIPMENT,    6,   250, "DEF +6"               },
    { 18, "Dragon Plate",   ITEM_EQUIPMENT,   12,   800, "DEF +12"              },
};

/* ── Public API ──────────────────────────────────────── */

void inventory_init(Inventory *inv) {
    memset(inv, 0, sizeof(Inventory));
}

bool inventory_add(Inventory *inv, int item_id, int count) {
    if (item_id < 0 || item_id >= ITEM_DB_COUNT || count <= 0) {
        return false;
    }

    /* Try to stack with existing slot */
    for (int i = 0; i < inv->count; i++) {
        if (inv->slots[i].item_id == (uint8_t)item_id) {
            int new_count = inv->slots[i].count + count;
            if (new_count > 99) new_count = 99;
            inv->slots[i].count = (uint8_t)new_count;
            return true;
        }
    }

    /* Find empty slot */
    if (inv->count >= MAX_INVENTORY) {
        return false;
    }

    inv->slots[inv->count].item_id = (uint8_t)item_id;
    inv->slots[inv->count].count   = (uint8_t)(count > 99 ? 99 : count);
    inv->count++;
    return true;
}

void inventory_remove(Inventory *inv, int slot_index, int count) {
    if (slot_index < 0 || slot_index >= inv->count) return;

    if (inv->slots[slot_index].count <= (uint8_t)count) {
        /* Remove slot entirely — shift remaining slots */
        for (int i = slot_index; i < inv->count - 1; i++) {
            inv->slots[i] = inv->slots[i + 1];
        }
        memset(&inv->slots[inv->count - 1], 0, sizeof(InventorySlot));
        inv->count--;
    } else {
        inv->slots[slot_index].count -= (uint8_t)count;
    }
}

bool inventory_use(Inventory *inv, int slot_index, Character *target) {
    if (slot_index < 0 || slot_index >= inv->count) return false;

    const ItemData *item = inventory_get_item_data(inv->slots[slot_index].item_id);
    if (!item) return false;

    switch (item->type) {
        case ITEM_POTION:
        case ITEM_HI_POTION: {
            /* Heal HP */
            target->hp += item->value;
            if (target->hp > target->max_hp) {
                target->hp = target->max_hp;
            }
            break;
        }
        case ITEM_ETHER: {
            /* Restore MP */
            target->mp += item->value;
            if (target->mp > target->max_mp) {
                target->mp = target->max_mp;
            }
            break;
        }
        case ITEM_ANTIDOTE: {
            /* Cure poison — status effect system TBD */
            break;
        }
        case ITEM_EQUIPMENT:
            /* Equip the item on the target character */
            return inventory_equip(inv, target, slot_index);
        case ITEM_BOMB:
        case ITEM_KEY:
        case ITEM_TREASURE:
            /* These items cannot be "used" on a character directly */
            return false;
    }

    /* Consume 1 from stack */
    inventory_remove(inv, slot_index, 1);
    return true;
}

bool inventory_equip(Inventory *inv, Character *ch, int slot_index) {
    if (slot_index < 0 || slot_index >= inv->count) return false;

    const ItemData *item = inventory_get_item_data(inv->slots[slot_index].item_id);
    if (!item || item->type != ITEM_EQUIPMENT) return false;

    int item_id = item->id;

    /* Determine if weapon (13-15) or armor (16-18) */
    if (item_id >= 13 && item_id <= 15) {
        /* Weapon — check if inventory has room for old weapon before swap */
        if (ch->weapon_id >= 0 && inv->count >= MAX_INVENTORY) {
            /* Inventory full — can't unequip old weapon to make room */
            return false;
        }
        /* Unequip old weapon first */
        if (ch->weapon_id >= 0) {
            const ItemData *old = inventory_get_item_data(ch->weapon_id);
            if (old) {
                ch->atk -= old->value;
                inventory_add(inv, ch->weapon_id, 1);
            }
        }
        /* Equip new weapon */
        ch->weapon_id = (int8_t)item_id;
        ch->atk += item->value;
        inventory_remove(inv, slot_index, 1);
        return true;
    } else if (item_id >= 16 && item_id <= 18) {
        /* Armor — check if inventory has room for old armor before swap */
        if (ch->armor_id >= 0 && inv->count >= MAX_INVENTORY) {
            /* Inventory full — can't unequip old armor to make room */
            return false;
        }
        /* Unequip old armor first */
        if (ch->armor_id >= 0) {
            const ItemData *old = inventory_get_item_data(ch->armor_id);
            if (old) {
                ch->def -= old->value;
                inventory_add(inv, ch->armor_id, 1);
            }
        }
        /* Equip new armor */
        ch->armor_id = (int8_t)item_id;
        ch->def += item->value;
        inventory_remove(inv, slot_index, 1);
        return true;
    }

    return false;
}

const ItemData *inventory_get_item_data(int item_id) {
    if (item_id < 0 || item_id >= ITEM_DB_COUNT) return NULL;
    return &g_item_db[item_id];
}

void inventory_render(Inventory *inv, int cursor_pos) {
    /* Background panel */
    platform_draw_rect(16, 8, 208, 144, 0x0842);

    /* Title */
    platform_draw_text(80, 12, "INVENTORY", 0x7FFF);

    if (inv->count == 0) {
        platform_draw_text(72, 70, "No items", 0x5294);
        return;
    }

    /* Item list — show up to 8 visible items with scrolling */
    int scroll_offset = 0;
    int visible_count = 8;
    if (cursor_pos >= visible_count) {
        scroll_offset = cursor_pos - visible_count + 1;
    }

    for (int i = 0; i < visible_count && (i + scroll_offset) < inv->count; i++) {
        int slot_idx = i + scroll_offset;
        const ItemData *item = inventory_get_item_data(inv->slots[slot_idx].item_id);
        if (!item) continue;

        int y = 28 + i * 14;
        uint16_t color = (slot_idx == cursor_pos) ? 0x03FF : 0x7FFF;

        /* Cursor indicator */
        if (slot_idx == cursor_pos) {
            platform_draw_text(20, y, ">", 0x03FF);
        }

        /* Item name and count */
        char line[40];
        snprintf(line, sizeof(line), "%-14s x%d", item->name, inv->slots[slot_idx].count);
        platform_draw_text(32, y, line, color);
    }

    /* Description of selected item at bottom */
    if (cursor_pos < inv->count) {
        const ItemData *sel = inventory_get_item_data(inv->slots[cursor_pos].item_id);
        if (sel) {
            platform_draw_rect(16, 140, 208, 12, 0x0421);
            platform_draw_text(20, 142, sel->description, 0x5EF7);
        }
    }
}
