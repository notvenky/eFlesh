#!/usr/bin/env python
# -*- coding: utf-8 -*-

# System libs
import os
import sys
import json
import tempfile
import subprocess
from pathlib import Path
import re

# Third party libs
import numpy as np


meshing_options = {
    "domainErrorBound": 1e-5,
    "facetAngle": 30.0,
    "facetSize": 0.02,
    "facetDistance": 1e-3,
    "cellSize": 0.50,
    "edgeSize": 0.015,
    "cellRadiusEdgeRatio": 2.0,

    "marchingSquaresGridSize": 512,
    "marchingSquaresCoarsening": 3,
    "marchingCubesGridSize": 128,

    "maxArea": 0.001,
    "featureAngleThreshold": 0.7853981633974483
}


material_options = {
    "type": "isotropic_material",
    "dim": 3,
    "young": 1.0,
    "poisson": 0.0
}


def compute_anisotropy(homogenized_properties):
    C = homogenized_properties["homogenized_moduli"]
    E = C[0]
    nu = C[3]
    mu = C[6]
    ani = mu / (E / (2 * (1 + nu)))
    homogenized_properties["anisotropy"] = {
        "zener": ani
    }

def simulate_mesh(mesh_name, material_options, output_log):
    pathname = os.path.dirname(Path(__file__).absolute())
    homogenizer_exe = pathname + '/../../../MeshFEM/cmake-build-release/MeshFEM/PeriodicHomogenization_cli'

    with tempfile.NamedTemporaryFile(suffix=".json", prefix="material_") as tmp_material:
        with open(tmp_material.name, 'w') as f:
            f.write(json.dumps(material_options))

        cmd = [homogenizer_exe, mesh_name, '-m', tmp_material.name, '--ortho']
        print(' '.join(cmd))
        with open(output_log, 'w') as out_log:
            try:
                result = subprocess.call(cmd, stdout=out_log)
                return bool(result == 0)
            except KeyboardInterrupt:
                raise
            except:
                print("Could not run simulation. Go to the following!")
                return False


def read_simulation_log(log_path):
    out_log = open(log_path, 'r')

    float_pattern = re.compile(r'\-?\d+\.?\d*e?-?\d*')  # Compile a pattern to capture float values

    wire_content = out_log.readlines()
    for l, line in enumerate(wire_content):
        if line.startswith("Elasticity tensor:") or line.startswith("Homogenized elasticity tensor:"):
            tensor_values = []
            for r in range(1, 7):
                row = [float(i) for i in float_pattern.findall(wire_content[l + r])]
                for v in row:
                    tensor_values.append(v)

    C1111 = tensor_values[0]
    C1122 = tensor_values[1]
    C2323 = tensor_values[21]
    nu = 1.0 / (1.0 + C1111 / C1122)
    E = C1111 * (2 * nu * nu + nu - 1.0) / (nu - 1.0)
    G = C2323

    anisotropy = 2 * (1.0 + nu) * G / E

    return [nu, E, G], anisotropy


def homogenize_msh(msh_filename, material_options, jacobian = [1, 0, 0, 0, 1, 0, 0, 0, 1], quiet=False):
    # Find DeformedCells_cli path
    pathname = os.path.dirname(Path(__file__).absolute())
    homogenizer_exe = pathname + '/../../../MeshFEM/cmake-build-release/MeshFEM/DeformedCells_cli'

    with tempfile.NamedTemporaryFile(suffix=".json", prefix="meshing_") as tmp_material, \
            tempfile.NamedTemporaryFile(suffix=".json", prefix="homogenized_") as tmp_output:

        # Write options as json files
        with open(tmp_material.name, 'w') as f:
            f.write(json.dumps(material_options))

        args = [homogenizer_exe,
                '-m', tmp_material.name,
                '--homogenize',
                '--dumpJson', tmp_output.name,
                '--jacobian', '{} {} {} {} {} {} {} {} {}'.format(*jacobian),
                msh_filename]

        try:
            if quiet:
                subprocess.check_call(args, stdout=open(os.devnull, 'wb'), stderr=open(os.devnull, 'wb'))
            else:
                print(' '.join(args))
                sys.stdout.flush()
                subprocess.check_call(args)
        except subprocess.CalledProcessError as e:
            print("Return Code: ", e.returncode)
            print(json.dumps(material_options, indent=4))
            raise e
        else:
            with open(tmp_output.name, 'r') as f:
                try:
                    homogenized_properties = json.load(f)
                    compute_anisotropy(homogenized_properties)
                    return homogenized_properties
                except ValueError:
                    print("[homogenize] Warning: output json file is empty!")
                    return None


def get_elasticity_tensor(homogenized_properties):
    C = np.array(homogenized_properties['elasticity_tensor'])
    return C.reshape(6, 6)


def get_young_poisson(homogenized_properties):
    E, _, _, nu, _, _, muXY, _, _ = homogenized_properties['homogenized_moduli']
    return E, nu
