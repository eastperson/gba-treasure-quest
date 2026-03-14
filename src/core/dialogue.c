/**
 * dialogue.c — Dialogue system implementation
 * Platform-independent: uses platform.h functions only.
 */
#include "dialogue.h"
#include "platform.h"
#include <string.h>

/* ── State ─────────────────────────────────────────────── */
static DialogueScript g_current;
static bool           g_active;
static int            g_choice_result; /* -1=no choice made, 0=Yes, 1=No */
static uint8_t        g_choice_cursor; /* 0=Yes, 1=No */
static bool           g_showing_choice;
static uint8_t       *g_quest_flags;   /* pointer to game's quest flags */

/* ── Dialogue Database ─────────────────────────────────── */
static const DialogueScript g_dialogues[MAX_DIALOGUES] = {
    /* 0: Villager greeting */
    [0] = {
        .lines = {
            { "Welcome to Port Town!", "Villager" },
            { "The seven islands hold great\ntreasures...", "Villager" },
        },
        .line_count = 2,
    },
    /* 1: Shopkeeper */
    [1] = {
        .lines = {
            { "Want to buy something?", "Shopkeeper" },
            { "Come back anytime!", "Shopkeeper" },
        },
        .line_count = 2,
    },
    /* 2: Sage */
    [2] = {
        .lines = {
            { "I sense great power in you...", "Sage" },
            { "Seek the treasures of the\nseven islands.", "Sage" },
            { "Each island hides a legendary\nartifact.", "Sage" },
        },
        .line_count = 3,
    },
    /* 3: Guard */
    [3] = {
        .lines = {
            { "The forest ahead is dangerous.", "Guard" },
            { "Be careful of wild monsters!", "Guard" },
        },
        .line_count = 2,
    },
    /* 4: Fisher */
    [4] = {
        .lines = {
            { "The sea is calm today.", "Fisher" },
            { "I heard there's treasure on\nthe far islands.", "Fisher" },
        },
        .line_count = 2,
    },
    /* 5: Old woman */
    [5] = {
        .lines = {
            { "My grandson went to the\nforest and never returned...", "Old Woman" },
            { "Please, if you find him,\ntell him to come home.", "Old Woman" },
        },
        .line_count = 2,
    },
    /* 6: Sailor */
    [6] = {
        .lines = {
            { "Ahoy! The port can take you\nto other islands.", "Sailor" },
            { "Just step on the dock and\nyou'll set sail!", "Sailor" },
        },
        .line_count = 2,
    },
    /* 7: Mysterious traveler */
    [7] = {
        .lines = {
            { "I've traveled all seven\nislands...", "Traveler" },
            { "The final treasure lies\nbeyond the volcano.", "Traveler" },
            { "But beware the guardian that\nprotects it.", "Traveler" },
        },
        .line_count = 3,
    },
    /* 8: Sign post */
    [8] = {
        .lines = {
            { "Port Town - Founded 100\nyears ago.", "Sign" },
        },
        .line_count = 1,
    },
    /* 9: Warning sign */
    [9] = {
        .lines = {
            { "WARNING: Monsters ahead!\nProceed with caution.", "Sign" },
        },
        .line_count = 1,
    },
    /* 10: Temple entrance sign (Desert) */
    [10] = {
        .lines = {
            { "Ancient Temple of the Sun.\nOnly the worthy may enter.", "Sign" },
        },
        .line_count = 1,
    },
    /* 11: Village entrance sign (Frozen Peaks) */
    [11] = {
        .lines = {
            { "Frost Haven Village.\nBeware of thin ice!", "Sign" },
        },
        .line_count = 1,
    },
    /* 12: Sky Temple entrance sign */
    [12] = {
        .lines = {
            { "The Sky Temple awaits.\nFinal trial lies within.", "Sign", false, 0, 0, 0xFF },
        },
        .line_count = 1,
    },
    /* 13: Sage treasure hint quest (with choice) */
    [13] = {
        .lines = {
            { "I know where a treasure\nlies hidden...", "Sage", false, 0, 0, 0xFF },
            { "Shall I share the secret\nwith you?", "Sage", true, 14, 15, 0xFF },
        },
        .line_count = 2,
    },
    /* 14: Sage yes response — gives hint + sets flag 0 */
    [14] = {
        .lines = {
            { "The Coral Crown rests in\nthe town's old building.", "Sage", false, 0, 0, 0 },
            { "Look for the chest inside\nthe northern house!", "Sage", false, 0, 0, 0xFF },
        },
        .line_count = 2,
    },
    /* 15: Sage no response */
    [15] = {
        .lines = {
            { "Very well. Come back if\nyou change your mind.", "Sage", false, 0, 0, 0xFF },
        },
        .line_count = 1,
    },
    /* 20: Oasis Trader (Island 2 shopkeeper) */
    [20] = {
        .lines = {
            { "Welcome, traveler! The desert\nis harsh without supplies.", "Oasis Trader" },
            { "Take a look at my wares.\nBest prices in the desert!", "Oasis Trader" },
        },
        .line_count = 2,
    },
    /* 21: Nomad (Island 2 villager) */
    [21] = {
        .lines = {
            { "The oasis is the only green\nspot in this endless sand.", "Nomad" },
            { "I heard the ancient temple\nholds the Sun Amulet.", "Nomad" },
        },
        .line_count = 2,
    },
    /* 22: Temple Sage (Island 2 sage) */
    [22] = {
        .lines = {
            { "This temple was built by an\nancient sun-worshipping clan.", "Temple Sage" },
            { "The Sun Amulet radiates with\nthe power of the desert.", "Temple Sage" },
            { "Beware the scorpions that\nguard these ruins.", "Temple Sage" },
        },
        .line_count = 3,
    },
    /* 23: Fire Warden (Island 3 guard) */
    [23] = {
        .lines = {
            { "The volcano has been restless\nlately...", "Fire Warden" },
            { "A Fire Dragon nests in the\ncave up north. Stay away!", "Fire Warden" },
        },
        .line_count = 2,
    },
    /* 24: Lava Smith (Island 3 shopkeeper) */
    [24] = {
        .lines = {
            { "I forge weapons in volcanic\nheat! Need something?", "Lava Smith" },
            { "Bombs are great against\nfire creatures, ironically.", "Lava Smith" },
        },
        .line_count = 2,
    },
    /* 25: Mountaineer (Island 4 villager) */
    [25] = {
        .lines = {
            { "Brrr! These peaks are the\ncoldest place I've ever been.", "Mountaineer" },
            { "The Frost Crystal is hidden\nsomewhere in the village.", "Mountaineer" },
        },
        .line_count = 2,
    },
    /* 26: Ice Merchant (Island 4 shopkeeper) */
    [26] = {
        .lines = {
            { "Stock up before venturing\ninto the frozen wilderness!", "Ice Merchant" },
            { "Hi-Potions are a must for\nthe dangers ahead.", "Ice Merchant" },
        },
        .line_count = 2,
    },
    /* 27: Frost Sage (Island 4 sage) */
    [27] = {
        .lines = {
            { "The ice here never melts.\nIt holds ancient magic.", "Frost Sage" },
            { "Beyond these peaks lie the\nSunken Ruins of old.", "Frost Sage" },
            { "You'll need all your strength\nfor what lies ahead.", "Frost Sage" },
        },
        .line_count = 3,
    },
    /* 28: Ruin Scholar (Island 5 sage) */
    [28] = {
        .lines = {
            { "These ruins sank beneath\nthe waves centuries ago.", "Ruin Scholar" },
            { "The Abyssal Pearl rests in\nthe ancient chamber above.", "Ruin Scholar" },
            { "The Kraken guards the\neastern platform. Be wary!", "Ruin Scholar" },
        },
        .line_count = 3,
    },
    /* 29: Ruin Guard (Island 5 guard) */
    [29] = {
        .lines = {
            { "I've stood watch here for\nyears. The ruins shift.", "Ruin Guard" },
            { "Don't fall into the water!\nThe currents are deadly.", "Ruin Guard" },
        },
        .line_count = 2,
    },
    /* 30: Sky Oracle (Island 6 sage) */
    [30] = {
        .lines = {
            { "You've come far, hero.\nThe final trial awaits.", "Sky Oracle" },
            { "The Sky Lord guards the\nlast treasure.", "Sky Oracle" },
            { "Gather your courage. Only\nthe brave will prevail.", "Sky Oracle" },
        },
        .line_count = 3,
    },
    /* 31: Temple Knight (Island 6 guard) */
    [31] = {
        .lines = {
            { "None have defeated the\nSky Lord. Will you try?", "Temple Knight" },
            { "The inner sanctum lies\nbeyond this hall.", "Temple Knight" },
        },
        .line_count = 2,
    },
};

