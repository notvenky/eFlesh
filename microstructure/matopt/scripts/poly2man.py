#!/usr/bin/env python3.9
import argparse
import numpy as np

parser = argparse.ArgumentParser(description='Convert poly file to man format file')
parser.add_argument('input', help='input .poly file')
parser.add_argument('output', help='output .man file')

args = parser.parse_args()

print("Input is: {}".format(args.input))
print("Output is: {}".format(args.output))

parsing_vertices = False
parsing_edges = False
parsing_elements = False

vertices_indices = []
vertices = []
edges_indices = []
edges = []
with open(args.input) as input_file:
    lines = input_file.readlines()

    print("Input content:")
    li = 0
    for l in lines:

        if not l or not l.strip():
            continue

        if l[0] == '#':
            continue

        if li == 0:
            print("Parsing Vertices...")
            parsing_vertices = True
            parsing_edges = False
            fields = l.split(" ")
            num_vertices = int(fields[0])
        elif li <= num_vertices:
            print(".")
            fields = l.split(" ")
            vertices_indices.append(int(fields[0]))
            vertices.append([float(fields[1]), float(fields[2])])
        elif li == (num_vertices+1):
            print("Parsing Edges...")
            parsing_vertices = False
            parsing_edges = True
            fields = l.split(" ")
            num_edges = int(fields[0])
        elif li <= num_vertices + num_edges + 1:
            print(",")
            fields = l.split(" ")
            edges_indices.append(int(fields[0]))
            edges.append([int(fields[1]), int(fields[2])])
        else:
            print("Parsing holes")

        li += 1

print("List of vertices indices: " + str(vertices_indices))
print("List of vertices: "+str(vertices))
print("List of edges indices: " + str(edges_indices))
print("List of edges: "+str(edges))

output = open(args.output, 'w')
output.write("Vertices\n")
for i, v in enumerate(vertices):
    output.write("{}, {}, {}\n".format(vertices_indices[i], v[0], v[1]))

output.write("\nEdges\n")
for i, e in enumerate(edges):
    output.write("{}, {}, {}, -1, -1\n".format(edges_indices[i], e[0], e[1]))

output.close()

