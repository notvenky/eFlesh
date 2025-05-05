$MICROSTRUCTURES_PATH/wire_generator/tile_and_fit_periodic.py \
    --material $MICROSTRUCTURES_PATH/wire_generator/examples/B9CreatorCherryResin.material \
    pattern0059.config tmp.msh

$MICROSTRUCTURES_PATH/wire_generator/tile.py \
    -o pattern0059_tile.msh \
    pattern0059_tile.config

#$MICROSTRUCTURES_PATH/wire_generator/tile.py \
#    -o tmp.msh \
#    pattern0059.config
