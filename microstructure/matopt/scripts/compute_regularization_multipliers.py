#!/usr/bin/env python3.9
import meshio
import argparse
import numpy as np
import shapely
import json
import connectivity_tools as ct

from shapely.geometry import Point
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
parser.add_argument('output_msh', help='output triangle mesh with painted boundary')
parser.add_argument('--default_multiplier', type=float, default=1.0, help='default value')
parser.add_argument('--boundary_multiplier', type=float, default=10.0, help='largest value')
parser.add_argument('--boundary_distance', type=float, default=0.1, help='distance defining boundary zone')
parser.add_argument('--regularization_json', default=None, help='json for regularization weights')

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

distance = [float('inf')] * len(tri_elements)
for i, te in enumerate(tri_elements):
    print("Checking triangle {}".format(i))
    # compute barycenter of triangle
    total = np.array([0.0, 0.0, 0.0])
    for tev in te:
        vertex = tri_vertices[tev]
        total += vertex
    barycenter = Point(total[0]/3, total[1]/3)

    for j, ce in enumerate(cutcell_elements):
        # build polygon with cutcell element
        vertices_list = []
        for cev in ce:
            vertex = cutcell_vertices[cev]
            vertices_list.append((vertex[0], vertex[1]))
        polygon = Polygon(vertices_list)

        # check that barycenter is not contained
        # if it is, mark te as boundary element
        if polygon.contains(barycenter):
            distance[i] = distance_cutcell[j]
            break

#distance = [float('inf')] * len(tri_elements)
#for pol in polygons:
#    vertices_list = pol
#    vertices_list.append(pol[0])

#    line_string = LineString(vertices_list)
    #print(line_string)

#    for i, e in enumerate(tri_elements):
#        a = tri_vertices[e[0]]
#        b = tri_vertices[e[1]]
#        c = tri_vertices[e[2]]
#        dist = line_string.distance(Polygon([a, b, c]))
        #print(Polygon([a, b, c]))
#        if dist < distance[i]:
#            distance[i] = dist

regularization_weights = []
num_boundary_triangles = 0
for i, te in enumerate(tri_elements):
    weight = args.default_multiplier
    if distance[i] < args.boundary_distance:
        weight = args.boundary_multiplier
        num_boundary_triangles += 1

    regularization_weights.append(weight)

regularization_string = json.dumps(regularization_weights, indent=4, sort_keys=True)
if args.regularization_json is not None:
    json_file = open(args.regularization_json, 'w')
    json_file.write(regularization_string)
    json_file.close()

print("We have {} triangles located close to boundary out of {}".format(num_boundary_triangles, len(tri_elements)))
meshio.write_points_cells(args.output_msh, tri_vertices, mesh_tri.cells, cell_data={"distance": [np.array(distance)], "weight": [np.array(regularization_weights)]}, file_format="gmsh22")
