#!/usr/bin/env python

import os
import os.path
from subprocess import check_call

ROOT_DIR = os.environ.get("MICROSTRUCTURES_PATH");
if ROOT_DIR is None:
    raise RuntimeError("Environment variable \"MICROSTRUCTURES_PATH\" is not set!");

#exe_name = os.path.join(ROOT_DIR, "Luigi/wireinflator2D/python/extract_symmetry_orbits.py");
exe_name = os.path.join(ROOT_DIR, "wire_generator/extract_symmetry_orbits.py");
wire_dir = os.path.join(ROOT_DIR, "patterns/2D/");

for wire_file in os.listdir(wire_dir):
    wire_file = os.path.join(wire_dir, wire_file);
    name, ext = os.path.splitext(wire_file);
    if ext != ".wire": continue;
    print("processing {}".format(wire_file));

    command = "{} {}".format(exe_name, wire_file);
    check_call(command.split());

