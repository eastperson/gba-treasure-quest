/**
 * battle.c — Turn-based combat implementation
 * Platform-independent: no GBA or SDL headers here.
 */
#include "battle.h"
#include "inventory.h"
#include "effects.h"
#include "platform.h"
#include <string.h>
#include <stdio.h>

/* ── Pseudo-random number generator ───────────────────── */
static uint32_t s_battle_seed = 0;

static uint32_t battle_rand(void) {
    s_battle_seed = s_battle_seed * 1103515245u + 12345u;
    return (s_battle_seed >> 16) & 0x7FFF;
}

/* ── Enemy Table (balanced per-island difficulty) ─────── */
#define ENEMY_COUNT 14

static const EnemyData enemy_table[ENEMY_COUNT] = {
    /* name             hp  max  atk def spd  exp  gold sprite */
    /* Island 0-1 enemies (indices 0-1) */
    { "Slime",          10, 10,   4,  2,  3,   5,   3,  16 },
    { "Goblin",         18, 18,   7,  3,  5,  12,   8,  17 },
    /* Island 2-3 enemies (indices 2-3) */
    { "Scorpion",       22, 22,  10,  4,  6,  18,  12,  18 },
    { "Fire Bat",       16, 16,  12,  3,  9,  15,  10,  19 },
    /* Island 4-5 enemies (indices 4-5) */
    { "Ice Golem",      35, 35,  13,  8,  3,  25,  18,  20 },
    { "Wraith",         28, 28,  15,  5,  7,  22,  15,  21 },
    /* Island 6 enemy (index 6) */
    { "Temple Guard",   40, 40,  16, 10,  5,  30,  25,  22 },
    /* Bosses (indices 7-9) */
    /* Island 3 boss */
    { "Fire Dragon",    80, 80,  18, 10,  6,  50,  40,  23 },
    /* Island 5 boss */
    { "Kraken",        100,100,  20, 12,  5,  70,  50,  24 },
    /* Island 6 boss */
    { "Sky Lord",      150,150,  25, 15,  7, 100,  80,  25 },
    /* Legacy enemies kept for compatibility (indices 10-13) */
    { "Wolf",           15, 15,   9,  2,  8,  10,   5,  18 },
    { "Skeleton",       25, 25,   8,  6,  4,  18,  15,  19 },
    { "Boss Pirate",    60, 60,  12,  8,  6,  50,  40,  20 },
    { "Imp",            12, 12,   5,  2,  7,   7,   5,  16 },
};

/* ── Command names for menu ───────────────────────────── */
static const char *cmd_names[CMD_COUNT] = {
    "Attack",
    "Magic",
    "Defend",
    "Item",
    "Flee",
};

/* ── Spell Table ─────────────────────────────────────── */
static const SpellData g_spells[MAX_SPELLS] = {
    { "Fire",    5,  15, false },  /* 5 MP, 15 power damage */
    { "Ice",     8,  22, false },  /* 8 MP, 22 power damage */
    { "Thunder", 12, 30, false },  /* 12 MP, 30 power damage */
    { "Heal",    4,  20, true  },  /* 4 MP, heals 20 HP */
};

/* ── Damage Calculation ───────────────────────────────── */
int battle_calc_damage(int atk, int def) {
    int base = atk - def;
    int variance = (int)(battle_rand() % 5) - 2;  /* -2 to +2 */
    int dmg = base + variance;
    if (dmg < 1) dmg = 1;
    return dmg;
}

/* ── Initialization ───────────────────────────────────── */
void battle_init(BattleContext *bc, Character *hero, int enemy_id, Inventory *inv, Party *party, uint8_t island) {
    (void)hero;
    memset(bc, 0, sizeof(BattleContext));

    if (enemy_id < 0 || enemy_id >= ENEMY_COUNT) {
        enemy_id = 0;
    }

    bc->enemy = enemy_table[enemy_id];

    /* Scale enemy exp/gold by island progression */
    {
        int scale_num = 10 + island * 2; /* (1 + island*0.2) * 10 */
        bc->enemy.exp_reward  = (uint16_t)((bc->enemy.exp_reward * scale_num) / 10);
        bc->enemy.gold_reward = (uint16_t)((bc->enemy.gold_reward * scale_num) / 10);
    }

    bc->state = BATTLE_START;
    bc->turn_timer = 0;
    bc->cursor_pos = 0;
    bc->spell_cursor = 0;
    bc->item_cursor = 0;
    bc->player_defending = false;
    bc->damage_display = 0;
    bc->inv = inv;
    bc->party = party;
    bc->boss_turn_count = 0;
    bc->hero_stunned = false;
    bc->island = island;

    snprintf(bc->message, sizeof(bc->message),
             "A %s appeared!", bc->enemy.name);
}

