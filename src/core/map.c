/**
 * map.c — Tilemap system implementation
 * Platform-independent: no GBA or SDL headers here.
 */
#include "map.h"
#include "platform.h"
#include <string.h>

/* ── Tile Size ────────────────────────────────────────── */
#define TILE_SIZE 8

/* ── Patterned Tile Drawing ──────────────────────────── */
/*
 * draw_tile_pattern — draws an 8x8 pixel-art tile pattern using
 * platform_draw_rect for individual pixels or small rects.
 * tx, ty = tile coordinates for deterministic variation.
 */
static void draw_tile_pattern(int sx, int sy, TileType tile, int tx, int ty) {
    int i, j;
    uint16_t base, detail;

    switch (tile) {

    case TILE_GRASS:
        /* Green base with darker green dots */
        base   = 0x2108; /* dark green */
        detail = 0x0100; /* darker green */
        platform_draw_rect(sx, sy, 8, 8, base);
        /* Deterministic "random" dots */
        for (j = 0; j < 8; j += 2) {
            for (i = 0; i < 8; i += 2) {
                if (((tx * 7 + i) ^ (ty * 13 + j)) % 5 == 0) {
                    platform_draw_rect(sx + i, sy + j, 1, 1, detail);
                }
            }
        }
        /* A few lighter highlights */
        if ((tx + ty) % 3 == 0) {
            platform_draw_rect(sx + 3, sy + 5, 1, 1, 0x3DEF);
        }
        if ((tx + ty) % 4 == 1) {
            platform_draw_rect(sx + 6, sy + 2, 1, 1, 0x3DEF);
        }
        break;

    case TILE_WATER:
        /* Blue base with lighter wave lines */
        base   = 0x7C00; /* blue */
        detail = 0x7E10; /* lighter blue */
        platform_draw_rect(sx, sy, 8, 8, base);
        /* Horizontal wave lines with offset per row */
        for (j = 1; j < 8; j += 3) {
            int wave_off = ((tx + ty + j) % 3);
            for (i = wave_off; i < 8; i += 3) {
                platform_draw_rect(sx + i, sy + j, 2, 1, detail);
            }
        }
        /* Sparkle dot */
        if ((tx * 3 + ty * 5) % 7 == 0) {
            platform_draw_rect(sx + ((tx + ty) % 6) + 1, sy + 2, 1, 1, 0x7FFF);
        }
        break;

    case TILE_SAND:
        /* Sandy yellow with brown speckles */
        base   = 0x2D7F; /* sandy yellow */
        detail = 0x155F; /* brown-ish */
        platform_draw_rect(sx, sy, 8, 8, base);
        for (j = 0; j < 8; j += 2) {
            for (i = 1; i < 8; i += 3) {
                if (((tx * 11 + i) ^ (ty * 7 + j)) % 7 == 0) {
                    platform_draw_rect(sx + i, sy + j, 1, 1, detail);
                }
            }
        }
        /* Lighter sand grain */
        if ((tx + ty) % 2 == 0) {
            platform_draw_rect(sx + 5, sy + 3, 1, 1, 0x3FFF);
        }
        break;

    case TILE_STONE:
        /* Gray base with darker crack lines */
        base   = 0x4A52; /* gray */
        detail = 0x2529; /* dark gray */
        platform_draw_rect(sx, sy, 8, 8, base);
        /* Horizontal crack */
        platform_draw_rect(sx + 1, sy + 3, 3, 1, detail);
        platform_draw_rect(sx + 3, sy + 4, 2, 1, detail);
        /* Vertical crack */
        if ((tx + ty) % 2 == 0) {
            platform_draw_rect(sx + 5, sy + 1, 1, 3, detail);
        } else {
            platform_draw_rect(sx + 2, sy + 5, 1, 2, detail);
            platform_draw_rect(sx + 6, sy + 0, 1, 2, detail);
        }
        /* Light speckle */
        platform_draw_rect(sx + ((tx * 3) % 7), sy + ((ty * 5) % 7), 1, 1, 0x5AD6);
        break;

    case TILE_TREE:
        /* Brown trunk center, green foliage top */
        platform_draw_rect(sx, sy, 8, 8, 0x2108); /* grass under tree */
        /* Foliage (top 5 rows) */
        platform_draw_rect(sx + 1, sy + 0, 6, 2, 0x01C0); /* bright green */
        platform_draw_rect(sx + 0, sy + 2, 8, 2, 0x01C0);
        platform_draw_rect(sx + 1, sy + 4, 6, 1, 0x0180); /* darker green */
        /* Dark spots in foliage */
        platform_draw_rect(sx + 2, sy + 1, 1, 1, 0x0100);
        platform_draw_rect(sx + 5, sy + 2, 1, 1, 0x0100);
        platform_draw_rect(sx + 3, sy + 3, 1, 1, 0x0100);
        /* Trunk (bottom 3 rows, center 2 pixels wide) */
        platform_draw_rect(sx + 3, sy + 5, 2, 3, 0x015F); /* brown */
        platform_draw_rect(sx + 3, sy + 6, 1, 1, 0x00BF); /* darker brown line */
        break;

    case TILE_WALL:
        /* Dark gray brick pattern with mortar lines */
        base   = 0x294A; /* dark gray */
        detail = 0x1CE7; /* mortar/lighter line */
        platform_draw_rect(sx, sy, 8, 8, base);
        /* Horizontal mortar lines */
        platform_draw_rect(sx, sy + 3, 8, 1, detail);
        platform_draw_rect(sx, sy + 7, 8, 1, detail);
        /* Vertical mortar lines (offset between rows for brick pattern) */
        platform_draw_rect(sx + 3, sy + 0, 1, 3, detail);
        platform_draw_rect(sx + 7, sy + 0, 1, 3, detail);
        platform_draw_rect(sx + 0, sy + 4, 1, 3, detail);
        platform_draw_rect(sx + 5, sy + 4, 1, 3, detail);
        break;

    case TILE_DOOR:
        /* Brown wood with dark keyhole/handle */
        base   = 0x015F; /* brown */
        platform_draw_rect(sx, sy, 8, 8, base);
        /* Door frame (darker edges) */
        platform_draw_rect(sx, sy, 1, 8, 0x00BF);
        platform_draw_rect(sx + 7, sy, 1, 8, 0x00BF);
        platform_draw_rect(sx, sy, 8, 1, 0x00BF);
        /* Wood grain lines */
        platform_draw_rect(sx + 3, sy + 1, 1, 7, 0x019F);
        /* Keyhole (dark) */
        platform_draw_rect(sx + 5, sy + 3, 1, 1, 0x0000);
        /* Handle (metallic) */
        platform_draw_rect(sx + 5, sy + 4, 1, 2, 0x4A52);
        break;

    case TILE_CHEST:
        /* Orange/gold box with dark lid line */
        platform_draw_rect(sx, sy, 8, 8, 0x2108); /* grass underneath */
        /* Chest body */
        platform_draw_rect(sx + 1, sy + 2, 6, 5, 0x02BF); /* orange */
        /* Lid top */
        platform_draw_rect(sx + 1, sy + 2, 6, 2, 0x035F); /* lighter gold */
        /* Lid line divider */
        platform_draw_rect(sx + 1, sy + 4, 6, 1, 0x0000); /* dark line */
        /* Latch/lock */
        platform_draw_rect(sx + 3, sy + 3, 2, 1, 0x03FF); /* bright yellow */
        platform_draw_rect(sx + 4, sy + 5, 1, 1, 0x0000); /* keyhole */
        /* Outline */
        platform_draw_rect(sx + 1, sy + 2, 6, 1, 0x00BF); /* dark top edge */
        break;

    case TILE_PORT:
        /* Gray dock planks with water showing through */
        platform_draw_rect(sx, sy, 8, 8, 0x7C00); /* water base */
        /* Horizontal planks */
        platform_draw_rect(sx, sy + 0, 8, 2, 0x4210); /* light gray plank */
        platform_draw_rect(sx, sy + 3, 8, 2, 0x4210);
        platform_draw_rect(sx, sy + 6, 8, 2, 0x4210);
        /* Plank detail lines */
        platform_draw_rect(sx, sy + 1, 8, 1, 0x3DEF); /* darker grain */
        platform_draw_rect(sx, sy + 4, 8, 1, 0x3DEF);
        platform_draw_rect(sx, sy + 7, 8, 1, 0x3DEF);
        /* Gaps showing water */
        platform_draw_rect(sx + 3, sy + 0, 1, 1, 0x7C00);
        platform_draw_rect(sx + 6, sy + 3, 1, 1, 0x7C00);
        break;

    default:
        platform_draw_rect(sx, sy, 8, 8, 0x7FFF);
        break;
    }
}

