#!/bin/bash
export MESHFEM_PATH=/home/luigi/devel/MeshFEM

time $MESHFEM_PATH/MaterialOptimization_cli \
    -m isotropic \
    -b bound/homogenized_material.bound \
    -n 200 -r 0.03 \
    -d 2 \
    tet_mesh.msh \
    constraints.bd \
    result.msh

