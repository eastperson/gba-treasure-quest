/**
 * game.c — Core game loop and state machine
 * Platform-independent: no GBA or SDL headers here.
 */
#include "game.h"
#include "battle.h"
#include "sprite.h"
#include "effects.h"
#include "party.h"
#include "inventory.h"
#include "map.h"
#include "npc.h"
#include "dialogue.h"
#include "save.h"
#include "menu.h"
#include "platform.h"
#include "ui.h"
#include "audio.h"
#include "transition.h"
#include "treasure.h"
#include <string.h>
#include <stdio.h>

/* ── Global Map Data ──────────────────────────────────── */
static MapData g_map;

/* ── Battle Context ───────────────────────────────────── */
static BattleContext g_battle;

/* ── Player Sprite Animation ─────────────────────────── */
static SpriteAnim g_player_anim;

/* ── Initialization ────────────────────────────────────── */
void game_init(GameContext *ctx) {
    memset(ctx, 0, sizeof(GameContext));
    ctx->state   = STATE_TITLE;
    ctx->running = true;

    /* Default starting character */
    Character *hero = &ctx->party.members[0];
    strncpy(hero->name, "Hero", MAX_NAME_LEN);
    hero->hp      = 30;
    hero->max_hp  = 30;
    hero->mp      = 10;
    hero->max_mp  = 10;
    hero->atk     = 8;
    hero->def     = 5;
    hero->spd     = 6;
    hero->luk     = 4;
    hero->level   = 1;
    hero->exp     = 0;
    hero->sprite_id = 0;
    hero->weapon_id = -1;
    hero->armor_id  = -1;
    ctx->party.count = 1;
    ctx->party.gold  = 100;

    /* Starting position — first island, town center */
    ctx->pos.x     = 7;
    ctx->pos.y     = 5;
    ctx->pos.map_id    = 0;
    ctx->pos.direction = 0;

    ctx->current_island  = 0;
    ctx->treasures_found = 0;
    ctx->inv_cursor      = 0;

    /* Initialize inventory with starting items */
    inventory_init(&ctx->inventory);
    inventory_add(&ctx->inventory, 0, 2);  /* 2x Potion */
    inventory_add(&ctx->inventory, 3, 1);  /* 1x Antidote */

    /* Initialize save system */
    save_init();

    /* Initialize menu */
    ctx->menu.cursor = 0;
    ctx->menu.open   = false;
    ctx->save_cursor = 0;

    /* Initialize treasure collection system */
    treasure_init();

    /* Initialize dialogue system */
    dialogue_init();

    /* Initialize sprite animation and effects */
    sprite_anim_init(&g_player_anim);
    fx_init();

    /* Load starting map and NPCs */
    map_load(&g_map, 0);
    npc_init_map(0);

    /* Start title screen music */
    audio_play_bgm(BGM_TITLE);
}

/* ── Main Update ───────────────────────────────────────── */
void game_update(GameContext *ctx) {
    platform_poll_input();
    ctx->frame_count++;

    /* Handle active transition — skip normal input during transition */
    if (transition_update(ctx)) {
        return;
    }

    switch (ctx->state) {
        case STATE_TITLE:     state_title_update(ctx);  break;
        case STATE_WORLD:     state_world_update(ctx);  break;
        case STATE_BATTLE:    state_battle_update(ctx);  break;
        case STATE_DIALOGUE:
            if (!dialogue_update()) {
                ctx->state = STATE_WORLD;
            }
            break;
        case STATE_MENU:
            menu_update(&ctx->menu, ctx);
            break;
        case STATE_INVENTORY: state_inventory_update(ctx); break;
        case STATE_SAVE:
            state_save_update(ctx);
            break;
        case STATE_SHOP:
            state_shop_update(ctx);
            break;
        case STATE_GAME_OVER:
            state_game_over_update(ctx);
            break;
    }
}

