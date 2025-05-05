## Material optimization
#$MESHFEM_PATH/MaterialOptimization_cli \
#    -m isotropic \
#    -b bound/homogenized_material.bound \
#    -n 10 -R -r 0.001 \
#    cell_5/bird_tet.msh \
#    BC/bird.bd \
#    results/cell_5/material_tet.msh
#

## Pattern param lookup and microstructure generation
#$MICROSTRUCTURES_PATH/driver/preprocess_guide_mesh.py \
#    -o results/cell_5/material_hex.msh \
#    results/cell_5/material_tet.msh \
#    cell_5/bird_hex.msh
#
#$MICROSTRUCTURES_PATH/parameter_lookup/lookup.py \
#    --metric compliance \
#    --index-dir $MICROSTRUCTURES_PATH/lookup_table/configs/3D/family61/5mm_cell/Julian/index \
#    results/cell_5/material_hex.msh \
#    results/cell_5/guide.msh
#
#$MICROSTRUCTURES_PATH/parameter_lookup/generate_config.py \
#    $MICROSTRUCTURES_PATH/patterns/3D/reference_wires/wires.txt \
#    results/cell_5/guide.msh \
#    results/cell_5/bird.config
#
#$MICROSTRUCTURES_PATH/wire_generator/tile.py \
#    -o results/cell_5/bird_microstructure.obj \
#    results/cell_5/bird.config

## Support generation
$MICROSTRUCTURES_PATH/wire_generator/tile_only.py \
    --guide-mesh \
    results/cell_5/bird.config \
    results/cell_5/bird.wire

$MICROSTRUCTURES_PATH/Nico/printing_optimizer/bin/support_gen \
    results/cell_5/bird.wire \
    results/cell_5/bird_guide.obj \
    results/cell_5/bird_microstructure.obj \
    1 0 0 \
    3.0 \
    results/cell_5/bird_reorientated.wire \
    results/cell_5/bird_support.wire \
    results/cell_5/bird_guide.obj \
    results/cell_5/bird_microstructure_reorientated.obj

$MICROSTRUCTURES_PATH/wire_generator/inflate.py \
    --thickness 0.5 \
    -o results/cell_5/bird_support.obj \
    --subdiv 0 \
    results/cell_5/bird_support.wire