/* ── Public Functions ──────────────────────────────────── */

void dialogue_init(void) {
    memset(&g_current, 0, sizeof(g_current));
    g_active = false;
    g_choice_result = -1;
    g_choice_cursor = 0;
    g_showing_choice = false;
    g_quest_flags = NULL;
}

void dialogue_start(int dialogue_id) {
    dialogue_start_with_flags(dialogue_id, NULL);
}

void dialogue_start_with_flags(int dialogue_id, uint8_t *quest_flags) {
    if (dialogue_id < 0 || dialogue_id >= MAX_DIALOGUES) return;
    if (g_dialogues[dialogue_id].line_count == 0) return;

    g_quest_flags = quest_flags;
    g_current = g_dialogues[dialogue_id];
    g_current.current_line = 0;
    g_active = true;
    g_choice_result = -1;
    g_choice_cursor = 0;
    g_showing_choice = false;
}

int dialogue_get_choice_result(void) {
    return g_choice_result;
}

bool dialogue_update(void) {
    if (!g_active) return false;

    uint16_t keys = platform_keys_pressed();

    /* If showing a choice, handle choice input */
    if (g_showing_choice) {
        if (keys & KEY_UP) { g_choice_cursor = 0; }
        if (keys & KEY_DOWN) { g_choice_cursor = 1; }
        if (keys & KEY_A) {
            const DialogueLine *line = &g_current.lines[g_current.current_line];
            g_choice_result = g_choice_cursor;
            g_showing_choice = false;

            /* Jump to the appropriate follow-up dialogue */
            uint8_t next_id = (g_choice_cursor == 0) ? line->yes_next : line->no_next;
            if (next_id < MAX_DIALOGUES && g_dialogues[next_id].line_count > 0) {
                g_current = g_dialogues[next_id];
                g_current.current_line = 0;
            } else {
                g_active = false;
                return false;
            }
        }
        if (keys & KEY_B) {
            /* B = No */
            const DialogueLine *line = &g_current.lines[g_current.current_line];
            g_choice_result = 1;
            g_showing_choice = false;

            uint8_t next_id = line->no_next;
            if (next_id < MAX_DIALOGUES && g_dialogues[next_id].line_count > 0) {
                g_current = g_dialogues[next_id];
                g_current.current_line = 0;
            } else {
                g_active = false;
                return false;
            }
        }
        return true;
    }

    if (keys & KEY_A) {
        /* Set quest flag if this line has one */
        const DialogueLine *cur = &g_current.lines[g_current.current_line];
        if (cur->quest_flag_set != 0xFF && g_quest_flags) {
            if (cur->quest_flag_set < 8) {
                g_quest_flags[cur->quest_flag_set] = 1;
            }
        }

        /* Check if current line has a choice */
        if (cur->has_choice) {
            g_showing_choice = true;
            g_choice_cursor = 0;
            return true;
        }

        g_current.current_line++;
        if (g_current.current_line >= g_current.line_count) {
            g_active = false;
            return false;
        }
    }

    /* B button also closes dialogue */
    if (keys & KEY_B) {
        g_active = false;
        return false;
    }

    return true;
}