/* ── Hardcoded Map Data ───────────────────────────────── */

/*
 * Island 0: Starter Town (30x20)
 * Legend: G=grass W=water S=sand T=tree N=stone L=wall D=door C=chest P=port
 */
static void load_island_0(MapData *map) {
    strncpy(map->name, "Starter Town", MAX_MAP_NAME);
    map->width   = 30;
    map->height  = 20;
    map->spawn_x = 7;
    map->spawn_y = 10;

    /* Fill base with grass */
    memset(map->tiles, TILE_GRASS, sizeof(map->tiles));

    int w = map->width;
    int h = map->height;

    /* Water border on top and bottom rows */
    for (int x = 0; x < w; x++) {
        map->tiles[0][x]     = TILE_WATER;
        map->tiles[h-1][x]   = TILE_WATER;
    }
    /* Water border on left and right columns */
    for (int y = 0; y < h; y++) {
        map->tiles[y][0]     = TILE_WATER;
        map->tiles[y][w-1]   = TILE_WATER;
    }

    /* Sand beach (row 1 and row h-2 inner border) */
    for (int x = 1; x < w - 1; x++) {
        map->tiles[1][x]     = TILE_SAND;
        map->tiles[h-2][x]   = TILE_SAND;
    }
    for (int y = 1; y < h - 1; y++) {
        map->tiles[y][1]     = TILE_SAND;
        map->tiles[y][w-2]   = TILE_SAND;
    }

    /* Stone path (horizontal, row 10) */
    for (int x = 2; x < w - 2; x++) {
        map->tiles[10][x] = TILE_STONE;
    }
    /* Stone path (vertical, col 15) */
    for (int y = 2; y < h - 2; y++) {
        map->tiles[y][15] = TILE_STONE;
    }

    /* Town building — small house (walls + door) */
    for (int x = 10; x <= 14; x++) {
        map->tiles[4][x] = TILE_WALL;
        map->tiles[7][x] = TILE_WALL;
    }
    for (int y = 4; y <= 7; y++) {
        map->tiles[y][10] = TILE_WALL;
        map->tiles[y][14] = TILE_WALL;
    }
    map->tiles[7][12] = TILE_DOOR;  /* front door */

    /* Second building */
    for (int x = 20; x <= 24; x++) {
        map->tiles[4][x] = TILE_WALL;
        map->tiles[7][x] = TILE_WALL;
    }
    for (int y = 4; y <= 7; y++) {
        map->tiles[y][20] = TILE_WALL;
        map->tiles[y][24] = TILE_WALL;
    }
    map->tiles[7][22] = TILE_DOOR;

    /* Trees scattered */
    map->tiles[3][3]   = TILE_TREE;
    map->tiles[3][5]   = TILE_TREE;
    map->tiles[12][4]  = TILE_TREE;
    map->tiles[12][6]  = TILE_TREE;
    map->tiles[14][25] = TILE_TREE;
    map->tiles[15][26] = TILE_TREE;
    map->tiles[16][25] = TILE_TREE;
    map->tiles[13][3]  = TILE_TREE;
    map->tiles[14][3]  = TILE_TREE;

    /* Chest inside first building */
    map->tiles[5][12] = TILE_CHEST;

    /* Port at south beach */
    map->tiles[h-2][15] = TILE_PORT;

    /* Entities */
    map->entity_count = 0;

    /* Event triggers */
    map->trigger_count = 3;
    /* Treasure chest — item_id 6 (Coral Crown) */
    map->triggers[0] = (EventTrigger){ 12, 5, EVENT_CHEST, 6, false };
    /* Port at south beach — warps to island 1 (Dark Forest) */
    map->triggers[1] = (EventTrigger){ 15, map->height - 2, EVENT_PORT, 1, false };
    /* Sign near town entrance */
    map->triggers[2] = (EventTrigger){ 8, 10, EVENT_SIGN, 8, false };
}

