/**
 * ui.c — UI Framework (Window/Cursor/Selection)
 * Platform-independent: uses platform.h for rendering and input.
 */
#include "ui.h"
#include "platform.h"

/* ── Colors ───────────────────────────────────────────── */
#define UI_COLOR_FILL   0x1842  /* dark blue fill */
#define UI_COLOR_BORDER 0x4A52  /* lighter blue border */
#define UI_COLOR_TEXT   0x7FFF  /* white text */
#define UI_COLOR_BAR_BG 0x2108  /* dark gray bar background */

/* ── Window Drawing ───────────────────────────────────── */
void ui_draw_window(int x, int y, int w, int h) {
    /* Fill interior */
    platform_draw_rect(x + 1, y + 1, w - 2, h - 2, UI_COLOR_FILL);

    /* Top and bottom borders */
    platform_draw_rect(x, y, w, 1, UI_COLOR_BORDER);
    platform_draw_rect(x, y + h - 1, w, 1, UI_COLOR_BORDER);

    /* Left and right borders */
    platform_draw_rect(x, y, 1, h, UI_COLOR_BORDER);
    platform_draw_rect(x + w - 1, y, 1, h, UI_COLOR_BORDER);
}

/* ── Cursor Drawing ───────────────────────────────────── */
void ui_draw_cursor(int x, int y) {
    platform_draw_text(x, y, ">", UI_COLOR_TEXT);
}

/* ── Selection Init ───────────────────────────────────── */
void ui_selection_init(UISelection *sel, int count) {
    sel->count     = (uint8_t)(count > UI_MAX_OPTIONS ? UI_MAX_OPTIONS : count);
    sel->cursor    = 0;
    sel->confirmed = false;
    sel->cancelled = false;
}

/* ── Selection Update ─────────────────────────────────── */
void ui_selection_update(UISelection *sel) {
    uint16_t keys = platform_keys_pressed();

    sel->confirmed = false;
    sel->cancelled = false;

    if (keys & KEY_UP) {
        if (sel->cursor == 0) {
            sel->cursor = sel->count - 1;  /* wrap to bottom */
        } else {
            sel->cursor--;
        }
    }
    if (keys & KEY_DOWN) {
        sel->cursor++;
        if (sel->cursor >= sel->count) {
            sel->cursor = 0;  /* wrap to top */
        }
    }
    if (keys & KEY_A) {
        sel->confirmed = true;
    }
    if (keys & KEY_B) {
        sel->cancelled = true;
    }
}

/* ── Selection Render ─────────────────────────────────── */
void ui_selection_render(UISelection *sel) {
    int line_h = 12;  /* pixels per option line */
    int pad    = 8;    /* padding inside window */
    int win_w  = 80;
    int win_h  = pad * 2 + sel->count * line_h;

    /* Draw background window */
    ui_draw_window(sel->x, sel->y, win_w, win_h);

    /* Draw each option with cursor */
    for (int i = 0; i < sel->count; i++) {
        int tx = sel->x + pad + 10;  /* leave room for cursor */
        int ty = sel->y + pad + i * line_h;

        /* Draw cursor indicator next to selected item */
        if (i == sel->cursor) {
            ui_draw_cursor(sel->x + pad, ty);
        }

        platform_draw_text(tx, ty, sel->options[i], UI_COLOR_TEXT);
    }
}

/* ── HP Bar Drawing ───────────────────────────────────── */
void ui_draw_hp_bar(int x, int y, int w, int current, int max, uint16_t color) {
    /* Draw dark background bar */
    platform_draw_rect(x, y, w, 4, UI_COLOR_BAR_BG);

    /* Draw filled portion proportional to current/max */
    if (max > 0 && current > 0) {
        int fill_w = (current * w) / max;
        if (fill_w < 1) fill_w = 1;
        if (fill_w > w) fill_w = w;
        platform_draw_rect(x, y, fill_w, 4, color);
    }
}
