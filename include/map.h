/**
 * map.h — Tilemap system and map data structures
 */
#ifndef MAP_H
#define MAP_H

#include <stdint.h>
#include <stdbool.h>

/* ── Tile Types ───────────────────────────────────────── */
typedef enum {
    TILE_GRASS,
    TILE_WATER,
    TILE_SAND,
    TILE_STONE,
    TILE_TREE,
    TILE_WALL,
    TILE_DOOR,
    TILE_CHEST,
    TILE_PORT,
    TILE_COUNT
} TileType;

/* ── Map Dimensions ───────────────────────────────────── */
#define MAP_MAX_W  32
#define MAP_MAX_H  32
#define MAX_MAP_NAME 16

/* ── Map Entity ───────────────────────────────────────── */
#define MAX_ENTITIES 32

typedef struct {
    uint8_t  type;
    int16_t  x;
    int16_t  y;
    uint8_t  id;
    bool     active;
} MapEntity;

/* ── Event Triggers ──────────────────────────────────── */
#define MAX_TRIGGERS 8

typedef enum {
    EVENT_CHEST,
    EVENT_PORT,
    EVENT_SIGN,
    EVENT_BOSS,
} EventType;

typedef struct {
    int16_t   x;
    int16_t   y;
    EventType type;
    uint8_t   id;        /* item_id for chests, map_id for ports, dialogue_id for signs */
    bool      triggered;
} EventTrigger;

/* ── Map Data ─────────────────────────────────────────── */
typedef struct {
    uint8_t      width;
    uint8_t      height;
    uint8_t      tiles[MAP_MAX_H][MAP_MAX_W];
    char         name[MAX_MAP_NAME];
    int16_t      spawn_x;
    int16_t      spawn_y;
    MapEntity    entities[MAX_ENTITIES];
    uint8_t      entity_count;
    EventTrigger triggers[MAX_TRIGGERS];
    uint8_t      trigger_count;
} MapData;

/* ── Functions ────────────────────────────────────────── */
void     map_init(MapData *map);
bool     map_load(MapData *map, uint8_t map_id);
TileType map_get_tile(const MapData *map, int16_t x, int16_t y);
bool     map_is_walkable(const MapData *map, int16_t x, int16_t y);
void          map_draw(const MapData *map, int16_t cam_x, int16_t cam_y);
void          map_draw_atmosphere(const MapData *map, int16_t cam_x, int16_t cam_y,
                                  uint32_t frame_count, uint8_t island_id);
EventTrigger *map_check_trigger(MapData *map, int16_t x, int16_t y);

#endif /* MAP_H */
