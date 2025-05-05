import meshio
import argparse
import numpy as np
#import material2geometry as m2g
import material2geometry_angle as m2g
import json
import os

parser = argparse.ArgumentParser(description='Generate json with information for every quadcell')
parser.add_argument('input', help='input .msh with triangulated mesh and material properties')
parser.add_argument('--coefficients', help='input file with coefficients to be used to generate geometries')
parser.add_argument('--pattern', help='input pattern')
parser.add_argument('--symmetry', default='cubic', help='input pattern')
parser.add_argument('--base-nu', type=float, default=0.0, help='poisson ratio value')
parser.add_argument('--base-E', type=float, default=1.0, help='youngs modulus value')
parser.add_argument('output_json', help='output json containing geometry information for every quad cells')

if __name__ == "__main__":
    args = parser.parse_args()

    #mat2geo = m2g.Material2Geometry(in_path=args.coefficients, larger_than_90=False, base_E=float(args.base_E), base_nu=float(args.base_nu))
    mat2geo = m2g.Material2Geometry(in_path=args.coefficients, base_E=float(args.base_E), base_nu=float(args.base_nu))

    # Parse information of quad mesh
    mesh_quad = meshio.read(args.input)
    quad_vertices = mesh_quad.points
    quad_elements = mesh_quad.cells[0][1]
    quad_youngs = mesh_quad.cell_data_dict["E"]["quad"]
    quad_poisson = mesh_quad.cell_data_dict["nu"]["quad"]

    params = []
    for q in range(0, len(quad_elements)):
        geo_params = mat2geo.evaluate(quad_poisson[q], quad_youngs[q], 90.0)
        #geo_params = mat2geo.evaluate(quad_poisson[q], quad_youngs[q])
        print(geo_params)

        params.append({
            "symmetry": args.symmetry,
            "pattern": os.path.realpath(args.pattern),
            "params": geo_params
        })

    params_string = json.dumps(params, indent=4, sort_keys=True)
    json_file = open(args.output_json, 'w')
    json_file.write(params_string)
    json_file.close()



