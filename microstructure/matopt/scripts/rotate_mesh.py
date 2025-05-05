#!/usr/bin/env python3.9
import argparse
import meshio
import numpy as np

parser = argparse.ArgumentParser(description='Rotate Mesh')
parser.add_argument('input', help='input mesh file')
parser.add_argument('output', help='output mesh file')
parser.add_argument('--angle', default=90, type=float, help='rotation angle')

args = parser.parse_args()

mesh = meshio.read(args.input)
vertices = mesh.points
elements = mesh.cells[0][1]

theta = np.radians(args.angle)
c, s = np.cos(theta), np.sin(theta)
R = np.array([[c, -s, 0.0], [s, c, 0.0], [0.0, 0.0, 0.0]])

rotated_vertices = []
for v in vertices:
    rotated_v = R.dot(v)
    rotated_vertices.append(rotated_v)
    #print(v)
    #print(rotated_v)

meshio.write_points_cells(args.output, rotated_vertices, mesh.cells)