CC = gcc
CFLAGS = -Wall -Wextra -Os -ffunction-sections -fdata-sections
LDFLAGS = -lm -Wl,--gc-sections
# Strip symbols to reduce size
LDFLAGS += -s

# Platform detection
OS := $(shell uname -s 2>/dev/null || echo Windows_NT)

ifeq ($(OS),Windows_NT)
    DETECTED_BACKEND = WINMM
else ifeq ($(OS),Linux)
    SDL3_CHECK := $(shell pkg-config --exists sdl3 && echo yes)
    SDL2_CHECK := $(shell pkg-config --exists sdl2 && echo yes)
    SDL_CHECK  := $(shell sdl-config --version >/dev/null 2>&1 && echo yes)
    ifeq ($(SDL3_CHECK),yes)
        DETECTED_BACKEND = SDL3
    else ifeq ($(SDL2_CHECK),yes)
        DETECTED_BACKEND = SDL2
    else ifeq ($(SDL_CHECK),yes)
        DETECTED_BACKEND = SDL
    else
        DETECTED_BACKEND = ALSA
    endif
endif

AUDIO_BACKEND ?= $(DETECTED_BACKEND)
CFLAGS += -DUSE_$(AUDIO_BACKEND)

# Apply flags based on final selection
ifeq ($(AUDIO_BACKEND),WINMM)
    LDFLAGS += -lwinmm -static
else ifeq ($(AUDIO_BACKEND),ALSA)
    LDFLAGS += -lasound
else ifeq ($(AUDIO_BACKEND),SDL3)
    CFLAGS += $(shell pkg-config --cflags sdl3)
    LDFLAGS += $(shell pkg-config --libs sdl3)
else ifeq ($(AUDIO_BACKEND),SDL2)
    CFLAGS += $(shell pkg-config --cflags sdl2)
    LDFLAGS += $(shell pkg-config --libs sdl2)
else ifeq ($(AUDIO_BACKEND),SDL)
    CFLAGS += $(shell sdl-config --cflags)
    LDFLAGS += $(shell sdl-config --libs)
endif

SONG ?= scratch
all: $(SONG) mmlplay

# Compiles the song (scratch.c) and the engine (mmlplay.c) into one standalone binary
$(SONG): $(SONG).c mmlplay.c
	$(CC) $(CFLAGS) $(SONG).c mmlplay.c -o $(SONG) $(LDFLAGS)

# Compiles a standalone utility that can play MML from stdin
mmlplay: mmlplay.c
	$(CC) $(CFLAGS) -DSTANDALONE_PLAYER mmlplay.c -o mmlplay $(LDFLAGS)

clean:
	rm -f scratch mmlplay *.o