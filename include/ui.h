/**
 * ui.h — UI Framework (Window/Cursor/Selection)
 */
#ifndef UI_H
#define UI_H

#include <stdint.h>
#include <stdbool.h>

/* ── UI Window ────────────────────────────────────────── */
typedef struct {
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
    bool    visible;
} UIWindow;

/* ── UI Selection ─────────────────────────────────────── */
#define UI_MAX_OPTIONS   8
#define UI_MAX_OPTION_LEN 16

typedef struct {
    char    options[UI_MAX_OPTIONS][UI_MAX_OPTION_LEN];
    uint8_t count;
    uint8_t cursor;
    int16_t x;
    int16_t y;
    bool    confirmed;
    bool    cancelled;
} UISelection;

/* ── Functions ────────────────────────────────────────── */
void ui_draw_window(int x, int y, int w, int h);
void ui_draw_cursor(int x, int y);
void ui_selection_init(UISelection *sel, int count);
void ui_selection_update(UISelection *sel);
void ui_selection_render(UISelection *sel);
void ui_draw_hp_bar(int x, int y, int w, int current, int max, uint16_t color);

#endif /* UI_H */
