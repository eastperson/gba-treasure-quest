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
    ctx->state   = STATE_INTRO;
    ctx->intro_page = 0;
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
    hero->skill_id  = 0;  /* Power Slash */
    ctx->party.count = 1;
    ctx->party.gold  = 100;

    /* Starting position — first island, town center */
    ctx->pos.x     = 7;
    ctx->pos.y     = 5;
    ctx->pos.map_id    = 0;
    ctx->pos.direction = 0;

    ctx->current_island  = 0;
    ctx->treasures_found = 0;
    ctx->visited_islands = 0x01;  /* Island 0 visited at start */
    ctx->inv_cursor      = 0;
    ctx->port_cursor     = 0;
    ctx->status_cursor   = 0;
    memset(ctx->quest_flags, 0, sizeof(ctx->quest_flags));

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
    ctx->shop_mode   = 0;
    ctx->drop_msg[0] = '\0';
    ctx->drop_msg_timer = 0;

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
        case STATE_INTRO:     state_intro_update(ctx);  break;
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
        case STATE_PORT_SELECT:
            state_port_select_update(ctx);
            break;
        case STATE_STATUS:
            state_status_update(ctx);
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
        case STATE_INTRO:     state_intro_render(ctx);  break;
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
        case STATE_PORT_SELECT:
            state_port_select_render(ctx);
            break;
        case STATE_STATUS:
            state_status_render(ctx);
            break;
        case STATE_GAME_OVER:
            state_game_over_render(ctx);
            break;
    }

    /* Draw transition overlay on top of everything */
    transition_render();

    platform_frame_end();
}

/* ── Intro Story Screen ───────────────────────────────── */
/* Story text for each intro page (Korean on web, English fallback) */
#ifdef PLATFORM_WEB
static const char *INTRO_TEXT[INTRO_PAGES] = {
    "\xEC\x98\xA4\xEB\x9E\x98\xEC\xA0\x84, \xEC\x9D\xBC\xEA\xB3\xB1 \xEA\xB0\x9C\xEC\x9D\x98 \xEC\x84\xAC\xEC\x97\x90\n"
    "\xEC\xA0\x84\xEC\x84\xA4\xEC\x9D\x98 \xEB\xB3\xB4\xEB\xAC\xBC\xEC\x9D\xB4\n"
    "\xED\x9D\xA9\xEC\x96\xB4\xEC\xA1\x8C\xEB\x8B\xA4\xEA\xB3\xA0 \xED\x95\xA9\xEB\x8B\x88\xEB\x8B\xA4...",
    /* 오래전, 일곱 개의 섬에\n전설의 보물이\n흩어졌다고 합니다... */

    "\xEA\xB0\x81 \xEC\x84\xAC\xEC\x97\x90\xEB\x8A\x94 \xEB\xAC\xB4\xEC\x8B\x9C\xEB\xAC\xB4\xEC\x8B\x9C\xED\x95\x9C\n"
    "\xEB\xAA\xAC\xEC\x8A\xA4\xED\x84\xB0\xEC\x99\x80 \xEB\x93\x9C\xEB\x9E\x98\xEA\xB3\xA4\xEC\x9D\xB4\n"
    "\xEB\xB3\xB4\xEB\xAC\xBC\xEC\x9D\x84 \xEC\xA7\x80\xED\x82\xA4\xEA\xB3\xA0 \xEC\x9E\x88\xEC\x8A\xB5\xEB\x8B\x88\xEB\x8B\xA4.",
    /* 각 섬에는 무시무시한\n몬스터와 드래곤이\n보물을 지키고 있습니다. */

    "\xEB\x8B\xB9\xEC\x8B\xA0\xEC\x9D\x80 \xEC\x9A\xA9\xEA\xB0\x90\xED\x95\x9C \xEB\xAA\xA8\xED\x97\x98\xEA\xB0\x80!\n"
    "\xEB\x8F\x99\xEB\xA3\x8C\xEB\x93\xA4\xEC\x9D\x84 \xEB\xAA\xA8\xEC\x9C\xBC\xEA\xB3\xA0\n"
    "\xEC\x9D\xBC\xEA\xB3\xB1 \xEC\x84\xAC\xEC\x9D\x84 \xED\x83\x90\xED\x97\x98\xED\x95\x98\xEC\x84\xB8\xEC\x9A\x94!",
    /* 당신은 용감한 모험가!\n동료들을 모으고\n일곱 섬을 탐험하세요! */

    "\xEB\xAA\xA8\xEB\x93\xA0 \xEB\xB3\xB4\xEB\xAC\xBC\xEC\x9D\x84 \xEC\xB0\xBE\xEC\x95\x84\n"
    "\xEC\xA0\x84\xEC\x84\xA4\xEC\x9D\x98 \xEB\xB9\x84\xEB\xB0\x80\xEC\x9D\x84\n"
    "\xEB\xB0\x9D\xED\x98\x80\xEC\xA3\xBC\xEC\x84\xB8\xEC\x9A\x94!",
    /* 모든 보물을 찾아\n전설의 비밀을\n밝혀주세요! */
};
#else
static const char *INTRO_TEXT[INTRO_PAGES] = {
    "Long ago, legendary\ntreasures were scattered\nacross seven islands...",
    "Fearsome monsters and\ndragons guard the\ntreasures on each island.",
    "You are a brave adventurer!\nGather companions and\nexplore the seven islands!",
    "Find all the treasures\nand uncover the secret\nof the legend!",
};
#endif

