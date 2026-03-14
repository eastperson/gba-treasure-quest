/**
 * menu.h — In-game menu system
 */
#ifndef MENU_H
#define MENU_H

#include "game.h"

/* ── Menu Items ───────────────────────────────────────── */
typedef enum {
    MENU_ITEMS,
    MENU_STATUS,
    MENU_PARTY,
    MENU_SAVE,
    MENU_LOAD,
    MENU_QUIT,
    MENU_COUNT
} MenuItem;

/* MenuContext is defined in game.h (embedded in GameContext) */

/* ── Functions ────────────────────────────────────────── */
void menu_update(MenuContext *menu, GameContext *ctx);
void menu_render(MenuContext *menu, GameContext *ctx);

#endif /* MENU_H */
