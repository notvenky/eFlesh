# OMAV thimble utils

Forked from the [eFlesh](https://github.com/notvenky/eFlesh) repo, this is a trimmed down version for the OMAV platform.
#####

## prerequisites

### to design/buy before you start
- design the STL/OBJ file of your desired shape for the sensor. Other designs are on [FILL IN LATER]
- get [N52 neodymium magnets](https://www.supermagnete.ch/scheibenmagnete-neodym/scheibenmagnet-9mm-3mm_S-09-03-N52N?group=lp-n52) N52 Ø9 mm, height 3 mm. This size is the best for SNR and provide a strong magnetism. 
- arduino [Adafruit QT py](https://www.berrybase.ch/adafruit-qt-py-samd21-dev-board-stemma-qt) with STEMMA QT connectors and some [Qwiic cables](https://www.berrybase.ch/sparkfun-qwiic-kabel-kit), how to setup the [adafruit](https://learn.adafruit.com/adafruit-qt-py)
- the [magnometer PCB board](https://shop.wowrobo.com/products/eflesh-magnetometer-board)
- a compass

### software
- bambu studio (the [ubuntu version](https://github.com/bambulab/BambuStudio/releaseshttps://bambulab.com/en/download/studio))
- [arduino ide](https://learn.adafruit.com/adafruit-arduino-ide-setup/linux-setup) for flashing the Adafruit (the newer versions of arduino failed, this one is old but good)
- [blender](https://www.blender.org/download/) to modify the mesh after generation


## getting started on crazy

log into `crazyinsight` 
```console
ssh omav4@crazyinsight.asl.ethz.ch
```
pw -> ask @luceharris or @michaelpantic

design the STL/OBJ file of your desired shape for the sensor. Other designs are on in `/home/omav4/thimble/`. Make sure to `scp` your design over to crazy, and put in the `thimble/` directory

now activate the docker
```console
docker run -it --rm -v ~/thimble:/workspace -w /workspace/omav_thimble_utils thimble-dev
```

now you are inside the docker workspace

### creating the mesh
the mesh is created using the eFlesh repository, which is already built. 

```console
cd /workspace/omav_thimble_utils/microstructure/microstructure_inflators
python create_mesh.py --input <path/to/your/design>
```

this could take some time, and will automatically generate the mesh size based on the overall size of the object. 
look at the folder /workspace/thimble/ to see the new generated `.obj` file. This name you will need next.

e.g. `panda_ee_0.01.3.obj`

### adding magnet pouches and cap


now we will use blender to add the pouches and cap:

```
cd /workspace/omav_thimble_utils/scripts/
blender -b -P generate_thimble.py -- --input <path/to/new.obj> --output <path/to/final.stl>
```
this should generate the cap and the magnet pouches again based on the size. #TODO @luceharris pouches currently not generating


## getting started - local

clone the repo

```
git clone --recurse-submodules git@github.com:ethz-asl/omav_thimble_utils.git
cd omav_thimble_utils
```

create python env e.g.
```
conda env create -f env.yml
conda activate thimble
```

or venv (recommended)
```
python -m venv /path/to/venv/thimble
source /path/to/thimble/bin/activate
pip install numpy scipy reskin-sensor matplotlib meshio tqdm libigl
```

## mesh generation

system pre-requisites
```
sudo apt-get update && sudo apt-get install -y build-essential cmake libgmp-dev libmpfr-dev libcgal-dev libeigen3-dev libsuitesparse-dev libboost-all-dev
```
> note: Running the following command as it is, uses 12 CPU nodes. You can customize by running ```./build.sh cpu_nodes=n``` where you can choose 'n' based on your system.
```
cd microstructure/microstructure_inflators && chmod +x build.sh && ./build.sh
```

You're now all set to use [`cut-cell.ipynb`](microstruture/microstructure_inflators/notebooks/cut-cell.ipynb) to make your own thimble sensor, point the notebook to your OBJ/STL fle.

## adding pouches and other parts to the sensor

now you should have a mesh STL/OBJ file generated, and we need to add pouches for the magnets. 

there are some pre-made [scripts for the pouches](scripts/create_pouches.py)
```
source /path/to/venv/thimble
/path/to/your/blender -bP scripts/create_pouches.py
```

you can change the magnet size, pouch location, and add other features such as [caps](scripts/create_caps.py) here. [STILL WIP]

## sensor fabrication with 3D printer

<p align="center">
  <img src="https://github.com/user-attachments/assets/de48d4cc-23c9-44f1-8513-785790dfbc8a" width="400" alt="fabrication_only">
</p>

using the bambu lab PS1, 3D print it with [TPU 95A](https://www.amazon.com/Polymaker-Filament-Flexible-1-75mm-Cardboard/dp/B09KKRYHS6). Warning, each print takes about 7h so plan well. 

- in prepare, add the STL model generated from before
- go to file > import > import configs to load the [`thimble.json`](cfg/thimble.json)
- click on the object on the plate and choose the loaded thimble settings
- right click on the object and add 4 primitive cylinders and place them inside the magnet pouch locations
- inside the magnet pouch, change the infill so the pouch doesn't fill up
- slice the plate, check the magnet pouches, and add a pause at the end of the magnet pouch
- print, then add the magnets with glue making sure each magnet points the same pole
  - please wear gloves!!!

## other parts

[WIP] first perform the initial [fabrication validation](https://github.com/ethz-asl/omav_thimble/omav_thimble/docs/fabrication_validation.md) to see if this sensor is viable. 

then to connect the thimble mesh to the endeffector for the OMAV, first print the attachment [ADD LATER]
- glue in the thimble sensor
- screw in the magnometer PCB board into the slot

[LATER] should epoxy the whole thing. 

## flashing the arduino

first follow the [adafruit](https://learn.adafruit.com/adafruit-qt-py/arduino-ide-setup) steps to add the board to the IDE

next upload the arduino code located in [`5X_thimble_stream.ino`](arduino/5X_thimble_stream/5X_thimble_stream.ino) to the qtPy arduino using the arduino IDE. 

## sensor characterisation

data collection [`data_collection.md`](https://github.com/ethz-asl/omav_thimble/omav_thimble/docs/data_collection.md)




