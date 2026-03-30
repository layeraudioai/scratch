#!/bin/bash
# Interactive Song Compiler
clear
echo "=== MML Song Encoder Build System ==="

echo "Available songs:"
SONGS=()
idx=0
for f in *.c; do
    if [[ "$f" != "mmlplay.c" ]]; then
        SONGS+=("${f%.c}")
        echo "  [$idx] ${f%.c}"
        ((idx++))
    fi
done
read -p "Select song [0-$((idx-1))] (default 0): " s_idx
s_idx=${s_idx:-0}
SONG=${SONGS[$s_idx]}

echo "Select Target Platform:"
echo "  [0] linux"
echo "  [1] windows"
echo "  [2] nintendo"
read -p "Select [0-2] (default 0): " p_idx
p_idx=${p_idx:-0}
PLATFORMS=("linux" "windows" "nintendo")
PLATFORM=${PLATFORMS[$p_idx]}

echo "Select Target Architecture:"
echo "  [0] amd64"
echo "  [1] arm"
echo "  [2] arm64"
read -p "Select [0-2] (default 0): " a_idx
a_idx=${a_idx:-0}
ARCHS=("amd64" "arm" "arm64")
ARCH=${ARCHS[$a_idx]}

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

EXT=""
if [[ "$PLATFORM" == "windows" ]]; then EXT=".exe"; fi
if [[ "$PLATFORM" == "nintendo" ]]; then
    if [[ "$ARCH" == "arm" ]]; then EXT=".cia";
    elif [[ "$ARCH" == "arm64" ]]; then EXT=".nsp";
    else EXT=".nro"; fi
fi
backend_lower=$(echo "$SELECTED" | tr '[:upper:]' '[:lower:]' 2>/dev/null || echo "$SELECTED")
OUTFILE="${PLATFORM}_${ARCH}_${backend_lower}_${SONG}${EXT}"

# Handle compiler prefix (e.g. for cross-compiling)
if [ ! -z "$2" ]; then
    CC_CMD="${2}cc"
elif [[ "$PLATFORM" == "nintendo" ]]; then
    if [[ "$ARCH" == "arm" ]]; then CC_CMD="arm-none-eabi-gcc"
    else CC_CMD="aarch64-none-elf-gcc"; fi
else
    CC_CMD="gcc"
fi

echo "Building $OUTFILE with $SELECTED backend..."
make SONG="$SONG" AUDIO_BACKEND="$SELECTED" CC="$CC_CMD" PLATFORM="$PLATFORM" ARCH="$ARCH" OUTFILE="$OUTFILE"
echo "--------------------------------------"
