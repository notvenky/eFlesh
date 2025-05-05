#!/usr/bin/env python3.9
import meshio
import argparse
import numpy as np
import json

from shapely.geometry import Point
from shapely.geometry.polygon import Polygon

parser = argparse.ArgumentParser(description='Transform triangulated mesh into one with information about boundary')
parser.add_argument('input_triangulation', help='input .obj with triangulated mesh')
parser.add_argument('input_cutcell', help='input .obj with mesh')
parser.add_argument('output_msh', help='output triangle mesh with painted boundary')
parser.add_argument('--cell_index_json', default=None, help='json for elements index')

args = parser.parse_args()

mesh_tri = meshio.read(args.input_triangulation)
tri_vertices = mesh_tri.points
tri_elements = mesh_tri.cells[0][1]

print("Starting step 0: Parsing obj file with polygons of multiple sides...")
cutcell_vertices = []
cutcell_elements = []
with open(args.input_cutcell) as input_file:
    lines = input_file.readlines()

    for l in lines:
        if l.startswith("v"):
            fields = l.split(" ")
            if len(fields) < 4:
                continue

            new_vertex = np.array([float(fields[1]), float(fields[2]), float(fields[3])])
            cutcell_vertices.append(new_vertex)

        elif l.startswith("f"):
            fields = l.split(" ")
            if len(fields) < 1:
                continue

            size = len(fields) - 1

            new_element = []
            for i in range(1,size):
                new_element.append(int(fields[i])-1)

            cutcell_elements.append(new_element)

print("There are {} cutcell elements".format(len(cutcell_elements)))

print("Step 1: For each triangle cell, check if barycenter is inside any of the cut elements cells...")
cell_index = [-1] * len(tri_elements)
for i, te in enumerate(tri_elements):
    print("Checking triangle {}".format(te))
    # compute barycenter of triangle
    total = np.array([0.0, 0.0, 0.0])
    for tev in te:
        vertex = tri_vertices[tev]
        total += vertex
    barycenter = Point(total[0]/3, total[1]/3)

    for ei, ce in enumerate(cutcell_elements):
        # build polygon with cutcell element
        vertices_list = []
        for cev in ce:
            vertex = cutcell_vertices[cev]
            vertices_list.append((vertex[0], vertex[1]))
        polygon = Polygon(vertices_list)

        # check that barycenter is not contained
        # if it is, mark te as boundary element
        if polygon.contains(barycenter):
            cell_index[i] = ei
            break

    if cell_index[i] == -1:
        print("Cell not found: {}, {}, {}".format(tri_vertices[te[0]], tri_vertices[te[1]], tri_vertices[te[2]]))
        break


cell_index_string = json.dumps(cell_index, indent=4, sort_keys=True)
if args.cell_index_json is not None:
    print("Step 2: Write json file...")
    json_file = open(args.cell_index_json, 'w')
    json_file.write(cell_index_string)
    json_file.close()

print("Step 3: Write mesh file...")
meshio.write_points_cells(args.output_msh, tri_vertices, mesh_tri.cells, cell_data={"index": [np.array(cell_index)]}, file_format="gmsh22")
