/**
 * main.c — GBA entry point (devkitARM / libtonc)
 */
#include "game.h"
#include "platform.h"

int main(void) {
    platform_init();

    GameContext ctx;
    game_init(&ctx);

    while (ctx.running) {
        game_update(&ctx);
        game_render(&ctx);
    }

    /* GBA never truly exits */
    while (1) {}
    return 0;
}