void state_intro_update(GameContext *ctx) {
    uint16_t keys = platform_keys_pressed();
    if (keys & (KEY_A | KEY_START)) {
        audio_play_sfx(SFX_CONFIRM);
        ctx->intro_page++;
        if (ctx->intro_page >= INTRO_PAGES) {
            ctx->state = STATE_TITLE;
        }
    }
    /* Skip intro with B */
    if (keys & KEY_B) {
        ctx->state = STATE_TITLE;
    }
}

void state_intro_render(GameContext *ctx) {
    platform_clear(0x0000); /* black */

    /* Starfield-like decorative dots */
    for (int i = 0; i < 20; i++) {
        int sx = (i * 47 + ctx->frame_count / 3) % SCREEN_W;
        int sy = (i * 31 + 17) % SCREEN_H;
        uint16_t star_c = (ctx->frame_count / 10 + i) % 3 == 0 ? 0x5294 : 0x294A;
        platform_draw_rect(sx, sy, 1, 1, star_c);
    }

    /* Page indicator dots */
    for (int i = 0; i < INTRO_PAGES; i++) {
        uint16_t dot_c = (i == ctx->intro_page) ? 0x7FFF : 0x294A;
        platform_draw_rect(106 + i * 10, 140, 4, 4, dot_c);
    }

    /* Text box */
    platform_draw_rect(16, 30, 208, 90, 0x0421); /* dark border */
    platform_draw_rect(18, 32, 204, 86, 0x0842); /* slightly lighter bg */
    platform_draw_rect(20, 34, 200, 82, 0x0000); /* black interior */

    /* Story text */
    const char *page_text = INTRO_TEXT[ctx->intro_page];
    /* Render line by line */
    int tx = 28, ty = 44;
    const char *p = page_text;
    while (*p) {
        if (*p == '\n') {
            tx = 28;
            ty += 16;
            p++;
            continue;
        }
        /* Find end of this line */
        const char *line_end = p;
        while (*line_end && *line_end != '\n') line_end++;
        /* Draw this segment */
        int seg_len = (int)(line_end - p);
        char line_buf[64];
        if (seg_len >= (int)sizeof(line_buf)) seg_len = sizeof(line_buf) - 1;
        memcpy(line_buf, p, seg_len);
        line_buf[seg_len] = '\0';
        platform_draw_text(tx, ty, line_buf, 0x7FFF);
        p = line_end;
        if (*p == '\n') { p++; tx = 28; ty += 16; }
    }

    /* Prompt */
    if ((ctx->frame_count / 25) % 2 == 0) {
#ifdef PLATFORM_WEB
        platform_draw_text(80, 150, "A: \xEB\x8B\xA4\xEC\x9D\x8C  B: \xEA\xB1\xB4\xEB\x84\x88\xEB\x9B\xB0\xEA\xB8\xB0", 0x5294);
        /* A: 다음  B: 건너뛰기 */
#else
        platform_draw_text(76, 150, "A: Next   B: Skip", 0x5294);
#endif
    }
}