/* ── Main Render───────────────────────────────────────── */
void game_render(GameContext *ctx) {
    platform_frame_start();

    switch (ctx->state) {
        case STATE_TITLE:     state_title_render(ctx);  break;
        case STATE_WORLD:     state_world_render(ctx);  break;
        case STATE_BATTLE:    state_battle_render(ctx);  break;
        case STATE_DIALOGUE:
            /* Render world underneath the dialogue box */
            state_world_render(ctx);
            dialogue_render();
            break;
        case STATE_MENU:
            menu_render(&ctx->menu, ctx);
            break;
        case STATE_INVENTORY: state_inventory_render(ctx); break;
        case STATE_SAVE:
            state_save_render(ctx);
            break;
        case STATE_SHOP:
            state_shop_render(ctx);
            break;
        case STATE_GAME_OVER:
            state_game_over_render(ctx);
            break;
    }

    /* Draw transition overlay on top of everything */
    transition_render();

    platform_frame_end();
}

/* ── Title Screen ──────────────────────────────────────── */
void state_title_update(GameContext *ctx) {
    uint16_t keys = platform_keys_pressed();
    if (keys & KEY_START) {
        audio_play_sfx(SFX_CONFIRM);
        audio_set_bgm_for_island(ctx->current_island);
        transition_start(TRANS_FADE_OUT, 20, STATE_WORLD);
    }
}

void state_title_render(GameContext *ctx) {
    platform_clear(0x1084); /* dark blue-gray */

    /* Decorative border */
    platform_draw_rect(10, 10, 220, 140, 0x0842);
    platform_draw_rect(12, 12, 216, 136, 0x0000);

    /* Logo box */
    platform_draw_rect(30, 20, 180, 40, 0x0421);
    platform_draw_rect(32, 22, 176, 36, 0x0000);

    /* Title text */
    platform_draw_text(40, 28, "TREASURE  QUEST", 0x03FF); /* yellow */
    platform_draw_text(48, 44, "Seven Islands", 0x5EF7);   /* light gray */

    /* Decorative treasure icons */
    for (int i = 0; i < 7; i++) {
        int ix = 54 + i * 20;
        uint16_t ic = (uint16_t)(0x001F + i * 0x0400); /* varying colors */
        platform_draw_rect(ix, 68, 8, 8, ic);
        platform_draw_rect(ix + 1, 69, 6, 6, 0x03FF);
    }

    /* Menu options */
    platform_draw_text(76, 90, "New Adventure", 0x7FFF);
    platform_draw_text(68, 106, "- PRESS START -", 0x5294);

    /* Blink "PRESS START" */
    if ((ctx->frame_count / 30) % 2 == 0) {
        platform_draw_text(68, 106, "- PRESS START -", 0x7FFF);
    }

    /* Credits */
    platform_draw_text(56, 132, "Sprint 18 Edition", 0x294A);
}

