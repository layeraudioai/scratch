#!/bin/bash
# Interactive Song Compiler
cd "$(dirname "$0")/.."
clear
echo "=== MML Song Encoder Build System ==="

SONG="${1:-scratch}"
SONG="${SONG%.c}"

echo "Scanning for available audio libraries..."
AVAILABLE=()
if pkg-config --exists alsa 2>/dev/null; then AVAILABLE+=("ALSA"); fi
if pkg-config --exists sdl3 2>/dev/null; then AVAILABLE+=("SDL3"); fi
if pkg-config --exists sdl2 2>/dev/null; then AVAILABLE+=("SDL2"); fi
if sdl-config --version &>/dev/null; then AVAILABLE+=("SDL"); fi
if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" || "$OS" == "Windows_NT" ]]; then AVAILABLE+=("WINMM"); fi
AVAILABLE+=("DUMMY")

if [ ${#AVAILABLE[@]} -eq 0 ]; then
    echo "No audio backends detected."
    SELECTED="DUMMY"
else
    echo "Available backends for '$SONG':"
    for i in "${!AVAILABLE[@]}"; do
        echo "  [$i] ${AVAILABLE[$i]}"
    done
    read -p "Select backend [0-$((${#AVAILABLE[@]}-1))]: " choice
    choice=${choice:-0}
    SELECTED=${AVAILABLE[$choice]}
fi

# Handle compiler prefix (e.g. for cross-compiling)
CC_CMD="gcc"
if [ ! -z "$2" ]; then CC_CMD="${2}cc"; fi

echo "Building $SONG with $SELECTED backend..."
make SONG="$SONG" AUDIO_BACKEND="$SELECTED" CC="$CC_CMD"
echo "--------------------------------------"