/* ── Title Screen ──────────────────────────────────────── */
void state_title_update(GameContext *ctx) {
    uint16_t keys = platform_keys_pressed();
    if (keys & (KEY_START | KEY_A)) {
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
#ifdef PLATFORM_WEB
    platform_draw_text(36, 26, "\xEB\xB3\xB4\xEB\xAC\xBC \xED\x83\x90\xED\x97\x98", 0x03FF);
    /* 보물 탐험 */
    platform_draw_text(42, 42, "\xEC\x9D\xBC\xEA\xB3\xB1 \xEC\x84\xAC\xEC\x9D\x98 \xEB\xB9\x84\xEB\xB0\x80", 0x5EF7);
    /* 일곱 섬의 비밀 */
#else
    platform_draw_text(40, 28, "TREASURE  QUEST", 0x03FF);
    platform_draw_text(48, 44, "Seven Islands", 0x5EF7);
#endif

    /* Decorative treasure icons */
    for (int i = 0; i < 7; i++) {
        int ix = 54 + i * 20;
        uint16_t ic = (uint16_t)(0x001F + i * 0x0400);
        platform_draw_rect(ix, 68, 8, 8, ic);
        platform_draw_rect(ix + 1, 69, 6, 6, 0x03FF);
    }

    /* Menu options */
#ifdef PLATFORM_WEB
    platform_draw_text(60, 90, "\xEC\x83\x88\xEB\xA1\x9C\xEC\x9A\xB4 \xEB\xAA\xA8\xED\x97\x98", 0x7FFF);
    /* 새로운 모험 */
#else
    platform_draw_text(76, 90, "New Adventure", 0x7FFF);
#endif

    /* Blink start prompt */
    if ((ctx->frame_count / 30) % 2 == 0) {
#ifdef PLATFORM_WEB
        platform_draw_text(46, 106, "- START \xEB\x98\x90\xEB\x8A\x94 A \xEB\xB2\x84\xED\x8A\xBC -", 0x7FFF);
        /* - START 또는 A 버튼 - */
#else
        platform_draw_text(68, 106, "- PRESS START -", 0x7FFF);
#endif
    }

    /* Credits */
    platform_draw_text(56, 132, "Sprint 21 Edition", 0x294A);
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
                dialogue_start_with_flags(npc->dialogue_id, ctx->quest_flags);
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
                    /* Open island selection UI instead of auto-warp */
                    audio_play_sfx(SFX_DOOR);
                    /* Mark next island as visitable (unlock progression) */
                    {
                        uint8_t next = trig->id;
                        if (next < 7) {
                            ctx->visited_islands |= (1 << next);
                        }
                    }
                    ctx->port_cursor = 0;
                    ctx->state = STATE_PORT_SELECT;
                    /* Don't mark trigger as triggered — ports are reusable */
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

    /* Decrement drop message timer */
    if (ctx->drop_msg_timer > 0) {
        ctx->drop_msg_timer--;
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

    /* Item drop message overlay */
    if (ctx->drop_msg_timer > 0 && ctx->drop_msg[0] != '\0') {
        platform_draw_rect(40, 70, 160, 14, 0x0000);
        platform_draw_text(44, 72, ctx->drop_msg, 0x03FF);
    }
}

/* ── Item Drop Table (by enemy sprite_id 16-25 → item_id, chance%) ── */
typedef struct { uint8_t item_id; uint8_t chance; } DropEntry;
#define DROP_TABLE_BASE 16
#define DROP_TABLE_COUNT 10

static const DropEntry enemy_drops[DROP_TABLE_COUNT] = {
    /* sprite 16: Slime   */ { 0, 30 },  /* Potion 30% */
    /* sprite 17: Goblin  */ { 0, 25 },  /* Potion 25% */
    /* sprite 18: Scorpion*/ { 3, 20 },  /* Antidote 20% */
    /* sprite 19: Fire Bat*/ { 2, 20 },  /* Ether 20% */
    /* sprite 20: Ice Gol */ { 1, 15 },  /* Hi-Potion 15% */
    /* sprite 21: Wraith  */ { 2, 20 },  /* Ether 20% */
    /* sprite 22: Temple G*/ { 5, 10 },  /* Skeleton Key 10% */
    /* sprite 23: FDragon */ { 4, 40 },  /* Bomb 40% */
    /* sprite 24: Kraken  */ { 1, 50 },  /* Hi-Potion 50% */
    /* sprite 25: Sky Lord*/ { 5, 40 },  /* Skeleton Key 40% */
};

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

                /* Item drop check */
                ctx->drop_msg[0] = '\0';
                ctx->drop_msg_timer = 0;
                {
                    int didx = g_battle.enemy.sprite_id - DROP_TABLE_BASE;
                    if (didx >= 0 && didx < DROP_TABLE_COUNT) {
                        uint32_t rng = (ctx->frame_count * 1103515245u + 12345u) >> 16;
                        if ((int)(rng % 100) < enemy_drops[didx].chance) {
                            int drop_id = enemy_drops[didx].item_id;
                            inventory_add(&ctx->inventory, drop_id, 1);
                            const ItemData *di = inventory_get_item_data(drop_id);
                            if (di) {
                                snprintf(ctx->drop_msg, sizeof(ctx->drop_msg),
                                         "Got %s!", di->name);
                                ctx->drop_msg_timer = 90;
                            }
                        }
                    }
                }

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

    /* Toggle buy/sell mode with L/R */
    if (keys & (KEY_L | KEY_R)) {
        ctx->shop_mode = ctx->shop_mode ? 0 : 1;
        ctx->shop_cursor = 0;
    }

    if (ctx->shop_mode == 0) {
        /* ── BUY mode ── */
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
    } else {
        /* ── SELL mode ── */
        if (keys & KEY_UP) {
            if (ctx->shop_cursor > 0) ctx->shop_cursor--;
        }
        if (keys & KEY_DOWN) {
            if (ctx->inventory.count > 0 &&
                ctx->shop_cursor < ctx->inventory.count - 1) {
                ctx->shop_cursor++;
            }
        }
        if (keys & KEY_A) {
            if (ctx->inventory.count > 0 && ctx->shop_cursor < ctx->inventory.count) {
                const ItemData *item = inventory_get_item_data(
                    ctx->inventory.slots[ctx->shop_cursor].item_id);
                if (item && item->type != ITEM_TREASURE && item->type != ITEM_KEY) {
                    int sell_price = (int)item->price / 2;
                    ctx->party.gold += (int16_t)sell_price;
                    inventory_remove(&ctx->inventory, ctx->shop_cursor, 1);
                    audio_play_sfx(SFX_CONFIRM);
                    if (ctx->shop_cursor >= ctx->inventory.count && ctx->shop_cursor > 0) {
                        ctx->shop_cursor--;
                    }
                }
            }
        }
    }

    if (keys & KEY_B) {
        ctx->shop_mode = 0;
        ctx->state = STATE_WORLD;
    }
}

void state_shop_render(GameContext *ctx) {
    platform_clear(0x0000);
    platform_draw_rect(16, 8, 208, 144, 0x0842);

    char gold_str[24];
    snprintf(gold_str, sizeof(gold_str), "Gold: %d", ctx->party.gold);
    platform_draw_text(160, 12, gold_str, 0x03FF);

    /* Mode tabs */
    platform_draw_text(40, 12, "BUY", ctx->shop_mode == 0 ? 0x03FF : 0x5294);
    platform_draw_text(80, 12, "SELL", ctx->shop_mode == 1 ? 0x03FF : 0x5294);

    if (ctx->shop_mode == 0) {
        /* ── BUY mode ── */
        int shop_count = 0;
        const uint8_t *shop_ids = get_shop_items(ctx->current_island, &shop_count);

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

        if (ctx->shop_cursor < shop_count) {
            const ItemData *sel = inventory_get_item_data(shop_ids[ctx->shop_cursor]);
            if (sel) {
                platform_draw_rect(16, 132, 208, 20, 0x0421);
                platform_draw_text(20, 134, sel->description, 0x5EF7);
                platform_draw_text(20, 144, "[A]Buy [B]Exit [L/R]Sell", 0x5294);
            }
        }
    } else {
        /* ── SELL mode ── */
        if (ctx->inventory.count == 0) {
            platform_draw_text(72, 70, "No items", 0x5294);
        } else {
            for (int i = 0; i < ctx->inventory.count && i < 6; i++) {
                const ItemData *item = inventory_get_item_data(
                    ctx->inventory.slots[i].item_id);
                if (!item) continue;

                int y = 30 + i * 16;
                uint16_t color = (i == ctx->shop_cursor) ? 0x03FF : 0x7FFF;
                if (i == ctx->shop_cursor) {
                    platform_draw_text(20, y, ">", 0x03FF);
                }
                int sell_price = (int)item->price / 2;
                char line[40];
                snprintf(line, sizeof(line), "%-12s x%d %dG",
                         item->name, ctx->inventory.slots[i].count, sell_price);
                platform_draw_text(32, y, line, color);
            }

            if (ctx->shop_cursor < ctx->inventory.count) {
                const ItemData *sel = inventory_get_item_data(
                    ctx->inventory.slots[ctx->shop_cursor].item_id);
                if (sel) {
                    platform_draw_rect(16, 132, 208, 20, 0x0421);
                    platform_draw_text(20, 134, sel->description, 0x5EF7);
                    platform_draw_text(20, 144, "[A]Sell [B]Exit [L/R]Buy", 0x5294);
                }
            }
        }
    }
}

/* ── Island Names Table ───────────────────────────────── */
static const char *island_names[7] = {
    "Starter Town",
    "Dark Forest",
    "Desert Oasis",
    "Volcanic Coast",
    "Frozen Peaks",
    "Sunken Ruins",
    "Sky Temple",
};

/* ── Port Warp Helper (handles recruitment + map load) ── */
static void port_warp_to_island(GameContext *ctx, uint8_t dest_map) {
    /* Party recruitment events when leaving certain islands */
    /* Island 1 (Dark Forest) -> recruit Elara (Mage) */
    if (ctx->current_island == 1 && dest_map != 1 && ctx->party.count < MAX_PARTY_SIZE) {
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
            ctx->party.members[ctx->party.count - 1].skill_id = 1; /* Arcane Burst */
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
            ctx->party.members[ctx->party.count - 1].skill_id = 2; /* Berserk */
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
            ctx->party.members[ctx->party.count - 1].skill_id = 3; /* Revive */
        }
    }

    map_load(&g_map, dest_map);
    npc_init_map(dest_map);
    ctx->pos.x = g_map.spawn_x;
    ctx->pos.y = g_map.spawn_y;
    ctx->pos.map_id = dest_map;
    ctx->current_island = dest_map;
    ctx->visited_islands |= (1 << dest_map);
    audio_set_bgm_for_island(dest_map);
    transition_start(TRANS_FADE_OUT, 15, STATE_WORLD);
}

