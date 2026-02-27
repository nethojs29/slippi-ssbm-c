#!/bin/bash
# Build script for Rotation Lobby Scene
# Compiles RotationLobby_Major.c (mjFunction) and RotationLobby_Minor.c (mnFunction)
# into RotationLobby.dat
#
# Run from slippi-ssbm-c/Scenes/Rotation/

set -e

TK_PATH="../../m-ex/MexTK/"
COMPILER_PATH="$TK_PATH/MexTK.exe"
LINK_PATH="../../melee.link"
BUILD_PATH="../../build/"
OUTPUT_PATH="../../output/"

mkdir -p "$OUTPUT_PATH"
mkdir -p "$BUILD_PATH"

echo "Building Rotation Lobby Scene..."

# Compile major scene (mjFunction) — creates .dat
$COMPILER_PATH -ff \
  -i "RotationLobby_Major.c" \
  -s mjFunction \
  -t "$TK_PATH/mjFunction.txt" \
  -l "$LINK_PATH" \
  -b $BUILD_PATH \
  -o "$OUTPUT_PATH/RotationLobby.dat" \
  -ow -c

# Compile minor scene (mnFunction) into same .dat
$COMPILER_PATH -ff \
  -i "RotationLobby_Minor.c" "../../Components/StockIcon.c" "../../Game/Characters.c" \
  -s mnFunction \
  -t "$TK_PATH/mnFunction.txt" \
  -l "$LINK_PATH" \
  -b $BUILD_PATH \
  -o "$OUTPUT_PATH/RotationLobby.dat" \
  -c

# Trim unused data from .dat
$COMPILER_PATH -trim "$OUTPUT_PATH/RotationLobby.dat"

echo "Built RotationLobby.dat successfully"
