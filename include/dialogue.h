/**
 * dialogue.h — Dialogue system for NPC conversations and signs
 */
#ifndef DIALOGUE_H
#define DIALOGUE_H

#include <stdint.h>
#include <stdbool.h>
#include "game.h"

/* ── Dialogue Line ────────────────────────────────────── */
typedef struct {
    char text[64];
    char speaker[MAX_NAME_LEN];
} DialogueLine;

/* ── Dialogue Script ──────────────────────────────────── */
#define MAX_DIALOGUE_LINES 8

typedef struct {
    DialogueLine lines[MAX_DIALOGUE_LINES];
    uint8_t      line_count;
    uint8_t      current_line;
} DialogueScript;

/* ── Constants ────────────────────────────────────────── */
#define MAX_DIALOGUES 32

/* ── Functions ────────────────────────────────────────── */
void dialogue_init(void);
void dialogue_start(int dialogue_id);
bool dialogue_update(void);    /* returns true if still active */
void dialogue_render(void);
bool dialogue_is_active(void);

#endif /* DIALOGUE_H */
