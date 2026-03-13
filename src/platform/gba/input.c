/**
 * input.c — GBA key input (libtonc)
 *
 * libtonc's KEY_A..KEY_L macros use the same bit positions as the
 * GBA hardware KEYINPUT register (A=bit0 .. L=bit9).  Our platform.h
 * KeyMask enum uses identical values, so key_curr_state() / key_hit()
 * return values that map 1:1 to what the core engine expects.
 */
#include "platform.h"

#ifdef PLATFORM_GBA

#include <tonc.h>

void platform_poll_input(void) {
    key_poll();
}

uint16_t platform_keys_held(void) {
    /*
     * key_curr_state() returns a bitmask with the same bit layout
     * as our KeyMask constants (both follow GBA hardware ordering).
     * No remapping needed.
     */
    return (uint16_t)key_curr_state();
}

uint16_t platform_keys_pressed(void) {
    /*
     * key_hit() returns keys that transitioned from released to pressed
     * this frame.  Same bit layout as KeyMask.
     */
    return (uint16_t)key_hit(KEY_FULL);
}

#endif /* PLATFORM_GBA */