/*
 * Island 1: Forest (30x20)
 * Dense forest with clearings and a hidden chest.
 */
static void load_island_1(MapData *map) {
    strncpy(map->name, "Dark Forest", MAX_MAP_NAME);
    map->width   = 30;
    map->height  = 20;
    map->spawn_x = 15;
    map->spawn_y = 18;

    int w = map->width;
    int h = map->height;

    /* Fill with trees */
    for (int y = 0; y < MAP_MAX_H; y++) {
        for (int x = 0; x < MAP_MAX_W; x++) {
            if (x < w && y < h)
                map->tiles[y][x] = TILE_TREE;
            else
                map->tiles[y][x] = TILE_GRASS;
        }
    }

    /* Water border */
    for (int x = 0; x < w; x++) {
        map->tiles[0][x]   = TILE_WATER;
        map->tiles[h-1][x] = TILE_WATER;
    }
    for (int y = 0; y < h; y++) {
        map->tiles[y][0]   = TILE_WATER;
        map->tiles[y][w-1] = TILE_WATER;
    }

    /* Sand beach on bottom */
    for (int x = 1; x < w - 1; x++) {
        map->tiles[h-2][x] = TILE_SAND;
    }

    /* Clear paths through forest (grass) */
    /* Main vertical path */
    for (int y = 1; y < h - 2; y++) {
        map->tiles[y][15] = TILE_GRASS;
        map->tiles[y][16] = TILE_GRASS;
    }
    /* Horizontal clearing */
    for (int x = 5; x < 25; x++) {
        map->tiles[10][x] = TILE_GRASS;
    }
    /* Entry from south */
    for (int x = 13; x <= 17; x++) {
        map->tiles[h-2][x] = TILE_SAND;
    }

    /* Clearing in upper area */
    for (int y = 3; y <= 6; y++) {
        for (int x = 6; x <= 10; x++) {
            map->tiles[y][x] = TILE_GRASS;
        }
    }

    /* Stone ruins in clearing */
    map->tiles[4][7]  = TILE_STONE;
    map->tiles[4][8]  = TILE_STONE;
    map->tiles[4][9]  = TILE_STONE;
    map->tiles[5][7]  = TILE_STONE;
    map->tiles[5][9]  = TILE_STONE;

    /* Hidden chest */
    map->tiles[5][8] = TILE_CHEST;

    /* Second clearing (east) */
    for (int y = 12; y <= 15; y++) {
        for (int x = 22; x <= 26; x++) {
            map->tiles[y][x] = TILE_GRASS;
        }
    }

    /* Port at bottom */
    map->tiles[h-2][15] = TILE_PORT;

    map->entity_count = 0;

    /* Event triggers */
    map->trigger_count = 3;
    /* Treasure chest in ruins — item_id 7 (Emerald Blade) */
    map->triggers[0] = (EventTrigger){ 8, 5, EVENT_CHEST, 7, false };
    /* Port at south — warps back to island 0 (Starter Town) */
    map->triggers[1] = (EventTrigger){ 15, h - 2, EVENT_PORT, 0, false };
    /* Warning sign at clearing */
    map->triggers[2] = (EventTrigger){ 15, 10, EVENT_SIGN, 9, false };
}