/* ── World / Overworld ─────────────────────────────────── */
void state_world_update(GameContext *ctx) {
    uint16_t keys = platform_keys_held();
    int16_t new_x = ctx->pos.x;
    int16_t new_y = ctx->pos.y;

    if (keys & KEY_UP)    { new_y--; ctx->pos.direction = 1; }
    if (keys & KEY_DOWN)  { new_y++; ctx->pos.direction = 0; }
    if (keys & KEY_LEFT)  { new_x--; ctx->pos.direction = 2; }
    if (keys & KEY_RIGHT) { new_x++; ctx->pos.direction = 3; }

    /* Collision check: map tiles + NPC positions */
    if (map_is_walkable(&g_map, new_x, new_y) && !npc_is_at(new_x, new_y)) {
        ctx->pos.x = new_x;
        ctx->pos.y = new_y;
    }

    /* Update sprite animation direction and play/stop state */
    {
        bool moving = (new_x != ctx->pos.x || new_y != ctx->pos.y)
                      || (keys & (KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT));
        sprite_anim_set_dir(&g_player_anim, ctx->pos.direction);
        if (moving) {
            sprite_anim_play(&g_player_anim);
        } else {
            sprite_anim_stop(&g_player_anim);
        }
        sprite_anim_update(&g_player_anim);
    }

    /* NPC update (wander behavior) */
    npc_update(ctx->frame_count);

    /* NPC interaction on A button */
    uint16_t pressed = platform_keys_pressed();
    if (pressed & KEY_A) {
        NPCData *npc = npc_check_interaction(ctx->pos.x, ctx->pos.y, ctx->pos.direction);
        if (npc) {
            if (npc->type == NPC_SHOPKEEPER) {
                ctx->shop_cursor = 0;
                ctx->state = STATE_SHOP;
            } else {
                dialogue_start(npc->dialogue_id);
                ctx->state = STATE_DIALOGUE;
            }
        }
    }

    /* Event trigger check — fires when player steps on trigger tile */
    {
        EventTrigger *trig = map_check_trigger(&g_map, ctx->pos.x, ctx->pos.y);
        if (trig) {
            switch (trig->type) {
                case EVENT_CHEST:
                    /* Check if this is a treasure chest (item_id 6-12) */
                    if (trig->id >= 6 && trig->id <= 12) {
                        int island = trig->id - 6;
                        treasure_collect(island, ctx);
                    } else {
                        /* Regular item chest */
                        inventory_add(&ctx->inventory, trig->id, 1);
                    }
                    trig->triggered = true;
                    audio_play_sfx(SFX_CHEST);
                    /* Change tile to grass so chest disappears */
                    if (ctx->pos.x >= 0 && ctx->pos.x < g_map.width &&
                        ctx->pos.y >= 0 && ctx->pos.y < g_map.height) {
                        g_map.tiles[ctx->pos.y][ctx->pos.x] = TILE_GRASS;
                    }
                    /* Check if all treasures collected */
                    if (treasure_check_complete(ctx)) {
                        ctx->victory = true;
                        audio_play_bgm(BGM_VICTORY);
                        transition_start(TRANS_FADE_OUT, 30, STATE_GAME_OVER);
                    }
                    break;

                case EVENT_PORT: {
                    /* Warp to another island */
                    uint8_t dest_map = trig->id;
                    audio_play_sfx(SFX_DOOR);

                    /* Party recruitment events when leaving certain islands */
                    /* Island 1 (Dark Forest) -> recruit Elara (Mage) */
                    if (ctx->current_island == 1 && dest_map != 1 && ctx->party.count < MAX_PARTY_SIZE) {
                        /* Check if Elara not already in party */
                        bool found = false;
                        for (int pi = 0; pi < ctx->party.count; pi++) {
                            if (ctx->party.members[pi].name[0] == 'E' &&
                                ctx->party.members[pi].name[1] == 'l') {
                                found = true; break;
                            }
                        }
                        if (!found) {
                            party_add_member(&ctx->party, "Elara",
                                25, 30, 5, 4, 7, 1);
                            ctx->party.members[ctx->party.count - 1].weapon_id = -1;
                            ctx->party.members[ctx->party.count - 1].armor_id = -1;
                        }
                    }
                    /* Island 3 (Volcanic Coast) -> recruit Drake (Fighter) */
                    if (ctx->current_island == 3 && dest_map != 3 && ctx->party.count < MAX_PARTY_SIZE) {
                        bool found = false;
                        for (int pi = 0; pi < ctx->party.count; pi++) {
                            if (ctx->party.members[pi].name[0] == 'D' &&
                                ctx->party.members[pi].name[1] == 'r') {
                                found = true; break;
                            }
                        }
                        if (!found) {
                            party_add_member(&ctx->party, "Drake",
                                45, 8, 14, 8, 5, 2);
                            ctx->party.members[ctx->party.count - 1].weapon_id = -1;
                            ctx->party.members[ctx->party.count - 1].armor_id = -1;
                        }
                    }
                    /* Island 5 (Sunken Ruins) -> recruit Naia (Healer) */
                    if (ctx->current_island == 5 && dest_map != 5 && ctx->party.count < MAX_PARTY_SIZE) {
                        bool found = false;
                        for (int pi = 0; pi < ctx->party.count; pi++) {
                            if (ctx->party.members[pi].name[0] == 'N' &&
                                ctx->party.members[pi].name[1] == 'a') {
                                found = true; break;
                            }
                        }
                        if (!found) {
                            party_add_member(&ctx->party, "Naia",
                                28, 35, 4, 5, 6, 3);
                            ctx->party.members[ctx->party.count - 1].weapon_id = -1;
                            ctx->party.members[ctx->party.count - 1].armor_id = -1;
                        }
                    }

                    map_load(&g_map, dest_map);
                    npc_init_map(dest_map);
                    ctx->pos.x = g_map.spawn_x;
                    ctx->pos.y = g_map.spawn_y;
                    ctx->pos.map_id = dest_map;
                    ctx->current_island = dest_map;
                    audio_set_bgm_for_island(dest_map);
                    transition_start(TRANS_FADE_OUT, 15, STATE_WORLD);
                    break;
                }

                case EVENT_SIGN:
                    dialogue_start(trig->id);
                    ctx->state = STATE_DIALOGUE;
                    /* Signs can be triggered repeatedly */
                    break;

                case EVENT_BOSS: {
                    /* Boss IDs: 0=Fire Dragon(7), 1=Kraken(8), 2=Sky Lord(9) */
                    int boss_enemy_id = 7 + trig->id;
                    battle_init(&g_battle, &ctx->party.members[0],
                                boss_enemy_id, &ctx->inventory,
                                &ctx->party, ctx->current_island);
                    audio_play_bgm(BGM_BATTLE);
                    transition_start(TRANS_FADE_OUT, 15, STATE_BATTLE);
                    trig->triggered = true;
                    break;
                }
            }
        }
    }

    /* Random encounter check — 5% chance per step */
    {
        /* Check if player moved this frame */
        static int16_t prev_x = -1, prev_y = -1;
        if ((ctx->pos.x != prev_x || ctx->pos.y != prev_y)
            && prev_x >= 0) {
            uint32_t rng = (ctx->frame_count * 1103515245u + 12345u) & 0x7FFFFFFFu;
            if ((rng % 100) < 5) {
                /* Pick island-appropriate enemy */
                int eid = battle_get_random_enemy_for_island(
                              ctx->current_island, rng >> 8);
                battle_init(&g_battle, &ctx->party.members[0], eid, &ctx->inventory,
                            &ctx->party, ctx->current_island);
                audio_play_bgm(BGM_BATTLE);
                transition_start(TRANS_FADE_OUT, 15, STATE_BATTLE);
            }
        }
        prev_x = ctx->pos.x;
        prev_y = ctx->pos.y;
    }

    /* Menu */
    if (pressed & KEY_START) {
        ctx->menu.cursor = 0;
        ctx->menu.open   = true;
        ctx->state = STATE_MENU;
    }
}

