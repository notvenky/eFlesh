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


parser = argparse.ArgumentParser(description='Split triangular mesh into boundary triangular mesh and interior quad mesh with material properties')
parser.add_argument('input_triangulation', help='input .msh with triangulated mesh and material properties')
parser.add_argument('input_cutcell', help='input .obj with mesh')
parser.add_argument('output_boundary_msh', help='output triangle mesh with boundary')
parser.add_argument('output_interior_msh', help='output quad mesh with material properties')
parser.add_argument('--output_boundary_obj', default=None, help='output cutcells')
parser.add_argument('--output_boundary_densities', default=None, help='output cutcells densities')
parser.add_argument('--output_interior_triangles', default=None, help='output interior triangles')

args = parser.parse_args()

print("Step 0: Reading triangular mesh with material information...")
mesh_tri = meshio.read(args.input_triangulation)
tri_vertices = mesh_tri.points
tri_elements = mesh_tri.cells[0][1]
tri_youngs = mesh_tri.cell_data_dict["E"]["triangle"]
tri_poisson = mesh_tri.cell_data_dict["nu"]["triangle"]


print("Step 1: Parsing obj file with polygons of multiple sides...")
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


print("Step 2: Identifying cut cells...")
# - Define boundary polygon edges and their corresponding faces
cutcells = compute_boundary_faces(cutcell_elements)
print(len(cutcells))

print("Step 3: Build all polygons...")
polygons = []
for ce in cutcell_elements:
    # build polygon with cutcell element
    vertices_list = []
    for cev in ce:
        vertex = cutcell_vertices[cev]
        vertices_list.append((vertex[0], vertex[1]))
    polygons.append(Polygon(vertices_list))

print("Step 3: Build cutcell polygons...")
cutcell_polygons = []
for ce in cutcells:
    # build polygon with cutcell element
    vertices_list = []
    for cev in cutcell_elements[ce]:
        vertex = cutcell_vertices[cev]
        vertices_list.append((vertex[0], vertex[1]))
    cutcell_polygons.append(Polygon(vertices_list))


print("Step 4: For each triangle cell, check if barycenter is inside any of the cut cells...")
boundary_triangles = []
on_boundary = [0] * len(tri_elements)
pol_E = [[] for i in range(len(polygons))]
pol_nu = [[] for i in range(len(polygons))]
for e, te in enumerate(tri_elements):
    print("Checking triangle {}".format(e))
    # compute barycenter of triangle
    total = np.array([0.0, 0.0, 0.0])
    for tev in te:
        vertex = tri_vertices[tev]
        total += vertex
    barycenter = Point(total[0]/3, total[1]/3)

    for p, pol in enumerate(polygons):
        #print(p)
        # check that barycenter is not contained
        # if it is, mark te as boundary element
        if pol.contains(barycenter):
            # add information about material to polygon
            pol_E[p].append(tri_youngs[e])
            pol_nu[p].append(tri_poisson[e])
            break

    for p, pol in enumerate(cutcell_polygons):
        # check that barycenter is not contained
        # if it is, mark te as boundary element
        if pol.contains(barycenter):
            boundary_triangles.append(te)
            on_boundary[e] = 1
            break




print("Step 5: Decide material for each polygon...")
pol_E_avg = [0.0] * len(polygons)
pol_nu_avg = [0.0] * len(polygons)
for p, pol in enumerate(polygons):
    sum = 0.0
    print(pol_E[p])
    print(pol_nu[p])
    for y in pol_E[p]:
        sum += y
    pol_E_avg[p] = sum / len(pol_E[p])

    sum = 0.0
    for nu in pol_nu[p]:
        sum += nu
    pol_nu_avg[p] = sum / len(pol_nu[p])


print("Step 6: Print mesh with only cutcell triangles...")
boundary_cells = [("triangle", np.array(boundary_triangles))]
meshio.write_points_cells(args.output_boundary_msh, tri_vertices, boundary_cells, file_format="gmsh22")

cutcells_set = set(cutcells)
interior_quads = []
interior_quads_E = []
interior_quads_nu = []
for c, cell in enumerate(cutcell_elements):
    if c not in cutcells_set:
        interior_quads.append(cell)
        interior_quads_E.append(pol_E_avg[c])
        interior_quads_nu.append(pol_nu_avg[c])

print("Step 7: Print quad mesh with material information about interior cells...")
interior_cells = [("quad", np.array(interior_quads))]
interior_cells_data={"E": [interior_quads_E], "nu": [interior_quads_nu]}
meshio.write_points_cells(args.output_interior_msh, cutcell_vertices, interior_cells, cell_data=interior_cells_data, file_format="gmsh22")

if args.output_boundary_obj is not None:
    f = open(args.output_boundary_obj, 'w')
    output_cells = []
    for c, cell in enumerate(cutcell_elements):
        if c in cutcells_set:
            output_cells.append(cell)

    for v in cutcell_vertices:
        f.write("v {} {} 0.0\n".format(v[0], v[1]))

    for c in cutcells:
        cell = cutcell_elements[c]
        row = "f "
        for v in cell:
            row = row + " " + str(v + 1)
        row = row + '\n'
        f.write(row)

    f.close()

if args.output_boundary_densities is not None:
    densities = []
    for c, cell in enumerate(cutcell_elements):
        if c in cutcells_set:
            densities.append(pol_E_avg[c])

    densities_string = json.dumps(densities, indent=4, sort_keys=True)
    json_file = open(args.output_boundary_densities, 'w')
    json_file.write(densities_string)
    json_file.close()

if args.output_interior_triangles is not None:
    interior_triangles = []
    for e, te in enumerate(tri_elements):
        if not on_boundary[e]:
            interior_triangles.append(te)

    interior_cells = [("triangle", np.array(interior_triangles))]
    meshio.write_points_cells(args.output_interior_triangles, tri_vertices, interior_cells, file_format="gmsh22")