/*
 * Island 2: Desert Oasis (30x20)
 * Sandy terrain with a water oasis, stone ruins, and a small temple.
 */
static void load_island_2(MapData *map) {
    strncpy(map->name, "Desert Oasis", MAX_MAP_NAME);
    map->width   = 30;
    map->height  = 20;
    map->spawn_x = 15;
    map->spawn_y = 18;

    int w = map->width;
    int h = map->height;

    /* Fill base with sand */
    for (int y = 0; y < MAP_MAX_H; y++)
        for (int x = 0; x < MAP_MAX_W; x++)
            map->tiles[y][x] = (x < w && y < h) ? TILE_SAND : TILE_GRASS;

    /* Water border */
    for (int x = 0; x < w; x++) {
        map->tiles[0][x]   = TILE_WATER;
        map->tiles[h-1][x] = TILE_WATER;
    }
    for (int y = 0; y < h; y++) {
        map->tiles[y][0]   = TILE_WATER;
        map->tiles[y][w-1] = TILE_WATER;
    }

    /* Central oasis — water pool */
    for (int y = 8; y <= 12; y++)
        for (int x = 12; x <= 18; x++)
            map->tiles[y][x] = TILE_WATER;

    /* Grass around oasis (vegetation) */
    for (int x = 11; x <= 19; x++) {
        map->tiles[7][x]  = TILE_GRASS;
        map->tiles[13][x] = TILE_GRASS;
    }
    for (int y = 7; y <= 13; y++) {
        map->tiles[y][11] = TILE_GRASS;
        map->tiles[y][19] = TILE_GRASS;
    }

    /* Trees near oasis */
    map->tiles[7][12]  = TILE_TREE;
    map->tiles[7][18]  = TILE_TREE;
    map->tiles[13][12] = TILE_TREE;
    map->tiles[13][18] = TILE_TREE;

    /* Stone ruins scattered at edges */
    map->tiles[2][3]  = TILE_STONE;
    map->tiles[2][4]  = TILE_STONE;
    map->tiles[3][3]  = TILE_STONE;
    map->tiles[2][25] = TILE_STONE;
    map->tiles[3][25] = TILE_STONE;
    map->tiles[3][26] = TILE_STONE;
    map->tiles[15][3] = TILE_STONE;
    map->tiles[16][4] = TILE_STONE;

    /* Temple structure (north-east) */
    for (int x = 21; x <= 27; x++) {
        map->tiles[2][x] = TILE_WALL;
        map->tiles[6][x] = TILE_WALL;
    }
    for (int y = 2; y <= 6; y++) {
        map->tiles[y][21] = TILE_WALL;
        map->tiles[y][27] = TILE_WALL;
    }
    map->tiles[6][24] = TILE_DOOR; /* temple entrance */
    /* Stone floor inside temple */
    for (int y = 3; y <= 5; y++)
        for (int x = 22; x <= 26; x++)
            map->tiles[y][x] = TILE_STONE;

    /* Chest inside temple */
    map->tiles[3][24] = TILE_CHEST;

    /* Stone path from oasis to temple */
    for (int y = 4; y <= 7; y++)
        map->tiles[y][20] = TILE_STONE;

    /* Port at bottom edge */
    map->tiles[h-2][15] = TILE_PORT;
    map->tiles[h-2][14] = TILE_SAND;
    map->tiles[h-2][16] = TILE_SAND;

    map->entity_count = 0;

    /* Event triggers */
    map->trigger_count = 3;
    /* Treasure chest inside temple — item_id 8 (Sun Amulet) */
    map->triggers[0] = (EventTrigger){ 24, 3, EVENT_CHEST, 8, false };
    /* Port — warps to island 3 (Volcanic Coast) */
    map->triggers[1] = (EventTrigger){ 15, h - 2, EVENT_PORT, 3, false };
    /* Sign at temple entrance */
    map->triggers[2] = (EventTrigger){ 24, 7, EVENT_SIGN, 10, false };
}

