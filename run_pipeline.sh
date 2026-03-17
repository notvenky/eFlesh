#!/bin/bash
# run_pipeline.sh — generate a thimble sensor from an STL/OBJ file
#
# Usage:
#   ./run_pipeline.sh <input.stl> <output.stl> [config.yaml] [options]
#
# Options:
#   --print-id  ID   short identifier for this variant, used in the manifest (default: output stem)
#   --notes     TEXT free-text notes recorded in the manifest
#   --layout    LAYOUT  override layout: circular | stacked | rings
#
# Example:
#   ./run_pipeline.sh designs/dome_v2.stl output/v1_rings.stl
#   ./run_pipeline.sh designs/dome_v2.stl output/v1_rings.stl cfg/thimble_config.yaml \
#       --print-id v1_rings_9mm --layout rings --notes "first 2-ring test"

set -e

if [ "$#" -lt 2 ]; then
    echo "Usage: $0 <input.stl> <output.stl> [config.yaml] [--print-id ID] [--notes TEXT] [--layout LAYOUT]"
    exit 1
fi

INPUT="$1"
OUTPUT="$2"

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# third arg is config if it doesn't start with --
if [ -n "$3" ] && [[ "$3" != --* ]]; then
    CONFIG="$3"
    shift 3
else
    CONFIG="$REPO_ROOT/cfg/thimble_config.yaml"
    shift 2
fi

# parse remaining named options
PRINT_ID=""
NOTES=""
LAYOUT_OVERRIDE=""
while [[ $# -gt 0 ]]; do
    case "$1" in
        --print-id) PRINT_ID="$2"; shift 2 ;;
        --notes)    NOTES="$2";    shift 2 ;;
        --layout)   LAYOUT_OVERRIDE="$2"; shift 2 ;;
        *) echo "Unknown option: $1"; exit 1 ;;
    esac
done

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

BLENDER_EXTRA_ARGS=""
[ -n "$PRINT_ID" ]        && BLENDER_EXTRA_ARGS="$BLENDER_EXTRA_ARGS --print-id $PRINT_ID"
[ -n "$NOTES" ]           && BLENDER_EXTRA_ARGS="$BLENDER_EXTRA_ARGS --notes $NOTES"
[ -n "$LAYOUT_OVERRIDE" ] && BLENDER_EXTRA_ARGS="$BLENDER_EXTRA_ARGS --layout $LAYOUT_OVERRIDE"

blender -b -P "$THIMBLE_SCRIPT" -- \
    --input "$MESH_OBJ" \
    --output "$OUTPUT" \
    --config "$CONFIG" \
    $BLENDER_EXTRA_ARGS

echo "=== Done: $OUTPUT ==="
echo "=== Manifest: ${OUTPUT%.*}.manifest.json ==="
