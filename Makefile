# Treasure Quest: Seven Islands — Triple Target Makefile
# GBA ROM (devkitARM) + PC/Steam (SDL2) + Web/Mobile (Emscripten)

# ============================================================
# Common
# ============================================================
PROJECT  := treasure_quest
CORE_SRC := $(wildcard src/core/*.c)
CORE_OBJ_GBA := $(CORE_SRC:src/%.c=build/gba/%.o)
CORE_OBJ_PC  := $(CORE_SRC:src/%.c=build/pc/%.o)
CORE_OBJ_WEB := $(CORE_SRC:src/%.c=build/web/%.o)

INCLUDES := -Iinclude

# ============================================================
# GBA Target (devkitARM)
# ============================================================
ifeq ($(strip $(DEVKITARM)),)
  DEVKITARM := /opt/devkitpro/devkitARM
endif

GBA_PREFIX := $(DEVKITARM)/bin/arm-none-eabi-
GBA_CC     := $(GBA_PREFIX)gcc
GBA_LD     := $(GBA_PREFIX)gcc
GBA_OBJCOPY:= $(GBA_PREFIX)objcopy

GBA_CFLAGS := -mthumb -mthumb-interwork -mcpu=arm7tdmi \
              -O2 -Wall -fno-strict-aliasing \
              $(INCLUDES) -DPLATFORM_GBA \
              -I$(DEVKITARM)/../libtonc/include

GBA_LDFLAGS := -mthumb -mthumb-interwork -specs=gba.specs \
               -L$(DEVKITARM)/../libtonc/lib -ltonc

GBA_SRC := $(wildcard src/platform/gba/*.c)
GBA_OBJ := $(GBA_SRC:src/%.c=build/gba/%.o) $(CORE_OBJ_GBA)

.PHONY: gba
gba: build/$(PROJECT).gba

build/$(PROJECT).gba: build/$(PROJECT).elf
	$(GBA_OBJCOPY) -O binary $< $@
	@echo "=== GBA ROM built: $@ ==="

build/$(PROJECT).elf: $(GBA_OBJ)
	@mkdir -p $(dir $@)
	$(GBA_LD) $(GBA_OBJ) $(GBA_LDFLAGS) -o $@

build/gba/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(GBA_CC) $(GBA_CFLAGS) -c $< -o $@

# ============================================================
# PC/Steam Target (SDL2)
# ============================================================
PC_CC     := gcc
PC_CFLAGS := -O2 -Wall $(INCLUDES) -DPLATFORM_SDL \
             $(shell sdl2-config --cflags 2>/dev/null)
PC_LDFLAGS:= $(shell sdl2-config --libs 2>/dev/null) -lSDL2_mixer -lSDL2_image

PC_SRC := $(wildcard src/platform/sdl/*.c)
PC_OBJ := $(PC_SRC:src/%.c=build/pc/%.o) $(CORE_OBJ_PC)

.PHONY: pc
pc: build/$(PROJECT)

build/$(PROJECT): $(PC_OBJ)
	@mkdir -p $(dir $@)
	$(PC_CC) $(PC_OBJ) $(PC_LDFLAGS) -o $@
	@echo "=== PC build complete: $@ ==="

build/pc/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(PC_CC) $(PC_CFLAGS) -c $< -o $@

# ============================================================
# Web/Mobile Target (Emscripten + SDL2)
# ============================================================
WEB_CC     := emcc
WEB_CFLAGS := -O2 -Wall $(INCLUDES) -DPLATFORM_SDL -DPLATFORM_WEB \
              -sUSE_SDL=2

WEB_LDFLAGS := -sUSE_SDL=2 -sALLOW_MEMORY_GROWTH=1 \
               -sEXPORTED_FUNCTIONS='["_main","_web_key_down","_web_key_up"]' \
               -sEXPORTED_RUNTIME_METHODS='["UTF8ToString","lengthBytesUTF8","stringToUTF8"]' \
               --shell-file web/shell.html \
               -sINITIAL_MEMORY=33554432

# Reuse SDL render.c + input.c; use web-specific main.c + platform_web.c
WEB_SDL_SRC := src/platform/sdl/render.c src/platform/sdl/input.c \
               src/platform/sdl/settings.c
WEB_OWN_SRC := $(wildcard src/platform/web/*.c)
WEB_SDL_OBJ := $(WEB_SDL_SRC:src/%.c=build/web/%.o)
WEB_OWN_OBJ := $(WEB_OWN_SRC:src/%.c=build/web/%.o)
WEB_OBJ     := $(WEB_SDL_OBJ) $(WEB_OWN_OBJ) $(CORE_OBJ_WEB)

.PHONY: web
web: build/$(PROJECT).html

build/$(PROJECT).html: $(WEB_OBJ)
	@mkdir -p $(dir $@)
	$(WEB_CC) $(WEB_OBJ) $(WEB_LDFLAGS) -o $@
	@echo "=== Web build complete: $@ ==="
	@echo "    Serve with: python3 -m http.server -d build/ 8080"

build/web/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(WEB_CC) $(WEB_CFLAGS) -c $< -o $@

# ============================================================
# Test Target (host-compiled, no external deps)
# ============================================================
.PHONY: test

test:
	@mkdir -p build/test
	@echo "=== Building and running tests ==="
	gcc -Iinclude -Itests -DPLATFORM_TEST -Wall -Wno-unused-function tests/test_inventory.c -o build/test/test_inventory && ./build/test/test_inventory
	gcc -Iinclude -Itests -DPLATFORM_TEST -Wall -Wno-unused-function tests/test_battle.c -o build/test/test_battle && ./build/test/test_battle
	gcc -Iinclude -Itests -DPLATFORM_TEST -Wall -Wno-unused-function tests/test_save.c -o build/test/test_save && ./build/test/test_save
	@echo "=== All tests complete ==="

# ============================================================
# Utilities
# ============================================================
.PHONY: clean all serve

all: gba pc web

clean:
	rm -rf build/
	@echo "=== Clean complete ==="

serve: web
	@echo "Serving at http://localhost:8080/$(PROJECT).html"
	python3 -m http.server -d build/ 8080