/*
 * Island 3: Volcanic Coast (30x20)
 * Stone and sand terrain with lava flows (water tiles), rocky paths, cave entrance.
 */
static void load_island_3(MapData *map) {
    strncpy(map->name, "Volcanic Coast", MAX_MAP_NAME);
    map->width   = 30;
    map->height  = 20;
    map->spawn_x = 15;
    map->spawn_y = 18;

    int w = map->width;
    int h = map->height;

    /* Fill base with stone */
    for (int y = 0; y < MAP_MAX_H; y++)
        for (int x = 0; x < MAP_MAX_W; x++)
            map->tiles[y][x] = (x < w && y < h) ? TILE_STONE : TILE_GRASS;

    /* Water/lava border */
    for (int x = 0; x < w; x++) {
        map->tiles[0][x]   = TILE_WATER;
        map->tiles[h-1][x] = TILE_WATER;
    }
    for (int y = 0; y < h; y++) {
        map->tiles[y][0]   = TILE_WATER;
        map->tiles[y][w-1] = TILE_WATER;
    }

    /* Sand beach at bottom */
    for (int x = 1; x < w - 1; x++)
        map->tiles[h-2][x] = TILE_SAND;

    /* Lava rivers (water tiles cutting through stone) */
    /* Diagonal lava flow from upper-left */
    for (int i = 1; i <= 8; i++) {
        map->tiles[i][i + 2]     = TILE_WATER;
        map->tiles[i][i + 3]     = TILE_WATER;
    }
    /* Horizontal lava pool in center */
    for (int x = 10; x <= 20; x++) {
        map->tiles[9][x]  = TILE_WATER;
        map->tiles[10][x] = TILE_WATER;
    }

    /* Rocky path through lava (sand walkway) */
    for (int x = 1; x < w - 1; x++)
        map->tiles[8][x] = TILE_SAND;
    for (int y = 1; y < h - 2; y++)
        map->tiles[y][15] = TILE_SAND;
    /* Bridge over lava */
    map->tiles[9][15]  = TILE_SAND;
    map->tiles[10][15] = TILE_SAND;

    /* Cave entrance (north-center) */
    for (int x = 13; x <= 17; x++) {
        map->tiles[2][x] = TILE_WALL;
        map->tiles[5][x] = TILE_WALL;
    }
    for (int y = 2; y <= 5; y++) {
        map->tiles[y][13] = TILE_WALL;
        map->tiles[y][17] = TILE_WALL;
    }
    map->tiles[5][15] = TILE_DOOR; /* cave entrance */
    /* Interior */
    for (int y = 3; y <= 4; y++)
        for (int x = 14; x <= 16; x++)
            map->tiles[y][x] = TILE_STONE;

    /* Chest near lava pool */
    map->tiles[11][22] = TILE_CHEST;

    /* Port at bottom */
    map->tiles[h-2][15] = TILE_PORT;

    map->entity_count = 0;

    /* Event triggers */
    map->trigger_count = 3;
    /* Treasure chest — item_id 9 (Flame Ruby) */
    map->triggers[0] = (EventTrigger){ 22, 11, EVENT_CHEST, 9, false };
    /* Port — warps to island 4 (Frozen Peaks) */
    map->triggers[1] = (EventTrigger){ 15, h - 2, EVENT_PORT, 4, false };
    /* Boss trigger inside cave */
    map->triggers[2] = (EventTrigger){ 15, 3, EVENT_BOSS, 0, false };
}