/* ── Port Selection Screen ────────────────────────────── */
void state_port_select_update(GameContext *ctx) {
    uint16_t keys = platform_keys_pressed();

    if (keys & KEY_UP) {
        if (ctx->port_cursor > 0) ctx->port_cursor--;
    }
    if (keys & KEY_DOWN) {
        if (ctx->port_cursor < 6) ctx->port_cursor++;
    }
    if (keys & KEY_A) {
        uint8_t dest = ctx->port_cursor;
        /* Can only travel to visited islands, and not current island */
        if ((ctx->visited_islands & (1 << dest)) && dest != ctx->current_island) {
            audio_play_sfx(SFX_CONFIRM);
            port_warp_to_island(ctx, dest);
        } else {
            audio_play_sfx(SFX_CANCEL);
        }
    }
    if (keys & KEY_B) {
        ctx->state = STATE_WORLD;
    }
}

void state_port_select_render(GameContext *ctx) {
    platform_clear(0x0000);
    platform_draw_rect(16, 8, 208, 144, 0x0842);

    platform_draw_text(60, 12, "SELECT DESTINATION", 0x03FF);

    for (int i = 0; i < 7; i++) {
        int y = 30 + i * 14;
        bool visited = (ctx->visited_islands & (1 << i)) != 0;
        bool is_current = (i == ctx->current_island);
        uint16_t color;

        if (i == ctx->port_cursor) {
            platform_draw_text(20, y, ">", 0x03FF);
            color = visited ? 0x03FF : 0x294A;
        } else {
            color = visited ? 0x7FFF : 0x294A;
        }

        char line[32];
        if (is_current) {
            snprintf(line, sizeof(line), "%-16s [HERE]", island_names[i]);
        } else if (visited) {
            snprintf(line, sizeof(line), "%-16s  [*]", island_names[i]);
        } else {
            snprintf(line, sizeof(line), "%-16s  [ ]", island_names[i]);
        }
        platform_draw_text(32, y, line, color);
    }

    /* Footer */
    platform_draw_rect(16, 134, 208, 18, 0x0421);
    platform_draw_text(20, 136, "[*]=Visited [ ]=Locked", 0x5EF7);
    platform_draw_text(20, 146, "[A]Go  [B]Cancel", 0x5294);
}

