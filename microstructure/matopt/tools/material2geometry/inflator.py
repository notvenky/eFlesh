#!/usr/bin/env python
# -*- coding: utf-8 -*-

# System libs
import os
import sys
import json
import tempfile
import subprocess
from pathlib import Path


def inflate(pattern_filename, meshing_options, pattern_properties, output_mesh, quiet=False):
    # Find isosurface_cli path
    pathname = os.path.dirname(Path(__file__).absolute())
    isosurface_exe = pathname + '/../../../microstructures/cmake-build-release/isosurface_inflator/isosurface_cli'

    # Prepare command-line arguments
    symmetry = pattern_properties['symmetry']

    with tempfile.NamedTemporaryFile(suffix=".json", prefix="meshing_") as tmp_meshing:
        # Write options as json files
        with open(tmp_meshing.name, 'w') as f:
            f.write(json.dumps(meshing_options))

        args = [isosurface_exe,
                symmetry,
                pattern_filename,
                output_mesh,
                '--inflation_graph_radius', str(5),
                '-m', tmp_meshing.name,
                '--cheapPostprocessing',
                '--params',
                '{}'.format(' '.join(str(x) for x in pattern_properties['params']))]

        try:
            if quiet:
                print(' '.join(args))
                sys.stdout.flush()
                subprocess.check_call(args, stdout = open(os.devnull, 'wb'))
            else:
                print(' '.join(args))
                sys.stdout.flush()
                subprocess.check_call(args)
        except subprocess.CalledProcessError as e:
            print("Return Code: ", e.returncode)
            print(json.dumps(meshing_options, indent=4))
            print(json.dumps(pattern_properties, indent=4))
            raise e