/*
 * Island 4: Frozen Peaks (30x20)
 * Snow (stone) tiles, frozen lakes (water), pine forest, mountain village.
 */
static void load_island_4(MapData *map) {
    strncpy(map->name, "Frozen Peaks", MAX_MAP_NAME);
    map->width   = 30;
    map->height  = 20;
    map->spawn_x = 15;
    map->spawn_y = 18;

    int w = map->width;
    int h = map->height;

    /* Fill base with stone (snow-covered ground) */
    for (int y = 0; y < MAP_MAX_H; y++)
        for (int x = 0; x < MAP_MAX_W; x++)
            map->tiles[y][x] = (x < w && y < h) ? TILE_STONE : TILE_GRASS;

    /* Water border (frozen sea) */
    for (int x = 0; x < w; x++) {
        map->tiles[0][x]   = TILE_WATER;
        map->tiles[h-1][x] = TILE_WATER;
    }
    for (int y = 0; y < h; y++) {
        map->tiles[y][0]   = TILE_WATER;
        map->tiles[y][w-1] = TILE_WATER;
    }

    /* Frozen lakes */
    for (int y = 4; y <= 7; y++)
        for (int x = 3; x <= 7; x++)
            map->tiles[y][x] = TILE_WATER;

    for (int y = 12; y <= 14; y++)
        for (int x = 22; x <= 26; x++)
            map->tiles[y][x] = TILE_WATER;

    /* Pine forest clusters (trees) */
    /* Northwest cluster */
    for (int y = 2; y <= 5; y++)
        for (int x = 10; x <= 13; x++)
            map->tiles[y][x] = TILE_TREE;
    /* East cluster */
    for (int y = 3; y <= 6; y++)
        for (int x = 22; x <= 25; x++)
            map->tiles[y][x] = TILE_TREE;
    /* South cluster */
    for (int y = 14; y <= 16; y++)
        for (int x = 5; x <= 9; x++)
            map->tiles[y][x] = TILE_TREE;

    /* Sand paths through snow */
    for (int x = 1; x < w - 1; x++)
        map->tiles[10][x] = TILE_SAND;
    for (int y = 1; y < h - 1; y++)
        map->tiles[y][15] = TILE_SAND;

    /* Mountain village (center-north) */
    /* Building 1 */
    for (int x = 16; x <= 20; x++) {
        map->tiles[3][x] = TILE_WALL;
        map->tiles[6][x] = TILE_WALL;
    }
    for (int y = 3; y <= 6; y++) {
        map->tiles[y][16] = TILE_WALL;
        map->tiles[y][20] = TILE_WALL;
    }
    map->tiles[6][18] = TILE_DOOR;
    /* Interior */
    for (int y = 4; y <= 5; y++)
        for (int x = 17; x <= 19; x++)
            map->tiles[y][x] = TILE_STONE;

    /* Building 2 */
    for (int x = 16; x <= 20; x++) {
        map->tiles[7][x] = TILE_WALL;
        map->tiles[9][x] = TILE_WALL;
    }
    for (int y = 7; y <= 9; y++) {
        map->tiles[y][16] = TILE_WALL;
        map->tiles[y][20] = TILE_WALL;
    }
    map->tiles[9][18] = TILE_DOOR;

    /* Chest in building 1 */
    map->tiles[4][18] = TILE_CHEST;

    /* Port at bottom */
    map->tiles[h-2][15] = TILE_PORT;
    map->tiles[h-2][14] = TILE_SAND;
    map->tiles[h-2][16] = TILE_SAND;

    map->entity_count = 0;

    /* Event triggers */
    map->trigger_count = 3;
    /* Treasure chest — item_id 10 (Frost Crystal) */
    map->triggers[0] = (EventTrigger){ 18, 4, EVENT_CHEST, 10, false };
    /* Port — warps to island 5 (Sunken Ruins) */
    map->triggers[1] = (EventTrigger){ 15, h - 2, EVENT_PORT, 5, false };
    /* Sign at village entrance */
    map->triggers[2] = (EventTrigger){ 18, 10, EVENT_SIGN, 11, false };
}

/*
 * Island 5: Sunken Ruins (30x20)
 * Mostly water with stone bridges/paths connecting ancient structures.
 */
