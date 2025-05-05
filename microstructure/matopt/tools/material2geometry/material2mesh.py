#!/usr/bin/env python3
import argparse
import sys
import os
import tempfile

import material2geometry as m2g
import inflator
import homogenization


parser = argparse.ArgumentParser(description='Filters coverage table')
parser.add_argument('nu', help='poisson ratio value')
parser.add_argument('E', help='youngs modulus value')
parser.add_argument('--in-coefficients', default=None, help='file where coefficients are stored')
parser.add_argument('--out', default="tmp.msh", help='output')
parser.add_argument('--homogenize', action='store_true', help='run homogenization to check quality of mesh')

if __name__ == "__main__":
    args = parser.parse_args()

    mat2geo = m2g.Material2Geometry(in_path=args.in_coefficients)

    geo_params = mat2geo.evaluate(float(args.nu), float(args.E))
    print(geo_params)

    # prepare properties structure
    pattern = "0646.wire"
    pattern_properties = dict()
    pattern_properties["dim"] = 3
    pattern_properties["symmetry"] = 'cubic'
    pattern_properties["params"] = geo_params
    pattern_properties["pattern"] = pattern

    # prepare path of pattern
    pathname = os.path.dirname(sys.argv[0])
    pattern_file = pathname + "/../../../microstructures/data/patterns/3D/reference_wires/pattern" + pattern

    # prepare meshing options
    meshing_options = homogenization.meshing_options
    inflator.inflate(pattern_filename=pattern_file, meshing_options=meshing_options, pattern_properties=pattern_properties, output_mesh=args.out, quiet=False)

    if args.homogenize:
        with tempfile.NamedTemporaryFile(suffix=".txt", prefix="log_") as tmp_log:
            is_good = homogenization.simulate_mesh(args.out, homogenization.material_options, tmp_log.name)
            properties, anisotropy = homogenization.read_simulation_log(tmp_log.name)
