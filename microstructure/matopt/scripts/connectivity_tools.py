import os
import sys
import argparse
import subprocess

# Third-party libs
import numpy as np
import meshio
import json
import tempfile

import paths

def parse_args():
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("input", default=None, help="input mesh")

    return parser.parse_args()


def find_polygons(vertices, edges):

    # count number of vertices in polygons
    boundary_vertices = set()
    for e in edges:
        v1, v2 = e
        boundary_vertices.add(v1)
        boundary_vertices.add(v2)

    num_polygon_vertices = len(boundary_vertices)

    # create dictionary relating each vertex to its edges
    vertex_edges = [[] for i in range(len(vertices))]
    for e, edge in enumerate(edges):
        v1, v2 = edge

        vertex_edges[v1].append(e)
        vertex_edges[v2].append(e)

    # find polygons
    polygons = []
    used_vertices = 0
    part_of_polygon = [False] * len(vertices)
    while used_vertices < num_polygon_vertices:
        polygon = []

        # Find non-used vertex
        initial_vertex = 0
        for i in boundary_vertices:
            if not part_of_polygon[i]:
                initial_vertex = i
                break

        # add initial vertex
        polygon.append(vertices[initial_vertex])
        part_of_polygon[initial_vertex] = True
        boundary_vertices.remove(initial_vertex)
        used_vertices += 1

        # search for whole polygon
        current_vertex = initial_vertex
        # print("Current vertex: " + str(current_vertex))
        while True:
            found_next = False

            # loop through edges to find next vertex
            edges_to_look = vertex_edges[current_vertex]
            for e in edges_to_look:
                v1, v2 = edges[e]

                if v1 == current_vertex and not part_of_polygon[v2]:
                    polygon.append(vertices[v2])
                    part_of_polygon[v2] = True
                    used_vertices += 1
                    current_vertex = v2
                    boundary_vertices.remove(current_vertex)
                    found_next = True
                    break
                elif v2 == current_vertex and not part_of_polygon[v1]:
                    polygon.append(vertices[v1])
                    part_of_polygon[v1] = True
                    current_vertex = v1
                    used_vertices += 1
                    boundary_vertices.remove(current_vertex)
                    found_next = True
                    break

            if not found_next:
                break

            if initial_vertex == current_vertex:
                part_of_polygon[initial_vertex] = True
                used_vertices += 1
                break

        polygons.append(polygon)

    return polygons


def write_poly(polygon, path):
    poly_file = open(path, 'w')

    # First line: <# of vertices> <dimension (must be 2)> <# of attributes> <# of boundary markers (0 or 1)>
    poly_file.write("# vertices list\n")
    poly_file.write("{} {} {} {}\n".format(len(polygon), 2, 0, 0))

    # Following lines: <vertex #> <x> <y> [attributes] [boundary marker]
    for v, vertex in enumerate(polygon):
        poly_file.write('{} {} {}\n'.format(v, vertex[0], vertex[1]))

    # One line: <# of segments> <# of boundary markers (0 or 1)>
    poly_file.write("\n# edges list\n")
    poly_file.write('{} {}\n'.format(len(polygon), 0))
    # Following lines: < segment  # > <endpoint> <endpoint> [boundary marker]
    for v in range(len(polygon)):
        v1_index = v
        v2_index = (v + 1) % len(polygon)

        poly_file.write('{} {} {}\n'.format(v, v1_index, v2_index))
    poly_file.write("\n")

    # One line: <  # of holes>
    poly_file.write("\n# holes list\n")
    poly_file.write("0\n")


def triangulate_hole(hole):
    triangle_exec = 'triangle'
    converter = paths.find("mesh_convert", paths.MESHFEM_BUILD_PATH)
    if converter is None:
        raise FileNotFoundError("mesh_convert")

    with tempfile.TemporaryDirectory() as tmpdirname:
        tmp_path = tmpdirname + '/polygon.poly'
        write_poly(hole, tmp_path)

        args = [triangle_exec, '-e', tmp_path]
        subprocess.check_call(args, stderr=subprocess.DEVNULL, stdout=subprocess.DEVNULL)

        nodes_file = tmpdirname + '/polygon.1.node'
        mesh_file =  tmpdirname + '/tmp.msh'
        args = [converter, nodes_file, mesh_file]
        subprocess.check_call(args, stderr=subprocess.DEVNULL, stdout=subprocess.DEVNULL)

        mesh = meshio.read(mesh_file)
        vertices = mesh.points
        elements = mesh.cells[0][1]

    return vertices, elements


def count_edges(faces):
    # collect all edges
    all_edges = set([])
    for f in faces:
        e1 = frozenset([f[0], f[1]])
        e2 = frozenset([f[1], f[2]])
        e3 = frozenset([f[2], f[0]])

        all_edges.add(e1)
        all_edges.add(e2)
        all_edges.add(e3)

    return len(all_edges)


def count_vertices(faces):
    # collect all vertices
    all_vertices = set([])
    for f in faces:
        all_vertices.add(f[0])
        all_vertices.add(f[1])
        all_vertices.add(f[2])

    return len(all_vertices)