static void load_island_5(MapData *map) {
    strncpy(map->name, "Sunken Ruins", MAX_MAP_NAME);
    map->width   = 30;
    map->height  = 20;
    map->spawn_x = 15;
    map->spawn_y = 18;

    int w = map->width;
    int h = map->height;

    /* Fill with water (underwater ruins) */
    for (int y = 0; y < MAP_MAX_H; y++)
        for (int x = 0; x < MAP_MAX_W; x++)
            map->tiles[y][x] = (x < w && y < h) ? TILE_WATER : TILE_GRASS;

    /* Stone platform at spawn (south) */
    for (int y = 16; y <= 18; y++)
        for (int x = 12; x <= 18; x++)
            map->tiles[y][x] = TILE_STONE;

    /* Main stone bridge — north-south */
    for (int y = 2; y <= 18; y++)
        map->tiles[y][15] = TILE_STONE;

    /* East-west bridges */
    for (int x = 5; x <= 25; x++)
        map->tiles[6][x] = TILE_STONE;
    for (int x = 8; x <= 22; x++)
        map->tiles[12][x] = TILE_STONE;

    /* Western ruin platform */
    for (int y = 4; y <= 8; y++)
        for (int x = 3; x <= 7; x++)
            map->tiles[y][x] = TILE_STONE;

    /* Eastern ruin platform */
    for (int y = 10; y <= 14; y++)
        for (int x = 22; x <= 27; x++)
            map->tiles[y][x] = TILE_STONE;

    /* Ancient structure (center-north) — walls and door */
    for (int x = 12; x <= 18; x++) {
        map->tiles[2][x] = TILE_WALL;
        map->tiles[5][x] = TILE_WALL;
    }
    for (int y = 2; y <= 5; y++) {
        map->tiles[y][12] = TILE_WALL;
        map->tiles[y][18] = TILE_WALL;
    }
    map->tiles[5][15] = TILE_DOOR;
    /* Interior */
    for (int y = 3; y <= 4; y++)
        for (int x = 13; x <= 17; x++)
            map->tiles[y][x] = TILE_STONE;

    /* Ruin walls on western platform */
    map->tiles[4][3] = TILE_WALL;
    map->tiles[4][7] = TILE_WALL;
    map->tiles[8][3] = TILE_WALL;
    map->tiles[8][7] = TILE_WALL;

    /* Ruin walls on eastern platform */
    map->tiles[10][22] = TILE_WALL;
    map->tiles[10][27] = TILE_WALL;
    map->tiles[14][22] = TILE_WALL;
    map->tiles[14][27] = TILE_WALL;

    /* Chest in ancient structure */
    map->tiles[3][15] = TILE_CHEST;

    /* Port on south platform */
    map->tiles[17][15] = TILE_PORT;

    map->entity_count = 0;

    /* Event triggers */
    map->trigger_count = 3;
    /* Treasure chest — item_id 11 (Abyssal Pearl) */
    map->triggers[0] = (EventTrigger){ 15, 3, EVENT_CHEST, 11, false };
    /* Port — warps to island 6 (Sky Temple) */
    map->triggers[1] = (EventTrigger){ 15, 17, EVENT_PORT, 6, false };
    /* Boss trigger on eastern platform */
    map->triggers[2] = (EventTrigger){ 24, 12, EVENT_BOSS, 1, false };
}

/*
 * Island 6: Sky Temple (30x20)
 * Final island — floating stone platform surrounded by void (water).
 * Grand temple with final boss room.
 */
