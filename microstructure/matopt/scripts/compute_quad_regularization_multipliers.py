#!/usr/bin/env python3.9
import meshio
import argparse
import numpy as np
import json
import connectivity_tools as ct

from shapely.geometry import LineString
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
parser.add_argument('regularization_json', default=None, help='json for regularization weights')
parser.add_argument('--default_multiplier', type=float, default=1.0, help='default value')
parser.add_argument('--boundary_multiplier', type=float, default=10.0, help='largest value')
parser.add_argument('--boundary_distance', type=float, default=0.1, help='distance defining boundary zone')

args = parser.parse_args()

mesh_tri = meshio.read(args.input_triangulation)
tri_vertices = mesh_tri.points
tri_elements = mesh_tri.cells[0][1]

boundary_edges = ct.compute_boundary_edges(tri_elements)
polygons = ct.find_polygons(tri_vertices, boundary_edges)

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

distance_cutcell = [float('inf')] * len(cutcell_elements)
for pol in polygons:
    vertices_list = pol
    vertices_list.append(pol[0])

    line_string = LineString(vertices_list)
    #print(line_string)

    for i, ce in enumerate(cutcell_elements):
        list_vertices = []
        for vi in ce:
            list_vertices.append(cutcell_vertices[vi])
        list_vertices.append(cutcell_vertices[ce[0]])
        #print(len(list_vertices))
        dist = line_string.distance(Polygon(list_vertices))
        #print(dist)
        if dist < distance_cutcell[i]:
            distance_cutcell[i] = dist

regularization_weights = []
num_boundary_cells = 0
for i, te in enumerate(cutcell_elements):
    weight = args.default_multiplier
    if distance_cutcell[i] < args.boundary_distance:
        weight = args.boundary_multiplier
        num_boundary_cells += 1

    regularization_weights.append(weight)

regularization_string = json.dumps(regularization_weights, indent=4, sort_keys=True)
json_file = open(args.regularization_json, 'w')
json_file.write(regularization_string)
json_file.close()

print("We have {} cells located close to boundary out of {}".format(num_boundary_cells, len(cutcell_elements)))