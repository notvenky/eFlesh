#!/bin/bash
# run_pipeline.sh — generate a thimble sensor from an STL/OBJ file
#
# Usage:
#   ./run_pipeline.sh <input.stl> <output.stl> [config.yaml]
#
# Example:
#   ./run_pipeline.sh designs/panda_ee.stl output/panda_thimble.stl
#   ./run_pipeline.sh designs/my_sensor.stl output/my_sensor.stl cfg/thimble_config.yaml

set -e

if [ "$#" -lt 2 ]; then
    echo "Usage: $0 <input.stl> <output.stl> [config.yaml]"
    exit 1
fi

INPUT="$1"
OUTPUT="$2"

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# default config; override by passing a third argument
CONFIG="${3:-$REPO_ROOT/cfg/thimble_config.yaml}"

MESH_SCRIPT="$REPO_ROOT/microstructure/microstructure_inflators/create_mesh.py"
THIMBLE_SCRIPT="$REPO_ROOT/scripts/generate_thimble.py"

# --- Step 1: generate microstructure mesh ---
echo "=== Step 1: generating mesh from $INPUT ==="

python "$MESH_SCRIPT" \
    --input "$INPUT" \
    --config "$CONFIG" \
    --microstructure-repo "$REPO_ROOT/microstructure/microstructure_inflators" \
    --matopt-repo "$REPO_ROOT/microstructure/matopt"

# Detect the generated .obj — convention: {stem}_{E}_{cell_size}.obj
INPUT_DIR="$(dirname "$INPUT")"
INPUT_STEM="$(basename "$INPUT" | sed 's/\.[^.]*$//')"
MESH_OBJ=$(ls -t "$INPUT_DIR"/${INPUT_STEM}_*.obj 2>/dev/null | head -1)

if [ -z "$MESH_OBJ" ]; then
    echo "Error: could not find generated .obj in $INPUT_DIR"
    exit 1
fi
echo "Generated mesh: $MESH_OBJ"

# --- Step 2: add caps and magnet pouches ---
echo "=== Step 2: adding caps and pouches → $OUTPUT ==="

blender -b -P "$THIMBLE_SCRIPT" -- \
    --input "$MESH_OBJ" \
    --output "$OUTPUT" \
    --config "$CONFIG"

echo "=== Done: $OUTPUT ==="