void dialogue_render(void) {
    if (!g_active) return;

    const DialogueLine *line = &g_current.lines[g_current.current_line];

    /* Dark blue text box at bottom of screen: 224x40 at (8, 112) */
    /* BGR555: dark blue = 0x5000 (approx) */
    platform_draw_rect(8, 112, 224, 40, 0x5000);

    /* Border (slightly lighter) */
    platform_draw_rect(8, 112, 224, 1, 0x6318);  /* top */
    platform_draw_rect(8, 151, 224, 1, 0x6318);  /* bottom */
    platform_draw_rect(8, 112, 1, 40, 0x6318);   /* left */
    platform_draw_rect(231, 112, 1, 40, 0x6318); /* right */

    /* Speaker name in yellow (BGR555: 0x03FF) */
    platform_draw_text(14, 115, line->speaker, 0x03FF);

    /* Text in white */
    platform_draw_text(14, 126, line->text, 0x7FFF);

    /* Show Yes/No choice if applicable */
    if (g_showing_choice) {
        platform_draw_rect(180, 96, 50, 24, 0x5000);
        platform_draw_rect(180, 96, 50, 1, 0x6318);
        platform_draw_rect(180, 119, 50, 1, 0x6318);
        platform_draw_rect(180, 96, 1, 24, 0x6318);
        platform_draw_rect(229, 96, 1, 24, 0x6318);

        platform_draw_text(196, 100, "Yes", g_choice_cursor == 0 ? 0x03FF : 0x5294);
        platform_draw_text(196, 112, "No",  g_choice_cursor == 1 ? 0x03FF : 0x5294);
        platform_draw_text(186, g_choice_cursor == 0 ? 100 : 112, ">", 0x03FF);
    } else {
        /* "v" indicator if more lines remain */
        if (g_current.current_line < g_current.line_count - 1) {
            platform_draw_text(220, 144, "v", 0x7FFF);
        }
    }
}

bool dialogue_is_active(void) {
    return g_active;
}
