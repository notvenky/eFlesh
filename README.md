<h1 align="center" style="font-size: 2.0em; font-weight: bold; margin-bottom: 0; border: none; border-bottom: none;">eFlesh: Highly customizable Magnetic Touch Sensing using Cut-Cell Miscrostructures</h1>

##### <p align="center"> [Venkatesh Pattabiraman](https://venkyp.com), [Zizhou Huang](https://huangzizhou.github.io/), [Daniele Panozzo](https://cims.nyu.edu/gcl/daniele.html), [Denis Zorin](https://cims.nyu.edu/gcl/denis.html), [Lerrel Pinto](https://www.lerrelpinto.com/) and [Raunaq Bhirangi](https://raunaqbhirangi.github.io/)</p>
##### <p align="center"> New York University </p>

<!-- <p align="center">
  <img src="assets/eflesh.gif">
 </p> -->

#####
<div align="center">
    <a href="https://e-flesh.com"><img src="https://img.shields.io/static/v1?label=Project%20Page&message=Website&color=blue"></a> &ensp;
<!--     <a href=""><img src="https://img.shields.io/static/v1?label=Paper&message=Arxiv&color=red"></a> &ensp;  -->
    <a href="https://github.com/notvenky/eFlesh/blob/main/microstructure/README.md"><img src="https://img.shields.io/static/v1?label=CAD-to-eFlesh&message=Conversion&color=blue"></a> &ensp;
<!--     <a href=""><img src="https://img.shields.io/static/v1?label=Community&message=Discord&color=violet"></a> &ensp; -->
    <a href="https://github.com/notvenky/eFlesh/tree/main/characterization/datasets"><img src="https://img.shields.io/static/v1?label=Characterization&message=Datasets&color=blue"></a> &ensp;
    
</div>

#####

## Getting Started
```
git clone https://github.com/notvenky/eFlesh.git
cd eFlesh
conda env create -f env.yml
```

## Microstructure

To run the cut-cell microstructure optimizers and generate the lattice structures, there are some dependancies to be installed. Please use the following links provided and download [oneTBB](https://github.com/uxlfoundation/oneTBB/blob/master/INSTALL.md) and [BOOST](https://www.boost.org/users/history/version_1_83_0.html) from source. 

<!--#### This requires some dependancies:-->
```
cd eFlesh/microstructure/microstructure_inflators
mkdir build && cd build
```
Please replace the path placeholders below to the correct local paths, during the installation. 
```
cmake -DCMAKE_BUILD_TYPE=release .. -DTBB_ROOT=</path/to/oneTBB/installation> -DBoost_NO_SYSTEM_PATHS=ON -DBOOST_ROOT=</path/to/boost_1_83_0>
```
```
make -j4 stitch_cells_cli
```
```
make -j4 cut_cells_cli
```
```
make -j4 stack_cells
```

In the conversion notebooks ```regular.ipynb``` and ```cut-cell.ipynb```, ensure to provide the correct paths against all marked palceholders.


## Sensor Characterization

We characterize eFlesh's spatial resolution, normal force and shear force prediction accuracy through controlled experiments, The curated datasets can be found in ```characterization/datasets/```. For training, we use a simple two layered MLP with 128 nodes (```python train.py --mode <spatial/normal/shear> --folder /path/to/corresponding/dataset```).

## Slip Detection

We grasp different objects using the Hello Stretch Robot equipped with eFlesh, and tug at it to collect our dataset. The dataset can be found in ```slip_detection/data```, and the trained classifier is ```slip_detection/checkpoints/eflesh_linear.pkl```.

## Visuo-Tactile Policy Learnig

We perform four precise manipulation tasks, using the [Visuo-Skin](https://visuoskin.github.io) framework, achieving an average success rate of >90%.

## References 
eFlesh draws upon these prior works:

1. [Cut-Cell Microstructures for Two-scale Structural Optimization](https://cims.nyu.edu/gcl/papers/2024-cutcells.pdf)
2. [Learning Precise, Contact-Rich Manipulation through Uncalibrated Tactile Skins](https://visuoskin.github.io)
3. [AnySkin: Plug-and-play Skin Sensing for Robotic Touch](https://any-skin.github.io)
4. [ReSkin: versatile, replaceable, lasting tactile skins](https://reskin.dev)
<!--
## Cite 
If you use eFlesh or its sub-components, please consider citing us as follows:
```

```
-->
