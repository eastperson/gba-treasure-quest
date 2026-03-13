/**
 * sprite.h — Sprite animation system
 */
#ifndef SPRITE_H
#define SPRITE_H

#include <stdint.h>
#include <stdbool.h>

/* ── Animation Frame ──────────────────────────────────── */
typedef struct {
    uint8_t  sprite_id;
    uint8_t  duration;    /* frames */
} AnimFrame;

/* ── Single Animation (one direction) ─────────────────── */
typedef struct {
    AnimFrame frames[4];
    uint8_t   frame_count;
    uint8_t   current_frame;
    uint8_t   timer;
    bool      looping;
} Animation;

/* ── Sprite Animation (all directions) ────────────────── */
/* Direction indices: 0=down, 1=up, 2=left, 3=right */
typedef struct {
    Animation anims[4];
    uint8_t   current_dir;
    bool      playing;
} SpriteAnim;

/* ── Functions ────────────────────────────────────────── */
void sprite_anim_init(SpriteAnim *sa);
void sprite_anim_set_dir(SpriteAnim *sa, int dir);
void sprite_anim_update(SpriteAnim *sa);
int  sprite_anim_get_frame(SpriteAnim *sa);
void sprite_anim_play(SpriteAnim *sa);
void sprite_anim_stop(SpriteAnim *sa);

#endif /* SPRITE_H */