/* ── Update ───────────────────────────────────────────── */
void battle_update(BattleContext *bc, Character *hero) {
    uint16_t pressed = platform_keys_pressed();

    /* Update visual effects each frame */
    fx_update();

    switch (bc->state) {

    /* ── BATTLE_START: show encounter message ──────────── */
    case BATTLE_START:
        bc->turn_timer++;
        if (bc->turn_timer >= 60) {
            bc->turn_timer = 0;
            bc->state = BATTLE_COMMAND;
            bc->cursor_pos = 0;
            bc->player_defending = false;
        }
        break;

    /* ── BATTLE_COMMAND: select action ────────────────── */
    case BATTLE_COMMAND:
        /* If hero is stunned, skip to enemy turn */
        if (bc->hero_stunned) {
            bc->hero_stunned = false;
            snprintf(bc->message, sizeof(bc->message),
                     "%s is stunned!", hero->name);
            bc->state = BATTLE_ENEMY_TURN;
            bc->turn_timer = 0;
            break;
        }
        if (pressed & KEY_UP) {
            if (bc->cursor_pos > 0)
                bc->cursor_pos--;
        }
        if (pressed & KEY_DOWN) {
            if (bc->cursor_pos < CMD_COUNT - 1)
                bc->cursor_pos++;
        }
        if (pressed & KEY_A) {
            bc->selected_command = (BattleCommand)bc->cursor_pos;

            switch (bc->selected_command) {
            case CMD_ATTACK:
                bc->state = BATTLE_EXECUTE;
                bc->turn_timer = 0;
                break;

            case CMD_MAGIC:
                bc->spell_cursor = 0;
                bc->state = BATTLE_MAGIC_SELECT;
                break;

            case CMD_DEFEND:
                bc->player_defending = true;
                snprintf(bc->message, sizeof(bc->message),
                         "%s defends!", hero->name);
                bc->state = BATTLE_ENEMY_TURN;
                bc->turn_timer = 0;
                break;

            case CMD_ITEM:
                if (bc->inv && bc->inv->count > 0) {
                    bc->item_cursor = 0;
                    bc->state = BATTLE_ITEM_SELECT;
                } else {
                    snprintf(bc->message, sizeof(bc->message),
                             "No items!");
                    bc->state = BATTLE_ENEMY_TURN;
                    bc->turn_timer = 0;
                }
                break;

            case CMD_FLEE: {
                /* 50% chance modified by speed comparison */
                int flee_chance = 50 + (hero->spd - bc->enemy.spd) * 5;
                if (flee_chance < 10) flee_chance = 10;
                if (flee_chance > 90) flee_chance = 90;
                int roll = (int)(battle_rand() % 100);
                if (roll < flee_chance) {
                    snprintf(bc->message, sizeof(bc->message),
                             "Got away safely!");
                    bc->state = BATTLE_RESULT;
                    bc->turn_timer = 0;
                } else {
                    snprintf(bc->message, sizeof(bc->message),
                             "Can't escape!");
                    bc->state = BATTLE_ENEMY_TURN;
                    bc->turn_timer = 0;
                }
                break;
            }

            default:
                break;
            }
        }
        break;

    /* ── BATTLE_MAGIC_SELECT: choose a spell ───────────── */
    case BATTLE_MAGIC_SELECT:
        if (pressed & KEY_UP) {
            if (bc->spell_cursor > 0) bc->spell_cursor--;
        }
        if (pressed & KEY_DOWN) {
            if (bc->spell_cursor < MAX_SPELLS - 1) bc->spell_cursor++;
        }
        if (pressed & KEY_B) {
            bc->state = BATTLE_COMMAND;
        }
        if (pressed & KEY_A) {
            const SpellData *sp = &g_spells[bc->spell_cursor];
            if (hero->mp < sp->mp_cost) {
                snprintf(bc->message, sizeof(bc->message),
                         "Not enough MP!");
            } else {
                hero->mp -= sp->mp_cost;
                if (sp->heals) {
                    int heal = sp->power;
                    hero->hp += (int16_t)heal;
                    if (hero->hp > hero->max_hp) hero->hp = hero->max_hp;
                    fx_spawn(FX_HEAL, 40, 120, heal, 24);
                    snprintf(bc->message, sizeof(bc->message),
                             "%s heals %d HP!", sp->name, heal);
                    bc->state = BATTLE_ENEMY_TURN;
                    bc->turn_timer = 0;
                } else {
                    int dmg = sp->power + hero->atk / 2 - bc->enemy.def / 2;
                    int variance = (int)(battle_rand() % 5) - 2;
                    dmg += variance;
                    if (dmg < 1) dmg = 1;
                    bc->enemy.hp -= (int16_t)dmg;
                    bc->damage_display = (int16_t)dmg;
                    fx_spawn(FX_DAMAGE_NUM, 116, 10, dmg, 30);
                    fx_spawn(FX_HIT_FLASH, 0, 0, 0, 6);
                    snprintf(bc->message, sizeof(bc->message),
                             "%s deals %d damage!", sp->name, dmg);
                    bc->state = BATTLE_EXECUTE;
                    bc->turn_timer = 1; /* skip frame 0 damage calc */
                }
            }
        }
        break;

    /* ── BATTLE_ITEM_SELECT: choose an item to use ──────── */
    case BATTLE_ITEM_SELECT:
        if (pressed & KEY_UP) {
            if (bc->item_cursor > 0) bc->item_cursor--;
        }
        if (pressed & KEY_DOWN) {
            if (bc->inv && bc->item_cursor < bc->inv->count - 1)
                bc->item_cursor++;
        }
        if (pressed & KEY_B) {
            bc->state = BATTLE_COMMAND;
        }
        if ((pressed & KEY_A) && bc->inv && bc->item_cursor < bc->inv->count) {
            const ItemData *item = inventory_get_item_data(
                bc->inv->slots[bc->item_cursor].item_id);
            if (item) {
                switch (item->type) {
                case ITEM_POTION:
                case ITEM_HI_POTION: {
                    int heal = item->value;
                    hero->hp += (int16_t)heal;
                    if (hero->hp > hero->max_hp) hero->hp = hero->max_hp;
                    fx_spawn(FX_HEAL, 40, 120, heal, 24);
                    snprintf(bc->message, sizeof(bc->message),
                             "Used %s! +%d HP", item->name, heal);
                    inventory_remove(bc->inv, bc->item_cursor, 1);
                    bc->state = BATTLE_ENEMY_TURN;
                    bc->turn_timer = 0;
                    break;
                }
                case ITEM_ETHER: {
                    int restore = item->value;
                    hero->mp += (int16_t)restore;
                    if (hero->mp > hero->max_mp) hero->mp = hero->max_mp;
                    snprintf(bc->message, sizeof(bc->message),
                             "Used %s! +%d MP", item->name, restore);
                    inventory_remove(bc->inv, bc->item_cursor, 1);
                    bc->state = BATTLE_ENEMY_TURN;
                    bc->turn_timer = 0;
                    break;
                }
                case ITEM_BOMB: {
                    int dmg = item->value;
                    bc->enemy.hp -= (int16_t)dmg;
                    bc->damage_display = (int16_t)dmg;
                    fx_spawn(FX_DAMAGE_NUM, 116, 10, dmg, 30);
                    fx_spawn(FX_HIT_FLASH, 0, 0, 0, 6);
                    snprintf(bc->message, sizeof(bc->message),
                             "Bomb deals %d damage!", dmg);
                    inventory_remove(bc->inv, bc->item_cursor, 1);
                    bc->state = BATTLE_EXECUTE;
                    bc->turn_timer = 1; /* skip frame 0 */
                    break;
                }
                default:
                    snprintf(bc->message, sizeof(bc->message),
                             "Can't use that here!");
                    break;
                }
            }
            if (bc->inv->count == 0 || bc->item_cursor >= bc->inv->count) {
                if (bc->item_cursor > 0) bc->item_cursor--;
            }
        }
        break;

    /* ── BATTLE_EXECUTE: player attacks enemy ─────────── */
    case BATTLE_EXECUTE:
        if (bc->turn_timer == 0) {
            int dmg = battle_calc_damage(hero->atk, bc->enemy.def);

            /* Critical hit: 5% base + LUK/256 */
            {
                int crit_chance = 5 + (hero->luk * 100) / 256;
                int crit_roll = (int)(battle_rand() % 100);
                if (crit_roll < crit_chance) {
                    dmg = (dmg * 3) / 2; /* 1.5x damage */
                    if (dmg < 2) dmg = 2;
                    snprintf(bc->message, sizeof(bc->message),
                             "CRITICAL! %s dealt %d!", hero->name, dmg);
                } else {
                    snprintf(bc->message, sizeof(bc->message),
                             "%s dealt %d damage!", hero->name, dmg);
                }
            }

            bc->enemy.hp -= (int16_t)dmg;
            bc->damage_display = (int16_t)dmg;

            /* Spawn visual effects: damage number at enemy + hit flash */
            fx_spawn(FX_DAMAGE_NUM, 116, 10, dmg, 30);
            fx_spawn(FX_HIT_FLASH, 0, 0, 0, 6);

            if (bc->enemy.hp <= 0) {
                bc->enemy.hp = 0;
            }
        }
        bc->turn_timer++;
        if (bc->turn_timer >= 30) {
            bc->turn_timer = 0;
            if (bc->enemy.hp <= 0) {
                bc->state = BATTLE_WIN;
                bc->turn_timer = 0;
            } else {
                bc->state = BATTLE_ENEMY_TURN;
            }
        }
        break;

    /* ── BATTLE_ENEMY_TURN: enemy attacks player ──────── */
    case BATTLE_ENEMY_TURN:
        if (bc->turn_timer == 0) {
            bc->boss_turn_count++;

            /* Enemy flee check: enemies with <20% HP have 30% chance to flee */
            /* (bosses don't flee — enemy ids 7-9) */
            {
                bool is_boss = (bc->enemy.sprite_id >= 23 && bc->enemy.sprite_id <= 25);
                int hp_pct = (bc->enemy.max_hp > 0)
                    ? (bc->enemy.hp * 100) / bc->enemy.max_hp : 100;
                if (!is_boss && hp_pct < 20) {
                    int flee_roll = (int)(battle_rand() % 100);
                    if (flee_roll < 30) {
                        snprintf(bc->message, sizeof(bc->message),
                                 "%s fled!", bc->enemy.name);
                        bc->enemy.hp = 0;
                        /* No rewards for fled enemy */
                        bc->enemy.exp_reward = 0;
                        bc->enemy.gold_reward = 0;
                        bc->turn_timer = 1; /* skip damage */
                        /* Fall through to win check below */
                    }
                }
            }

            if (bc->turn_timer == 0) {
                int dmg = battle_calc_damage(bc->enemy.atk, hero->def);

                /* Boss AI special attacks */
                bool is_fire_dragon = (bc->enemy.sprite_id == 23);
                bool is_kraken = (bc->enemy.sprite_id == 24);
                bool is_sky_lord = (bc->enemy.sprite_id == 25);

                if (is_fire_dragon && (bc->boss_turn_count % 3 == 0)) {
                    /* Fire Breath: 2x damage every 3rd turn */
                    dmg = battle_calc_damage(bc->enemy.atk * 2, hero->def);
                    snprintf(bc->message, sizeof(bc->message),
                             "Fire Breath! %d damage!", dmg);
                } else if (is_kraken && (bc->boss_turn_count % 4 == 0)) {
                    /* Tentacle Grab: stun hero for 1 turn + normal damage */
                    bc->hero_stunned = true;
                    snprintf(bc->message, sizeof(bc->message),
                             "Tentacle Grab! %d dmg! Stunned!", dmg);
                } else if (is_sky_lord && (bc->boss_turn_count % 2 == 0)) {
                    /* Thunder Storm: hits all party (deal damage to hero + party) */
                    dmg = battle_calc_damage(bc->enemy.atk, hero->def / 2);
                    /* Damage party members too */
                    if (bc->party) {
                        for (int pi = 1; pi < bc->party->count; pi++) {
                            Character *m = &bc->party->members[pi];
                            int pdmg = battle_calc_damage(bc->enemy.atk, m->def / 2);
                            if (bc->player_defending) pdmg /= 2;
                            if (pdmg < 1) pdmg = 1;
                            m->hp -= (int16_t)pdmg;
                            if (m->hp < 0) m->hp = 0;
                        }
                    }
                    snprintf(bc->message, sizeof(bc->message),
                             "Thunder Storm! %d to all!", dmg);
                } else {
                    snprintf(bc->message, sizeof(bc->message),
                             "%s attacks for %d!", bc->enemy.name, dmg);
                }

                if (bc->player_defending) {
                    dmg /= 2;
                    if (dmg < 1) dmg = 1;
                }
                hero->hp -= (int16_t)dmg;
                bc->damage_display = (int16_t)dmg;

                /* Spawn damage number at hero stats area */
                fx_spawn(FX_DAMAGE_NUM, 40, 100, dmg, 30);

                if (hero->hp <= 0) {
                    hero->hp = 0;
                }
            }
        }
        bc->turn_timer++;
        if (bc->turn_timer >= 30) {
            bc->turn_timer = 0;
            if (bc->enemy.hp <= 0) {
                bc->state = BATTLE_WIN;
            } else if (hero->hp <= 0) {
                bc->state = BATTLE_LOSE;
            } else {
                bc->state = BATTLE_COMMAND;
                bc->cursor_pos = 0;
                bc->player_defending = false;
            }
        }
        break;

    /* ── BATTLE_WIN: show rewards ─────────────────────── */
    case BATTLE_WIN:
        if (bc->turn_timer == 0) {
            snprintf(bc->message, sizeof(bc->message),
                     "%s defeated! +%dEXP +%dG",
                     bc->enemy.name,
                     bc->enemy.exp_reward,
                     bc->enemy.gold_reward);
        }
        bc->turn_timer++;
        if (bc->turn_timer >= 90) {
            /* Rewards applied externally via game.c */
            bc->state = BATTLE_RESULT;
            bc->turn_timer = 0;
        }
        break;

    /* ── BATTLE_LOSE: game over ───────────────────────── */
    case BATTLE_LOSE:
        if (bc->turn_timer == 0) {
            snprintf(bc->message, sizeof(bc->message),
                     "%s has fallen...", hero->name);
        }
        bc->turn_timer++;
        if (bc->turn_timer >= 90) {
            bc->state = BATTLE_RESULT;
            bc->turn_timer = 0;
        }
        break;

    /* ── BATTLE_RESULT: wait for transition ───────────── */
    case BATTLE_RESULT:
        /* Handled by game.c — transitions back to world or game over */
        break;
    }
}

