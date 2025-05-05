#!/usr/bin/env python3.9
import plotly.offline as py
import plotly.graph_objs as go
import meshio
import argparse
import toptools
import numpy as np

parser = argparse.ArgumentParser(description='Plot material distribution')
parser.add_argument('input', help='input .obj with triangulated mesh')
parser.add_argument('--output', default="material_distribution.html", help="output plot")

args = parser.parse_args()

mesh_tri = meshio.read(args.input)
tri_vertices = mesh_tri.points
tri_elements = mesh_tri.cells[0][1]

E = mesh_tri.cell_data['E'][0]
nu = mesh_tri.cell_data['nu'][0]

trace = go.Scatter(
    x=nu,
    y=E,
    mode='markers',
)

top_theoretical_triangle = toptools.top_theoretical_triangle(1.0, 0.3, 1.0)
left_point = np.array([-1.0, 0.0])
right_point = np.array([1.0, 0.0])
trace_theoretical = go.Scatter(x=[left_point[0], right_point[0], top_theoretical_triangle[0], left_point[0]], y=[left_point[1], right_point[1], top_theoretical_triangle[1], left_point[1]], fill="toself")

fig = go.Figure()

data = [trace, trace_theoretical]

layout = go.Layout(
    title='Material distribution',
    scene=dict(
        xaxis=dict(
            title= 'nu'
        ),
        yaxis=dict(
            title='E'
        )
    )
)

fig = go.Figure(data=data, layout=layout)
py.plot(fig, filename=args.output)
