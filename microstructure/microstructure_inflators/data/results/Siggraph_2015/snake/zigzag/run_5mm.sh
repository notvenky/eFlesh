## Pattern param lookup and microstructure generation
$MICROSTRUCTURES_PATH/driver/preprocess_guide_mesh.py \
    --young="E" \
    --poisson="nu" \
    -o results/material_hex.msh \
    snake.msh \
    snake_hex.msh

$MICROSTRUCTURES_PATH/parameter_lookup/lookup.py \
    --metric compliance \
    --index-dir $MICROSTRUCTURES_PATH/lookup_table/configs/3D/family61/5mm_cell/Julian/index \
    results/material_hex.msh \
    results/guide.msh

$MICROSTRUCTURES_PATH/parameter_lookup/generate_config.py \
    --geometry-correction-lookup=$MICROSTRUCTURES_PATH/wire_generator/examples/geometry_correction_new_pdms.csv \
    $MICROSTRUCTURES_PATH/patterns/3D/reference_wires/wires.txt \
    results/guide.msh \
    results/snake.config

$MICROSTRUCTURES_PATH/wire_generator/tile.py \
    -o results/snake_microstructure.obj \
    results/snake.config

## Support generation
$MICROSTRUCTURES_PATH/wire_generator/tile_only.py \
    --guide-mesh \
    results/snake.config \
    results/snake.wire

$MICROSTRUCTURES_PATH/Nico/printing_optimizer/bin/support_gen \
    results/snake.wire \
    results/snake_guide.obj \
    results/snake_microstructure.obj \
    0 0 1 \
    3.0 \
    results/snake_reorientated.wire \
    results/snake_support.wire \
    results/snake_guide.obj \
    results/snake_microstructure_reorientated.obj

$MICROSTRUCTURES_PATH/wire_generator/inflate.py \
    --thickness 0.5 \
    -o results/snake_support.obj \
    --subdiv 0 \
    results/snake_support.wire

