/**
 * audio.h — Audio System Abstraction
 */
#ifndef AUDIO_H
#define AUDIO_H

/* ── Sound Effect IDs ─────────────────────────────────── */
typedef enum {
    SFX_CURSOR,
    SFX_CONFIRM,
    SFX_CANCEL,
    SFX_HIT,
    SFX_HEAL,
    SFX_LEVELUP,
    SFX_CHEST,
    SFX_DOOR,
    SFX_COUNT,
} SFX_ID;

/* ── Background Music IDs ─────────────────────────────── */
typedef enum {
    BGM_TITLE,
    BGM_TOWN,
    BGM_FOREST,
    BGM_BATTLE,
    BGM_BOSS,
    BGM_VICTORY,
    BGM_GAMEOVER,
    BGM_COUNT,
} BGM_ID;

/* ── Functions ────────────────────────────────────────── */
void audio_play_bgm(BGM_ID id);
void audio_stop_bgm(void);
void audio_play_sfx(SFX_ID id);
void audio_set_bgm_for_island(int island_id);

#endif /* AUDIO_H */
