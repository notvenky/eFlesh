#!/usr/bin/env python3.9
import meshio
import argparse
import numpy as np
import shapely
import json

from shapely.geometry import Point
from shapely.geometry.polygon import Polygon


def compute_boundary_faces(faces):
    result = []
    is_cut_element = [False] * len(faces)
    # collect all edges
    all_edges = []
    edges_to_elements = []
    for k, f in enumerate(faces):
        face_edges = []
        for i, vi in enumerate(f):
            vim = f[(i+1) % len(f)]

            new_e = [vi, vim]
            face_edges.append(new_e)

        edges_to_elements += [k] * len(face_edges)
        all_edges += face_edges

    # count how many times each edge appears
    edge_count = dict()
    for i, e in enumerate(all_edges):
        e_set = frozenset(e)
        if e_set in edge_count:
            edge_count[e_set] += [i]
        else:
            edge_count[e_set] = [i]

    for es in edge_count:
        if len(edge_count[es]) == 1:
            is_cut_element[edges_to_elements[edge_count[es][0]]] = True

    for k, f in enumerate(faces):
        if is_cut_element[k]:
            result.append(k)

    return np.array(result)


parser = argparse.ArgumentParser(description='Transform triangulated mesh into one with information about boundary')
parser.add_argument('input_triangulation', help='input .obj with triangulated mesh')
parser.add_argument('input_cutcell', help='input .obj with mesh')
parser.add_argument('output_msh', help='output triangle mesh with painted boundary')
parser.add_argument('--bounds_json', default=None, help='json for material bounds')
parser.add_argument('--regularization_json', default=None, help='json for regularization weights')

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


print("Step 1: Identifying cut cells...")
# - Define boundary polygon edges and their corresponding faces
cut_cells = compute_boundary_faces(cutcell_elements)
print(len(cut_cells))

print("Step 2: For each triangle cell, check if barycenter is inside any of the cut cells...")
boundary_triangles = []
on_boundary = [0] * len(tri_elements)
for i, te in enumerate(tri_elements):
    print("Checking triangle {}".format(i))
    # compute barycenter of triangle
    total = np.array([0.0, 0.0, 0.0])
    for tev in te:
        vertex = tri_vertices[tev]
        total += vertex
    barycenter = Point(total[0]/3, total[1]/3)

    for ce in cut_cells:
        # build polygon with cutcell element
        vertices_list = []
        for cev in cutcell_elements[cut_cells[ce]]:
            vertex = cutcell_vertices[cev]
            vertices_list.append((vertex[0], vertex[1]))
        polygon = Polygon(vertices_list)

        # check that barycenter is not contained
        # if it is, mark te as boundary element
        if polygon.contains(barycenter):
            boundary_triangles.append(te)
            on_boundary[i] = 1
            break

material_bounds = []
for i, te in enumerate(tri_elements):
    element_material_bounds = {}
    if on_boundary[i] == 1:
        #element_material_bounds['lower'] = [[0, 0.999], [1, 0.299]]
        #element_material_bounds['upper'] = [[0, 1.000], [1, 0.301]]
        element_material_bounds['lower'] = [[0, 1.000], [1, 0.300]]
        element_material_bounds['upper'] = [[0, 1.000], [1, 0.300]]
        #element_material_bounds['lower'] = [[0, 0.1], [1, 0.300]] # densities
        #element_material_bounds['upper'] = [[0, 1.0], [1, 0.300]] # densities
        #simplex = [[1.3, -1.0, -1.0], [0.7, 1.0, -1.0], [-2.0, 0.0, 0.0]]  # entire theoretical range
        #simplex = [[0.3, -1.0, -0.765829], [-0.3, 1.0, -0.814573], [-1.0, 0.0, 0.005], [1., 0., -0.32],
        #           [1.050772, -0.50062, -0.087101], [0.7345313, 0.470559, -0.370772]] # quadfoam
        #element_material_bounds['simplex'] = simplex
    else:
        #element_material_bounds['lower'] = [[0, 0.100], [1, 0.2800]]
        #element_material_bounds['upper'] = [[0, 0.900], [1, 0.3200]]
        #simplex = [[0.248, -0.83, 0.0167], [-0.028, 0.38, -0.0872], [-0.22, 0.45, -0.0005]] # 0200
        #simplex = [[0.01, -0.3, 0.014], [0.236, -0.48, -0.0656], [-0.026, 0.33, -0.0748], [-0.22, 0.45, -0.0005]] # 0200
        #simplex = [[-0.065, -0.17, 0.04115], [0.152, -0.42, 0.01414], [0.123, 0.66, -0.27552], [-0.21, -0.07, 0.0371]] # 0201
        #simplex = [[-0.063, -0.155, 0.03793], [0.166, -0.485, 0.02246], [0.045, 0.7, -0.24585], [-0.148, -0.06, 0.02828]]  # 0201
        simplex = [[1.3, -1.0, -1.0], [0.7, 1.0, -1.0], [-2.0, 0.0, 0.0]]
        element_material_bounds['simplex'] = simplex

    material_bounds.append(element_material_bounds)

material_bounds_string = json.dumps(material_bounds, indent=4, sort_keys=True)
if args.bounds_json is not None:
    json_file = open(args.bounds_json, 'w')
    json_file.write(material_bounds_string)
    json_file.close()


regularization_weights = []
for i, te in enumerate(tri_elements):
    weight = 1.0
    if on_boundary[i] == 1:
        weight = 10.0

    regularization_weights.append(weight)

regularization_string = json.dumps(regularization_weights, indent=4, sort_keys=True)
if args.regularization_json is not None:
    json_file = open(args.regularization_json, 'w')
    json_file.write(regularization_string)
    json_file.close()

print("We have {} boundary triangles (inside {} cut cells) out of {}".format(len(boundary_triangles), len(cut_cells), len(tri_elements)))
meshio.write_points_cells(args.output_msh, tri_vertices, mesh_tri.cells, cell_data={"level": [np.array(on_boundary)]}, file_format="gmsh22")