void state_world_render(GameContext *ctx) {
    platform_clear(0x0000);

    /* Camera follows player, centered on screen */
    int16_t cam_x = ctx->pos.x * 8 - SCREEN_W / 2 + 4;
    int16_t cam_y = ctx->pos.y * 8 - SCREEN_H / 2 + 4;

    /* Clamp camera to map bounds */
    int16_t max_cam_x = g_map.width * 8 - SCREEN_W;
    int16_t max_cam_y = g_map.height * 8 - SCREEN_H;
    if (cam_x < 0) cam_x = 0;
    if (cam_y < 0) cam_y = 0;
    if (cam_x > max_cam_x) cam_x = max_cam_x;
    if (cam_y > max_cam_y) cam_y = max_cam_y;

    /* Draw tilemap */
    map_draw(&g_map, cam_x, cam_y);

    /* Draw NPCs */
    npc_draw(cam_x, cam_y);

    /* Draw player sprite (using animation system) */
    platform_draw_sprite(
        ctx->pos.x * 8 - cam_x,
        ctx->pos.y * 8 - cam_y,
        sprite_anim_get_frame(&g_player_anim),
        0,
        ctx->pos.direction == 2 /* flip if facing left */
    );

    /* ── HUD Bar (top of screen) ─────────────────────── */
    {
        /* Semi-transparent dark bar */
        platform_draw_rect(0, 0, 174, 10, 0x0000);

        Character *h = &ctx->party.members[0];
        char hud_line[64];
        snprintf(hud_line, sizeof(hud_line), "HP:%d/%d MP:%d/%d G:%d %s",
                 h->hp, h->max_hp, h->mp, h->max_mp,
                 ctx->party.gold, g_map.name);
        platform_draw_text(2, 1, hud_line, 0x7FFF);
    }

    /* ── Minimap (top-right corner, 60x40 pixels) ──── */
    {
        int mm_x = SCREEN_W - 62;
        int mm_y = 2;
        int mm_w = 60;
        int mm_h = 40;

        /* Background border */
        platform_draw_rect(mm_x - 1, mm_y - 1, mm_w + 2, mm_h + 2, 0x0000);

        /* Draw minimap: each tile = 2x2 pixels (max 30x20 = 60x40) */
        /* We scale to fit: tile_pw = mm_w / map_w, tile_ph = mm_h / map_h */
        /* For simplicity, use 2x2 per tile (30*2=60, 20*2=40 fits perfectly) */
        for (int my = 0; my < g_map.height && my * 2 < mm_h; my++) {
            for (int mx = 0; mx < g_map.width && mx * 2 < mm_w; mx++) {
                TileType t = (TileType)g_map.tiles[my][mx];
                uint16_t mc;
                switch (t) {
                    case TILE_GRASS: mc = 0x2108; break;
                    case TILE_WATER: mc = 0x7C00; break;
                    case TILE_SAND:  mc = 0x2D7F; break;
                    case TILE_STONE: mc = 0x4A52; break;
                    case TILE_TREE:  mc = 0x01C0; break;
                    case TILE_WALL:  mc = 0x294A; break;
                    case TILE_DOOR:  mc = 0x015F; break;
                    case TILE_CHEST: mc = 0x02BF; break;
                    case TILE_PORT:  mc = 0x4210; break;
                    default:         mc = 0x0000; break;
                }
                platform_draw_rect(mm_x + mx * 2, mm_y + my * 2, 2, 2, mc);
            }
        }

        /* Player dot (blinking) */
        if ((ctx->frame_count / 15) % 2 == 0) {
            int px = mm_x + ctx->pos.x * 2;
            int py = mm_y + ctx->pos.y * 2;
            platform_draw_rect(px, py, 2, 2, 0x7FFF); /* white blink */
        } else {
            int px = mm_x + ctx->pos.x * 2;
            int py = mm_y + ctx->pos.y * 2;
            platform_draw_rect(px, py, 2, 2, 0x001F); /* red blink */
        }
    }

    /* ── Treasure Counter (bottom of screen) ─────────── */
    {
        char tc[24];
        snprintf(tc, sizeof(tc), "Treasures: %d/%d",
                 treasure_get_count(ctx), TOTAL_TREASURES);
        platform_draw_rect(0, 150, 120, 10, 0x0000);
        platform_draw_text(2, 151, tc, 0x03FF);
    }

    /* Treasure collection icons */
    treasure_render_status(ctx);
}