/* ── Status Screen ────────────────────────────────────── */
/* Character skill names */
static const char *skill_names[4] = {
    "Power Slash",   /* Hero — physical 1.8x ATK */
    "Arcane Burst",  /* Elara — magic 25 damage */
    "Berserk",       /* Drake — ATKx2, DEF=0 next turn */
    "Revive",        /* Naia — revive fallen ally 30% HP */
};

void state_status_update(GameContext *ctx) {
    uint16_t keys = platform_keys_pressed();

    if (keys & KEY_UP) {
        if (ctx->status_cursor > 0) ctx->status_cursor--;
    }
    if (keys & KEY_DOWN) {
        if (ctx->status_cursor < ctx->party.count - 1) ctx->status_cursor++;
    }
    if (keys & KEY_B) {
        ctx->state = STATE_MENU;
    }
}

void state_status_render(GameContext *ctx) {
    platform_clear(0x0000);
    platform_draw_rect(8, 4, 224, 152, 0x0842);

    platform_draw_text(76, 8, "PARTY STATUS", 0x03FF);

    for (int i = 0; i < ctx->party.count && i < MAX_PARTY_SIZE; i++) {
        Character *ch = &ctx->party.members[i];
        int base_y = 22 + i * 32;
        uint16_t name_color = (i == ctx->status_cursor) ? 0x03FF : 0x7FFF;

        /* Name and level */
        char line1[40];
        snprintf(line1, sizeof(line1), "%s  Lv%d", ch->name, ch->level);
        platform_draw_text(12, base_y, line1, name_color);

        /* HP/MP */
        char line2[40];
        snprintf(line2, sizeof(line2), "HP:%d/%d MP:%d/%d EXP:%d",
                 ch->hp, ch->max_hp, ch->mp, ch->max_mp, ch->exp);
        platform_draw_text(16, base_y + 10, line2, 0x5EF7);

        /* ATK/DEF/SPD + Equipment + Skill */
        char line3[48];
        const char *wpn = (ch->weapon_id >= 0)
            ? inventory_get_item_data(ch->weapon_id)->name : "None";
        snprintf(line3, sizeof(line3), "ATK:%d DEF:%d SPD:%d W:%s",
                 ch->atk, ch->def, ch->spd, wpn);
        platform_draw_text(16, base_y + 20, line3, 0x5294);
    }

    /* Footer with skill info for selected character */
    if (ctx->status_cursor < ctx->party.count) {
        Character *sel = &ctx->party.members[ctx->status_cursor];
        char skill_line[40];
        snprintf(skill_line, sizeof(skill_line), "Skill: %s",
                 (sel->skill_id < 4) ? skill_names[sel->skill_id] : "None");
        platform_draw_rect(8, 148, 224, 10, 0x0421);
        platform_draw_text(12, 149, skill_line, 0x03FF);
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
        platform_draw_text(40, 20, "CONGRATULATIONS!", 0x03FF);
        platform_draw_text(28, 36, "All 7 treasures found!", 0x7FFF);
        platform_draw_text(36, 50, "You are the true", 0x5EF7);
        platform_draw_text(36, 62, "Treasure Hunter!", 0x5EF7);
        treasure_render_status(ctx);

        /* Statistics */
        {
            Character *h = &ctx->party.members[0];
            char stat1[40], stat2[40], stat3[40];
            int minutes = (int)(ctx->frame_count / 3600);
            int seconds = (int)((ctx->frame_count / 60) % 60);
            snprintf(stat1, sizeof(stat1), "Time: %d:%02d", minutes, seconds);
            snprintf(stat2, sizeof(stat2), "Hero Lv%d  Party: %d members", h->level, ctx->party.count);
            snprintf(stat3, sizeof(stat3), "Treasures: %d/7  Gold: %d",
                     treasure_get_count(ctx), ctx->party.gold);
            platform_draw_text(60, 84, stat1, 0x5294);
            platform_draw_text(28, 96, stat2, 0x5294);
            platform_draw_text(28, 108, stat3, 0x5294);
        }

        if ((ctx->frame_count / 30) % 2 == 0) {
            platform_draw_text(48, 136, "PRESS START", 0x7FFF);
        }
    } else {
        /* Defeat screen */
        platform_draw_text(64, 40, "GAME OVER", 0x001F);
        platform_draw_text(44, 60, "Your quest ends here...", 0x5294);

        /* Statistics */
        {
            Character *h = &ctx->party.members[0];
            char stat1[40], stat2[40], stat3[40];
            int minutes = (int)(ctx->frame_count / 3600);
            int seconds = (int)((ctx->frame_count / 60) % 60);
            snprintf(stat1, sizeof(stat1), "Time: %d:%02d", minutes, seconds);
            snprintf(stat2, sizeof(stat2), "Hero Lv%d  Party: %d members", h->level, ctx->party.count);
            snprintf(stat3, sizeof(stat3), "Treasures: %d/7  Islands: %d",
                     treasure_get_count(ctx),
                     (int)__builtin_popcount(ctx->visited_islands));
            platform_draw_text(60, 80, stat1, 0x5294);
            platform_draw_text(28, 92, stat2, 0x5294);
            platform_draw_text(28, 104, stat3, 0x5294);
        }

        if ((ctx->frame_count / 30) % 2 == 0) {
            platform_draw_text(48, 130, "PRESS START", 0x7FFF);
        }
    }
}
