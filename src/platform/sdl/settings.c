/**
 * settings.c — Simple settings save/load for PC build
 * Saves to "treasure_quest.cfg" in the current working directory.
 */
#include "platform.h"

#ifdef PLATFORM_SDL

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define CFG_FILENAME "treasure_quest.cfg"

void settings_load(int *scale, bool *fullscreen, int *volume) {
    /* Set defaults first */
    *scale      = 3;
    *fullscreen = false;
    *volume     = 80;

    FILE *f = fopen(CFG_FILENAME, "r");
    if (!f) return;

    char line[128];
    while (fgets(line, sizeof(line), f)) {
        char key[64];
        int  val;
        if (sscanf(line, "%63[^=]=%d", key, &val) == 2) {
            /* Trim trailing whitespace from key */
            char *end = key + strlen(key) - 1;
            while (end > key && (*end == ' ' || *end == '\t')) *end-- = '\0';

            if (strcmp(key, "window_scale") == 0) {
                if (val >= 2 && val <= 6) *scale = val;
            } else if (strcmp(key, "fullscreen") == 0) {
                *fullscreen = (val != 0);
            } else if (strcmp(key, "master_volume") == 0) {
                if (val < 0)   val = 0;
                if (val > 100) val = 100;
                *volume = val;
            }
        }
    }

    fclose(f);
}

void settings_save(int scale, bool fullscreen, int volume) {
    FILE *f = fopen(CFG_FILENAME, "w");
    if (!f) return;

    fprintf(f, "# Treasure Quest: Seven Islands — Settings\n");
    fprintf(f, "window_scale=%d\n", scale);
    fprintf(f, "fullscreen=%d\n", fullscreen ? 1 : 0);
    fprintf(f, "master_volume=%d\n", volume);

    fclose(f);
}

#endif /* PLATFORM_SDL */
