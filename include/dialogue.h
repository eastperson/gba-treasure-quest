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
    bool has_choice;        /* true = show Yes/No choice after this line */
    uint8_t yes_next;       /* dialogue_id to jump to on Yes */
    uint8_t no_next;        /* dialogue_id to jump to on No */
    uint8_t quest_flag_set; /* quest flag index to set on this line (0xFF=none) */
} DialogueLine;

/* ── Dialogue Script ──────────────────────────────────── */
#define MAX_DIALOGUE_LINES 8

typedef struct {
    DialogueLine lines[MAX_DIALOGUE_LINES];
    uint8_t      line_count;
    uint8_t      current_line;
    uint8_t      require_flag;  /* quest flag that must be set (0xFF=none) */
    uint8_t      alt_dialogue;  /* dialogue to show if flag IS set (0xFF=none) */
} DialogueScript;

/* ── Constants ────────────────────────────────────────── */
#define MAX_DIALOGUES 32

/* ── Functions ────────────────────────────────────────── */
void dialogue_init(void);
void dialogue_start(int dialogue_id);
void dialogue_start_with_flags(int dialogue_id, uint8_t *quest_flags);
bool dialogue_update(void);    /* returns true if still active */
void dialogue_render(void);
bool dialogue_is_active(void);
int  dialogue_get_choice_result(void); /* -1=no choice, 0=Yes, 1=No */

#endif /* DIALOGUE_H */