/* ── Battle ────────────────────────────────────────────── */
void state_battle_update(GameContext *ctx) {
    Character *hero = &ctx->party.members[0];
    battle_update(&g_battle, hero);

    /* Check if battle ended */
    if (g_battle.state == BATTLE_RESULT) {
        /* Check what the outcome was by inspecting enemy/hero HP */
        if (hero->hp <= 0) {
            ctx->victory = false;
            audio_play_bgm(BGM_GAMEOVER);
            transition_start(TRANS_FADE_OUT, 30, STATE_GAME_OVER);
        } else {
            /* Apply rewards if enemy was defeated */
            if (g_battle.enemy.hp <= 0) {
                hero->exp += g_battle.enemy.exp_reward;
                ctx->party.gold += g_battle.enemy.gold_reward;

                /* Level up using growth system */
                while (party_try_level_up(hero)) {
                    audio_play_sfx(SFX_LEVELUP);
                }
            }
            /* Play victory jingle then restore island BGM */
            audio_play_bgm(BGM_VICTORY);
            audio_set_bgm_for_island(ctx->current_island);
            transition_start(TRANS_FADE_OUT, 20, STATE_WORLD);
        }
    }
}

void state_battle_render(GameContext *ctx) {
    Character *hero = &ctx->party.members[0];
    battle_render(&g_battle, hero);
}