/* ── Island-appropriate Enemy Selection ────────────────── */
int battle_get_random_enemy_for_island(int island_id, uint32_t rng) {
    switch (island_id) {
        case 0:
        case 1:
            /* Slime (0) or Goblin (1) */
            return (int)(rng % 2);
        case 2:
        case 3:
            /* Scorpion (2) or Fire Bat (3) */
            return 2 + (int)(rng % 2);
        case 4:
        case 5:
            /* Ice Golem (4) or Wraith (5) */
            return 4 + (int)(rng % 2);
        case 6:
            /* Temple Guard (6) */
            return 6;
        default:
            return 0;
    }
}

/* ── Render ────────────────────────────────────────────── */
void battle_render(BattleContext *bc, Character *hero) {
    platform_clear(0x1084);  /* dark blue-gray background */

    /* ── Enemy sprite (top center) ────────────────────── */
    platform_draw_sprite(112, 20, bc->enemy.sprite_id, 0, false);

    /* Enemy HP bar background */
    platform_draw_rect(80, 48, 80, 10, 0x0000);
    /* Enemy HP bar fill */
    if (bc->enemy.max_hp > 0) {
        int bar_w = (bc->enemy.hp * 76) / bc->enemy.max_hp;
        if (bar_w < 0) bar_w = 0;
        platform_draw_rect(82, 50, bar_w, 6, 0x03E0); /* green */
    }

    {
        char ehp[24];
        snprintf(ehp, sizeof(ehp), "%s HP:%d", bc->enemy.name, bc->enemy.hp);
        platform_draw_text(80, 60, ehp, 0x7FFF);
    }

    /* ── Hero stats (bottom left) — show all party members ── */
    platform_draw_rect(0, 76, 120, 84, 0x0000);
    {
        char line1[32], line2[32];
        /* Show hero (primary fighter) with full info */
        snprintf(line1, sizeof(line1), "%s Lv%d", hero->name, hero->level);
        snprintf(line2, sizeof(line2), "HP:%d/%d MP:%d/%d", hero->hp, hero->max_hp, hero->mp, hero->max_mp);
        platform_draw_text(4, 78, line1, 0x7FFF);
        platform_draw_text(4, 88, line2, 0x7FFF);

        /* Show other party members' HP (compact) */
        if (bc->party) {
        for (int pi = 1; pi < bc->party->count && pi < MAX_PARTY_SIZE; pi++) {
            const Character *m = &bc->party->members[pi];
            int y = 98 + (pi - 1) * 12;
            snprintf(line1, sizeof(line1), "%s HP:%d/%d", m->name, m->hp, m->max_hp);
            platform_draw_text(4, y, line1, 0x5EF7);
        }
        } /* end if bc->party */
    }

    /* ── Command menu (bottom right) ──────────────────── */
    if (bc->state == BATTLE_COMMAND) {
        platform_draw_rect(120, 110, 120, 50, 0x0000);
        for (int i = 0; i < CMD_COUNT; i++) {
            uint16_t color = (i == bc->cursor_pos) ? 0x03FF : 0x5294;
            char item[20];
            snprintf(item, sizeof(item), "%s%s",
                     (i == bc->cursor_pos) ? "> " : "  ",
                     cmd_names[i]);
            platform_draw_text(124, 112 + i * 10, item, color);
        }
    }

    /* ── Magic selection menu ────────────────────────── */
    if (bc->state == BATTLE_MAGIC_SELECT) {
        platform_draw_rect(120, 110, 120, 50, 0x0000);
        for (int i = 0; i < MAX_SPELLS; i++) {
            uint16_t color = (i == bc->spell_cursor) ? 0x03FF : 0x5294;
            char line[24];
            snprintf(line, sizeof(line), "%s%s %dMP",
                     (i == bc->spell_cursor) ? ">" : " ",
                     g_spells[i].name, g_spells[i].mp_cost);
            platform_draw_text(124, 112 + i * 10, line, color);
        }
    }

    /* ── Item selection menu ─────────────────────────── */
    if (bc->state == BATTLE_ITEM_SELECT && bc->inv) {
        platform_draw_rect(120, 110, 120, 50, 0x0000);
        int visible = 5;
        int offset = 0;
        if (bc->item_cursor >= visible) offset = bc->item_cursor - visible + 1;
        for (int i = 0; i < visible && (i + offset) < bc->inv->count; i++) {
            int idx = i + offset;
            const ItemData *itm = inventory_get_item_data(bc->inv->slots[idx].item_id);
            if (!itm) continue;
            uint16_t color = (idx == bc->item_cursor) ? 0x03FF : 0x5294;
            char line[24];
            snprintf(line, sizeof(line), "%s%s x%d",
                     (idx == bc->item_cursor) ? ">" : " ",
                     itm->name, bc->inv->slots[idx].count);
            platform_draw_text(124, 112 + i * 10, line, color);
        }
    }

    /* ── Damage display ───────────────────────────────── */
    if ((bc->state == BATTLE_EXECUTE || bc->state == BATTLE_ENEMY_TURN)
        && bc->turn_timer < 30) {
        char dmg_text[12];
        snprintf(dmg_text, sizeof(dmg_text), "%d", bc->damage_display);
        if (bc->state == BATTLE_EXECUTE) {
            /* Damage on enemy */
            platform_draw_text(116, 10, dmg_text, 0x001F); /* red */
        } else {
            /* Damage on hero */
            platform_draw_text(40, 100, dmg_text, 0x001F); /* red */
        }
    }

    /* ── Message box ──────────────────────────────────── */
    platform_draw_rect(0, 0, 240, 12, 0x0000);
    platform_draw_text(4, 2, bc->message, 0x7FFF);

    /* ── Visual effects overlay ──────────────────────── */
    fx_render();
}
