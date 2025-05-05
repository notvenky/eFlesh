#!/usr/bin/env python3.9
import argparse
import meshio
import numpy as np

parser = argparse.ArgumentParser(description='Rotate Mesh')
parser.add_argument('input', help='input mesh file')
parser.add_argument('output', help='output mesh file')
parser.add_argument('--angle', default=90, type=float, help='rotation angle')

args = parser.parse_args()

print("Starting step 0: Parsing obj file with polygons of multiple sides...")
vertices = []
elements = []
with open(args.input) as input_file:
    lines = input_file.readlines()

    for l in lines:
        if l.startswith("v"):
            fields = l.split(" ")
            if len(fields) < 4:
                continue

            new_vertex = np.array([float(fields[1]), float(fields[2]), float(fields[3])])
            vertices.append(new_vertex)

        elif l.startswith("f"):
            fields = l.split(" ")
            if len(fields) < 1:
                continue

            size = len(fields) - 1

            new_element = []
            for i in range(1,size):
                new_element.append(int(fields[i])-1)

            elements.append(new_element)

theta = np.radians(args.angle)
c, s = np.cos(theta), np.sin(theta)
R = np.array([[c, -s, 0.0], [s, c, 0.0], [0.0, 0.0, 0.0]])

rotated_vertices = []
for v in vertices:
    rotated_v = R.dot(v)
    rotated_vertices.append(rotated_v)
    #print(v)
    #print(rotated_v)

print("Starting step 1: Outputing file...")
output_file = open(args.output, "w")
for v in rotated_vertices:
    output_file.write("v {} {} 0\n".format(v[0], v[1]))

output_file.write("\n")

for e in elements:
    face_string = "f "
    for vi in e:
        face_string += "{} ".format(str(vi+1))
    face_string += "\n"
    output_file.write(face_string)

output_file.close()