/* ── Inventory Screen ─────────────────────────────────── */
void state_inventory_update(GameContext *ctx) {
    uint16_t keys = platform_keys_pressed();

    if (keys & KEY_UP) {
        if (ctx->inv_cursor > 0) ctx->inv_cursor--;
    }
    if (keys & KEY_DOWN) {
        if (ctx->inventory.count > 0 &&
            ctx->inv_cursor < ctx->inventory.count - 1) {
            ctx->inv_cursor++;
        }
    }
    if (keys & KEY_A) {
        /* Use item on first party member */
        if (ctx->inventory.count > 0) {
            inventory_use(&ctx->inventory, ctx->inv_cursor,
                          &ctx->party.members[0]);
            /* Adjust cursor if it's now past the end */
            if (ctx->inv_cursor >= ctx->inventory.count && ctx->inv_cursor > 0) {
                ctx->inv_cursor--;
            }
        }
    }
    if (keys & KEY_B) {
        ctx->state = STATE_WORLD;
    }
}

void state_inventory_render(GameContext *ctx) {
    platform_clear(0x0000);
    inventory_render(&ctx->inventory, ctx->inv_cursor);
}

/* ── Save Screen ──────────────────────────────────────── */
static int g_save_msg_timer = 0;  /* frames remaining for "Saved!" message */

void state_save_update(GameContext *ctx) {
    /* While showing "Saved!" message, wait for timer to expire */
    if (g_save_msg_timer > 0) {
        g_save_msg_timer--;
        if (g_save_msg_timer == 0) {
            ctx->state = STATE_WORLD;
        }
        return;
    }

    uint16_t keys = platform_keys_pressed();

    if (keys & KEY_UP) {
        if (ctx->save_cursor > 0) ctx->save_cursor--;
    }
    if (keys & KEY_DOWN) {
        if (ctx->save_cursor < MAX_SAVE_SLOTS - 1) ctx->save_cursor++;
    }
    if (keys & KEY_A) {
        if (save_write(ctx->save_cursor, ctx)) {
            g_save_msg_timer = 90; /* show "Saved!" for ~1.5 seconds at 60fps */
        }
    }
    if (keys & KEY_B) {
        ctx->state = STATE_WORLD;
    }
}

void state_save_render(GameContext *ctx) {
    platform_clear(0x0000);
    save_render_slots(ctx->save_cursor);

    /* Show "Saved!" overlay when timer is active */
    if (g_save_msg_timer > 0) {
        platform_draw_rect(80, 70, 80, 20, 0x0000);
        platform_draw_text(92, 74, "Saved!", 0x1BE0);
    }

    /* Footer hint */
    if (g_save_msg_timer == 0) {
        platform_draw_text(48, 146, "[A] Save  [B] Cancel", 0x5294);
    }
}

/* ── Shop Screen ──────────────────────────────────────── */
/* Island 0-1: basic consumables */
static const uint8_t shop_items_basic[] = { 0, 1, 2, 3, 4, 5 };
#define SHOP_BASIC_COUNT 6
/* Island 2-3: add Short Sword, Leather Armor */
static const uint8_t shop_items_mid[] = { 0, 1, 2, 4, 13, 16 };
#define SHOP_MID_COUNT 6
/* Island 4-5: add Iron Blade, Chain Mail */
static const uint8_t shop_items_adv[] = { 1, 2, 4, 14, 17, 15 };
#define SHOP_ADV_COUNT 6
/* Island 6: endgame gear */
static const uint8_t shop_items_end[] = { 1, 2, 4, 15, 18, 5 };
#define SHOP_END_COUNT 6

