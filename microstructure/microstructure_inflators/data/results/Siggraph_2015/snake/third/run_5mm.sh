## Pattern param lookup and microstructure generation
$MICROSTRUCTURES_PATH/driver/preprocess_guide_mesh.py \
    --young="E" \
    --poisson="nu" \
    -o results/material_hex.msh \
    bend_third.msh \
    bend_third_hex.msh

$MICROSTRUCTURES_PATH/parameter_lookup/lookup.py \
    --metric compliance \
    --index-dir $MICROSTRUCTURES_PATH/lookup_table/configs/3D/family61/5mm_cell/Julian/index \
    results/material_hex.msh \
    results/guide.msh

    #--geometry-correction-lookup=$MICROSTRUCTURES_PATH/wire_generator/examples/geometry_correction_new_pdms.csv \
$MICROSTRUCTURES_PATH/parameter_lookup/generate_config.py \
    $MICROSTRUCTURES_PATH/patterns/3D/reference_wires/wires.txt \
    results/guide.msh \
    results/bend_third.config

$MICROSTRUCTURES_PATH/wire_generator/tile.py \
    -o results/bend_third_microstructure.obj \
    results/bend_third.config

## Support generation
$MICROSTRUCTURES_PATH/wire_generator/tile_only.py \
    --guide-mesh \
    results/bend_third.config \
    results/bend_third.wire

$MICROSTRUCTURES_PATH/Nico/printing_optimizer/bin/support_gen \
    results/bend_third.wire \
    results/bend_third_guide.obj \
    results/bend_third_microstructure.obj \
    0 0 1 \
    3.0 \
    results/bend_third_reorientated.wire \
    results/bend_third_support.wire \
    results/bend_third_guide.obj \
    results/bend_third_microstructure_reorientated.obj

$MICROSTRUCTURES_PATH/wire_generator/inflate.py \
    --thickness 0.5 \
    -o results/bend_third_support.obj \
    --subdiv 0 \
    results/bend_third_support.wire

