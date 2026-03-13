/**
 * sprite.c — Sprite animation system implementation
 * Platform-independent: no GBA or SDL headers here.
 */
#include "sprite.h"
#include <string.h>

/* ── Initialization ───────────────────────────────────── */
void sprite_anim_init(SpriteAnim *sa) {
    memset(sa, 0, sizeof(SpriteAnim));
    sa->current_dir = 0;
    sa->playing = false;

    /*
     * Set up 4-direction walk animations (2 frames each, 8 frame duration).
     * Sprite IDs are laid out as:
     *   dir 0 (down):  0, 1
     *   dir 1 (up):    2, 3
     *   dir 2 (left):  4, 5
     *   dir 3 (right): 6, 7
     */
    for (int dir = 0; dir < 4; dir++) {
        sa->anims[dir].frame_count   = 2;
        sa->anims[dir].current_frame = 0;
        sa->anims[dir].timer         = 0;
        sa->anims[dir].looping       = true;

        sa->anims[dir].frames[0].sprite_id = (uint8_t)(dir * 2);
        sa->anims[dir].frames[0].duration  = 8;
        sa->anims[dir].frames[1].sprite_id = (uint8_t)(dir * 2 + 1);
        sa->anims[dir].frames[1].duration  = 8;
    }
}

/* ── Set Direction ────────────────────────────────────── */
void sprite_anim_set_dir(SpriteAnim *sa, int dir) {
    if (dir < 0 || dir > 3) return;
    if (sa->current_dir != (uint8_t)dir) {
        sa->current_dir = (uint8_t)dir;
        /* Reset animation when changing direction */
        sa->anims[dir].current_frame = 0;
        sa->anims[dir].timer = 0;
    }
}

/* ── Update ───────────────────────────────────────────── */
void sprite_anim_update(SpriteAnim *sa) {
    if (!sa->playing) return;

    Animation *anim = &sa->anims[sa->current_dir];
    anim->timer++;

    if (anim->timer >= anim->frames[anim->current_frame].duration) {
        anim->timer = 0;
        anim->current_frame++;

        if (anim->current_frame >= anim->frame_count) {
            if (anim->looping) {
                anim->current_frame = 0;
            } else {
                anim->current_frame = anim->frame_count - 1;
            }
        }
    }
}

/* ── Get Current Frame ────────────────────────────────── */
int sprite_anim_get_frame(SpriteAnim *sa) {
    Animation *anim = &sa->anims[sa->current_dir];

    /* Idle = frame 0 of current direction */
    if (!sa->playing) {
        return (int)anim->frames[0].sprite_id;
    }

    return (int)anim->frames[anim->current_frame].sprite_id;
}

/* ── Play / Stop ──────────────────────────────────────── */
void sprite_anim_play(SpriteAnim *sa) {
    sa->playing = true;
}

void sprite_anim_stop(SpriteAnim *sa) {
    sa->playing = false;
    /* Reset to frame 0 (idle) */
    Animation *anim = &sa->anims[sa->current_dir];
    anim->current_frame = 0;
    anim->timer = 0;
}