static const uint8_t *get_shop_items(int island, int *count) {
    if (island <= 1) { *count = SHOP_BASIC_COUNT; return shop_items_basic; }
    if (island <= 3) { *count = SHOP_MID_COUNT;   return shop_items_mid;   }
    if (island <= 5) { *count = SHOP_ADV_COUNT;   return shop_items_adv;   }
    *count = SHOP_END_COUNT; return shop_items_end;
}

void state_shop_update(GameContext *ctx) {
    uint16_t keys = platform_keys_pressed();
    int shop_count = 0;
    const uint8_t *shop_ids = get_shop_items(ctx->current_island, &shop_count);

    if (keys & KEY_UP) {
        if (ctx->shop_cursor > 0) ctx->shop_cursor--;
    }
    if (keys & KEY_DOWN) {
        if (ctx->shop_cursor < shop_count - 1) ctx->shop_cursor++;
    }
    if (keys & KEY_A) {
        const ItemData *item = inventory_get_item_data(shop_ids[ctx->shop_cursor]);
        if (item && ctx->party.gold >= (int16_t)item->price) {
            ctx->party.gold -= (int16_t)item->price;
            inventory_add(&ctx->inventory, item->id, 1);
            audio_play_sfx(SFX_CONFIRM);
        }
    }
    if (keys & KEY_B) {
        ctx->state = STATE_WORLD;
    }
}

void state_shop_render(GameContext *ctx) {
    int shop_count = 0;
    const uint8_t *shop_ids = get_shop_items(ctx->current_island, &shop_count);

    platform_clear(0x0000);
    platform_draw_rect(16, 8, 208, 144, 0x0842);
    platform_draw_text(92, 12, "SHOP", 0x03FF);

    char gold_str[24];
    snprintf(gold_str, sizeof(gold_str), "Gold: %d", ctx->party.gold);
    platform_draw_text(160, 12, gold_str, 0x03FF);

    for (int i = 0; i < shop_count; i++) {
        const ItemData *item = inventory_get_item_data(shop_ids[i]);
        if (!item) continue;

        int y = 30 + i * 16;
        uint16_t color = (i == ctx->shop_cursor) ? 0x03FF : 0x7FFF;

        if (i == ctx->shop_cursor) {
            platform_draw_text(20, y, ">", 0x03FF);
        }

        char line[40];
        snprintf(line, sizeof(line), "%-14s %dG", item->name, item->price);
        platform_draw_text(32, y, line, color);
    }

    /* Description of selected item */
    if (ctx->shop_cursor < shop_count) {
        const ItemData *sel = inventory_get_item_data(shop_ids[ctx->shop_cursor]);
        if (sel) {
            platform_draw_rect(16, 132, 208, 20, 0x0421);
            platform_draw_text(20, 134, sel->description, 0x5EF7);
            platform_draw_text(20, 144, "[A] Buy  [B] Exit", 0x5294);
        }
    }
}

/* ── Game Over Screen ─────────────────────────────────── */
void state_game_over_update(GameContext *ctx) {
    uint16_t keys = platform_keys_pressed();
    if (keys & KEY_START) {
        game_init(ctx);
    }
}

void state_game_over_render(GameContext *ctx) {
    platform_clear(0x0000);
    if (ctx->victory) {
        /* Victory screen */
        platform_draw_text(40, 40, "CONGRATULATIONS!", 0x03FF);
        platform_draw_text(28, 60, "All 7 treasures found!", 0x7FFF);
        platform_draw_text(36, 80, "You are the true", 0x5EF7);
        platform_draw_text(36, 92, "Treasure Hunter!", 0x5EF7);
        treasure_render_status(ctx);
        if ((ctx->frame_count / 30) % 2 == 0) {
            platform_draw_text(48, 130, "PRESS START", 0x7FFF);
        }
    } else {
        /* Defeat screen */
        platform_draw_text(64, 60, "GAME OVER", 0x001F);
        platform_draw_text(44, 80, "Your quest ends here...", 0x5294);
        if ((ctx->frame_count / 30) % 2 == 0) {
            platform_draw_text(48, 110, "PRESS START", 0x7FFF);
        }
    }
}
