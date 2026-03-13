/**
 * audio.c — Audio System Abstraction
 * Platform-independent: delegates to platform_play_bgm / platform_play_sfx.
 */
#include "audio.h"
#include "platform.h"

/* ── Track Mapping Tables ─────────────────────────────── */
/* Map BGM_ID to platform track indices */
static const int bgm_track_map[BGM_COUNT] = {
    0,  /* BGM_TITLE */
    1,  /* BGM_TOWN */
    2,  /* BGM_FOREST */
    3,  /* BGM_BATTLE */
    4,  /* BGM_BOSS */
    5,  /* BGM_VICTORY */
    6,  /* BGM_GAMEOVER */
};

/* Map SFX_ID to platform sfx indices */
static const int sfx_track_map[SFX_COUNT] = {
    0,  /* SFX_CURSOR */
    1,  /* SFX_CONFIRM */
    2,  /* SFX_CANCEL */
    3,  /* SFX_HIT */
    4,  /* SFX_HEAL */
    5,  /* SFX_LEVELUP */
    6,  /* SFX_CHEST */
    7,  /* SFX_DOOR */
};

/* ── BGM Playback ─────────────────────────────────────── */
void audio_play_bgm(BGM_ID id) {
    if (id >= 0 && id < BGM_COUNT) {
        platform_play_bgm(bgm_track_map[id]);
    }
}

void audio_stop_bgm(void) {
    platform_stop_bgm();
}

/* ── SFX Playback ─────────────────────────────────────── */
void audio_play_sfx(SFX_ID id) {
    if (id >= 0 && id < SFX_COUNT) {
        platform_play_sfx(sfx_track_map[id]);
    }
}

/* ── Island BGM Selection ─────────────────────────────── */
void audio_set_bgm_for_island(int island_id) {
    switch (island_id) {
        case 0:
            audio_play_bgm(BGM_TOWN);
            break;
        case 1:
        case 2:
        case 3:
            audio_play_bgm(BGM_FOREST);
            break;
        case 4:
        case 5:
            /* Boss theme variant — use boss track for late islands */
            audio_play_bgm(BGM_BOSS);
            break;
        case 6:
            audio_play_bgm(BGM_BOSS);
            break;
        default:
            audio_play_bgm(BGM_TOWN);
            break;
    }
}