static void load_island_6(MapData *map) {
    strncpy(map->name, "Sky Temple", MAX_MAP_NAME);
    map->width   = 30;
    map->height  = 20;
    map->spawn_x = 15;
    map->spawn_y = 17;

    int w = map->width;
    int h = map->height;

    /* Fill with water (void / sky) */
    for (int y = 0; y < MAP_MAX_H; y++)
        for (int x = 0; x < MAP_MAX_W; x++)
            map->tiles[y][x] = (x < w && y < h) ? TILE_WATER : TILE_GRASS;

    /* Main floating stone platform */
    for (int y = 3; y <= 17; y++)
        for (int x = 5; x <= 24; x++)
            map->tiles[y][x] = TILE_STONE;

    /* Decorative sand border on platform */
    for (int x = 5; x <= 24; x++) {
        map->tiles[3][x]  = TILE_SAND;
        map->tiles[17][x] = TILE_SAND;
    }
    for (int y = 3; y <= 17; y++) {
        map->tiles[y][5]  = TILE_SAND;
        map->tiles[y][24] = TILE_SAND;
    }

    /* Grand temple outer walls */
    for (int x = 9; x <= 21; x++) {
        map->tiles[4][x]  = TILE_WALL;
        map->tiles[14][x] = TILE_WALL;
    }
    for (int y = 4; y <= 14; y++) {
        map->tiles[y][9]  = TILE_WALL;
        map->tiles[y][21] = TILE_WALL;
    }
    /* Temple entrance (south wall) */
    map->tiles[14][15] = TILE_DOOR;

    /* Inner hall */
    for (int y = 5; y <= 13; y++)
        for (int x = 10; x <= 20; x++)
            map->tiles[y][x] = TILE_STONE;

    /* Inner sanctum walls (boss room — north section) */
    for (int x = 12; x <= 18; x++)
        map->tiles[7][x] = TILE_WALL;
    for (int y = 5; y <= 7; y++) {
        map->tiles[y][12] = TILE_WALL;
        map->tiles[y][18] = TILE_WALL;
    }
    map->tiles[7][15] = TILE_DOOR; /* boss room entrance */

    /* Boss room interior */
    for (int y = 5; y <= 6; y++)
        for (int x = 13; x <= 17; x++)
            map->tiles[y][x] = TILE_STONE;

    /* Pillar decorations in hall */
    map->tiles[9][11]  = TILE_WALL;
    map->tiles[9][19]  = TILE_WALL;
    map->tiles[12][11] = TILE_WALL;
    map->tiles[12][19] = TILE_WALL;

    /* Stone path from entrance to spawn */
    for (int y = 15; y <= 16; y++)
        for (int x = 13; x <= 17; x++)
            map->tiles[y][x] = TILE_STONE;

    /* Final treasure chest in boss room */
    map->tiles[5][15] = TILE_CHEST;

    map->entity_count = 0;

    /* Event triggers — no port on final island */
    map->trigger_count = 3;
    /* Final treasure chest — item_id 12 (Sky Scepter) */
    map->triggers[0] = (EventTrigger){ 15, 5, EVENT_CHEST, 12, false };
    /* Boss trigger in boss room */
    map->triggers[1] = (EventTrigger){ 15, 6, EVENT_BOSS, 2, false };
    /* Sign at temple entrance */
    map->triggers[2] = (EventTrigger){ 15, 15, EVENT_SIGN, 12, false };
}

/* ── Public Functions ─────────────────────────────────── */

void map_init(MapData *map) {
    memset(map, 0, sizeof(MapData));
}

bool map_load(MapData *map, uint8_t map_id) {
    map_init(map);

    switch (map_id) {
        case 0:  load_island_0(map); return true;
        case 1:  load_island_1(map); return true;
        case 2:  load_island_2(map); return true;
        case 3:  load_island_3(map); return true;
        case 4:  load_island_4(map); return true;
        case 5:  load_island_5(map); return true;
        case 6:  load_island_6(map); return true;
        default: return false;
    }
}

TileType map_get_tile(const MapData *map, int16_t x, int16_t y) {
    if (x < 0 || x >= map->width || y < 0 || y >= map->height) {
        return TILE_WATER;  /* out of bounds = impassable */
    }
    return (TileType)map->tiles[y][x];
}

bool map_is_walkable(const MapData *map, int16_t x, int16_t y) {
    TileType tile = map_get_tile(map, x, y);
    switch (tile) {
        case TILE_WATER:
        case TILE_WALL:
        case TILE_TREE:
            return false;
        default:
            return true;
    }
}

void map_draw(const MapData *map, int16_t cam_x, int16_t cam_y) {
    /* Calculate visible tile range */
    int start_tx = cam_x / TILE_SIZE;
    int start_ty = cam_y / TILE_SIZE;
    int end_tx   = start_tx + (SCREEN_W / TILE_SIZE) + 1;
    int end_ty   = start_ty + (SCREEN_H / TILE_SIZE) + 1;

    /* Clamp to map bounds */
    if (start_tx < 0) start_tx = 0;
    if (start_ty < 0) start_ty = 0;
    if (end_tx > map->width)  end_tx = map->width;
    if (end_ty > map->height) end_ty = map->height;

    for (int ty = start_ty; ty < end_ty; ty++) {
        for (int tx = start_tx; tx < end_tx; tx++) {
            TileType tile = (TileType)map->tiles[ty][tx];
            int screen_x = tx * TILE_SIZE - cam_x;
            int screen_y = ty * TILE_SIZE - cam_y;

            draw_tile_pattern(screen_x, screen_y, tile, tx, ty);
        }
    }
}

EventTrigger *map_check_trigger(MapData *map, int16_t x, int16_t y) {
    for (int i = 0; i < map->trigger_count; i++) {
        EventTrigger *t = &map->triggers[i];
        if (t->x == x && t->y == y && !t->triggered) {
            return t;
        }
    }
    return NULL;
}