def compute_boundary_edges(faces):
    result = []

    # collect all edges
    all_edges = []
    for f in faces:
        e1 = [f[0], f[1]]
        e2 = [f[1], f[2]]
        e3 = [f[2], f[0]]

        all_edges += [e1, e2, e3]

    # count how many times each edge appears
    edge_count = dict()
    for e in all_edges:
        e_set = frozenset(e)
        if e_set in edge_count:
            edge_count[e_set] += [e]
        else:
            edge_count[e_set] = [e]

    for es in edge_count:
        if len(edge_count[es]) == 1:
            result += edge_count[es]

    return np.array(result)


def find_elements_neighbors(vertices, elements):
    # For each element, find its neighbors
    elements_neighbors = [[] for elem in elements]

    # Create list of elements per vertex
    elements_per_vertex = [[] for v in vertices]
    for e, elem in enumerate(elements):
        for v in elem:
            elements_per_vertex[v].append(e)

    for e, elem in enumerate(elements):
        # print("working on element: {}".format(e))
        candidates = []
        for v in elem:
            candidates += elements_per_vertex[v]

        candidates_count = dict()
        for c in candidates:
            if c in candidates_count:
                candidates_count[c] += 1
            else:
                candidates_count[c] = 1

        for c in candidates_count:
            if candidates_count[c] >= 2:
                elements_neighbors[e].append(c)

        # print("working on element: {}".format(e))
        # for n, nelem in enumerate(elements):
        #    if e == n:
        #        continue

        #    count = 0
        #    for v in nelem:
        #        if v in elem:
        #            count +=1

        #    if count >= 2:
        #        elements_neighbors[e].append(n)

    return elements_neighbors


def compute_connected_components(vertices, elements):
    elements_neighbors = find_elements_neighbors(vertices, elements)

    # Now, run flood/fill algorithm, walking through neighbors and counting number of components
    visited = [False for elem in elements]
    components = []
    for e, elem in enumerate(elements):
        if visited[e] == True:
            continue
        visited[e] = True

        # add one more to components
        new_component = [e]

        component_elements = elements_neighbors[e]
        while len(component_elements) > 0:
            current = component_elements.pop(0)
            if visited[current] == True:
                continue

            new_component.append(current)
            visited[current] = True
            component_elements += elements_neighbors[current]

        components.append(new_component)

    return components


def find_component_boundary(component, vertices, elements):

    component_elements = []
    for c in component:
        component_elements.append(elements[c])

    boundary_edges = compute_boundary_edges(component_elements)
    polygons = find_polygons(vertices, boundary_edges)

    bb_min = np.array([np.PINF, np.PINF])
    bb_max = np.array([np.NINF, np.NINF])
    boundary_idx = -1
    for i, pol in enumerate(polygons):
        pol_min = np.min(pol, axis=0)[:2]
        pol_max = np.max(pol, axis=0)[:2]

        if np.all(pol_min < bb_min) and np.all(pol_max > bb_max):
            bb_min = pol_min
            bb_max = pol_max

            boundary_idx = i

    return polygons[boundary_idx]


def find_component_holes(component, vertices, elements):

    component_elements = []
    for c in component:
        component_elements.append(elements[c])

    print("Computing boundary edges...")
    boundary_edges = compute_boundary_edges(component_elements)
    print("Computing polygons...")
    polygons = find_polygons(vertices, boundary_edges)

    print("Finding outer boundary...")
    bb_min = np.array([np.PINF, np.PINF])
    bb_max = np.array([np.NINF, np.NINF])
    boundary_idx = -1
    for i, pol in enumerate(polygons):
        pol_min = np.min(pol, axis=0)[:2]
        pol_max = np.max(pol, axis=0)[:2]

        if np.all(pol_min < bb_min) and np.all(pol_max > bb_max):
            bb_min = pol_min
            bb_max = pol_max

            boundary_idx = i

    print("Finding holes...")
    holes = []
    for i, pol in enumerate(polygons):
        if boundary_idx == i:
            continue

        holes.append(pol)

    return holes


def compute_holes_points(holes):
    holes_points = []
    for i, h in enumerate(holes):
        vh, eh = triangulate_hole(h)
        #print("Hole {} vertices: {}".format(i, vh))
        #print("Hole {} elements: {}".format(i, eh))

        centroid = np.array([0.0, 0.0])
        for v in eh[0]:
            vertex = vh[v, :2]
            centroid += vertex
        centroid /= len(eh[0])
        holes_points.append(centroid)
        print("Hole {} centroid: {}".format(i, centroid))

    return holes_points



if __name__ == "__main__":
    args = parse_args()

    mesh = meshio.read(args.input)
    vertices = mesh.points
    elements = mesh.cells[0][1]

    components = compute_connected_components(vertices, elements)

    print("Number of connected components: {}".format(len(components)))


    for i, c in enumerate(components):
        holes = find_component_holes(c, vertices, elements)

        print("Component {} has {} holes".format(i, len(holes)))

        compute_holes_points(holes)

