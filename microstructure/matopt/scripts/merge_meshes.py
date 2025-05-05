import meshio
import argparse
import numpy as np
import shapely
import json
import subprocess

from shapely.geometry import Point
from shapely.geometry.polygon import Polygon

import connectivity_tools
import paths

def extract_boundary_mesh(input, output):
    converter = paths.find("mesh_convert", paths.MESHFEM_BUILD_PATH)
    if converter is None:
        raise FileNotFoundError("mesh_convert")

    args = [converter, input, output, '-b']
    subprocess.check_call(args)

parser = argparse.ArgumentParser(description='Merge meshes into new one')
parser.add_argument("--input", nargs="+", default=[])
parser.add_argument('--output', help='output triangle mesh with merged geometry')

args = parser.parse_args()

# get all meshes
meshes_vertices = []
meshes_elements = []
meshes_boundaries = []
for mesh_path in args.input:
    mesh = meshio.read(mesh_path)
    vertices = mesh.points
    elements = mesh.cells[0][1]

    meshes_vertices.append(vertices)
    meshes_elements.append(elements)

# merge all meshes together and write file with it
merged_vertices = list(meshes_vertices[0])
merged_elements = list(meshes_elements[0])
for i in range(1, len(args.input)):
    n = len(merged_vertices)
    merged_vertices += list(meshes_vertices[i])
    for e in meshes_elements[i]:
        merged_elements.append([e[0]+n, e[1]+n, e[2]+n])
cells = [("triangle", np.array(merged_elements))]
meshio.write_points_cells("merged.msh", np.array(merged_vertices), cells, file_format="gmsh22")

# get file with only boundaries (use MeshFEM)
extract_boundary_mesh("merged.msh", "merged_boundary.msh")

# create file with holes for all meshes (note that this are not all holes for merged meshes due to intersections)
index = 0
components = connectivity_tools.compute_connected_components(merged_vertices, merged_elements)
holes = []
for i, c in enumerate(components):
    print("Component {}".format(i))
    new_holes = connectivity_tools.find_component_holes(c, merged_vertices, merged_elements)
    holes += new_holes
hole_points = connectivity_tools.compute_holes_points(holes)
holes_file = open("holes.xyz", "w")
for h in hole_points:
    holes_file.write("{} {}\n".format(h[0], h[1]))
holes_file.close()

# generate new merged mesh without outside and without holes of each mesh (use TriWild)


# remove remaining holes generating during merge
# idea: for each triangle, check if it's inside the exterior boundary of all the meshes passed as input. If yes, remove it!
# need to create polygons with shapely, including holes. One for each original mesh. Then, check each triangle against each of these...