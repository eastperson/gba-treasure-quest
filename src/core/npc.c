/**
 * npc.c — NPC system implementation
 * Platform-independent: uses platform.h functions only.
 */
#include "npc.h"
#include "platform.h"
#include <string.h>

/* ── NPC Storage ───────────────────────────────────────── */
static NPCData g_npcs[MAX_NPCS];
static uint8_t g_npc_count;

/* ── NPC Colors (BGR555) by type ───────────────────────── */
static const uint16_t npc_colors[NPC_TYPE_COUNT] = {
    [NPC_VILLAGER]       = 0x7FFF,  /* white */
    [NPC_SHOPKEEPER]     = 0x03FF,  /* yellow */
    [NPC_GUARD]          = 0x7C00,  /* blue */
    [NPC_SAGE]           = 0x7C1F,  /* purple */
    [NPC_ENEMY_OVERWORLD]= 0x001F,  /* red */
};

/* ── Helper: add NPC to array ──────────────────────────── */
static void npc_add(NPCType type, int16_t x, int16_t y,
                    uint8_t sprite_id, uint8_t dialogue_id,
                    const char *name)
{
    if (g_npc_count >= MAX_NPCS) return;

    NPCData *npc = &g_npcs[g_npc_count];
    memset(npc, 0, sizeof(NPCData));
    npc->type       = type;
    npc->x          = x;
    npc->y          = y;
    npc->direction  = 0;
    npc->sprite_id  = sprite_id;
    npc->dialogue_id= dialogue_id;
    npc->active     = true;
    strncpy(npc->name, name, MAX_NAME_LEN - 1);
    npc->name[MAX_NAME_LEN - 1] = '\0';

    g_npc_count++;
}

/* ── Map NPC Placement ─────────────────────────────────── */
void npc_init_map(uint8_t map_id) {
    memset(g_npcs, 0, sizeof(g_npcs));
    g_npc_count = 0;

    switch (map_id) {
        case 0: /* Starter Town */
            npc_add(NPC_VILLAGER,   10, 8,  1, 0, "Villager");
            npc_add(NPC_SHOPKEEPER, 15, 5,  2, 1, "Merchant");
            npc_add(NPC_SAGE,       20, 12, 3, 2, "Old Sage");
            npc_add(NPC_VILLAGER,   5,  14, 4, 3, "Fisher");
            break;

        case 1: /* Dark Forest */
            npc_add(NPC_GUARD,           5,  10, 5, 4, "Guard");
            npc_add(NPC_ENEMY_OVERWORLD, 22, 15, 6, 5, "Goblin");
            break;

        case 2: /* Desert Oasis */
            npc_add(NPC_SHOPKEEPER, 14, 8,  7, 20, "Oasis Trader");
            npc_add(NPC_VILLAGER,   20, 10, 8, 21, "Nomad");
            npc_add(NPC_SAGE,       24, 5,  9, 22, "Temple Sage");
            break;

        case 3: /* Volcanic Coast */
            npc_add(NPC_GUARD,      14, 6, 10, 23, "Fire Warden");
            npc_add(NPC_SHOPKEEPER, 10, 8, 11, 24, "Lava Smith");
            break;

        case 4: /* Frozen Peaks */
            npc_add(NPC_VILLAGER,   17, 5, 12, 25, "Mountaineer");
            npc_add(NPC_SHOPKEEPER, 19, 5, 13, 26, "Ice Merchant");
            npc_add(NPC_SAGE,       18, 8, 14, 27, "Frost Sage");
            break;

        case 5: /* Sunken Ruins */
            npc_add(NPC_SAGE,       5,  6, 15, 28, "Ruin Scholar");
            npc_add(NPC_GUARD,      24, 11,16, 29, "Ruin Guard");
            break;

        case 6: /* Sky Temple */
            npc_add(NPC_SAGE,       15, 12,17, 30, "Sky Oracle");
            npc_add(NPC_GUARD,      11, 10,18, 31, "Temple Knight");
            npc_add(NPC_GUARD,      19, 10,19, 31, "Temple Knight");
            break;

        default:
            break;
    }
}

/* ── Update: simple wander behavior ────────────────────── */
void npc_update(uint32_t frame_count) {
    for (int i = 0; i < g_npc_count; i++) {
        NPCData *npc = &g_npcs[i];
        if (!npc->active) continue;

        /* Villagers wander randomly every 60 frames */
        if (npc->type == NPC_VILLAGER && (frame_count % 60) == 0) {
            /* Simple pseudo-random: use frame_count + index to vary */
            uint32_t rnd = (frame_count / 60 + (uint32_t)i * 7) % 5;
            int16_t dx = 0, dy = 0;

            switch (rnd) {
                case 0: dy = -1; npc->direction = 1; break; /* up */
                case 1: dy =  1; npc->direction = 0; break; /* down */
                case 2: dx = -1; npc->direction = 2; break; /* left */
                case 3: dx =  1; npc->direction = 3; break; /* right */
                default: break; /* stay put */
            }

            /* Basic bounds check: don't wander off map (stay within 2..28) */
            int16_t nx = npc->x + dx;
            int16_t ny = npc->y + dy;
            if (nx >= 2 && nx <= 28 && ny >= 2 && ny <= 18) {
                npc->x = nx;
                npc->y = ny;
            }
        }
    }
}

/* ── Draw ──────────────────────────────────────────────── */
void npc_draw(int16_t cam_x, int16_t cam_y) {
    for (int i = 0; i < g_npc_count; i++) {
        NPCData *npc = &g_npcs[i];
        if (!npc->active) continue;

        int screen_x = npc->x * 8 - cam_x;
        int screen_y = npc->y * 8 - cam_y;

        /* Only draw if on screen */
        if (screen_x < -8 || screen_x >= SCREEN_W ||
            screen_y < -8 || screen_y >= SCREEN_H) {
            continue;
        }

        /* Draw NPC as a colored 8x8 block */
        uint16_t color = npc_colors[npc->type];
        platform_draw_rect(screen_x, screen_y, 8, 8, color);

        /* Draw a small marker in center to distinguish from tiles */
        uint16_t marker = 0x0000; /* black dot */
        platform_draw_rect(screen_x + 3, screen_y + 3, 2, 2, marker);
    }
}

/* ── Interaction Check ─────────────────────────────────── */
NPCData *npc_check_interaction(int16_t player_x, int16_t player_y, uint8_t direction) {
    /* Calculate the tile the player is facing */
    int16_t target_x = player_x;
    int16_t target_y = player_y;

    switch (direction) {
        case 0: target_y++; break; /* facing down */
        case 1: target_y--; break; /* facing up */
        case 2: target_x--; break; /* facing left */
        case 3: target_x++; break; /* facing right */
    }

    for (int i = 0; i < g_npc_count; i++) {
        NPCData *npc = &g_npcs[i];
        if (!npc->active) continue;

        if (npc->x == target_x && npc->y == target_y) {
            return npc;
        }
    }

    return NULL;
}

/* ── Position Query (for collision) ────────────────────── */
bool npc_is_at(int16_t x, int16_t y) {
    for (int i = 0; i < g_npc_count; i++) {
        if (!g_npcs[i].active) continue;
        if (g_npcs[i].x == x && g_npcs[i].y == y) {
            return true;
        }
    }
    return false;
}
