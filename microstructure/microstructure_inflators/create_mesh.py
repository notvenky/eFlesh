import os
import sys
import json
import copy
import argparse
from pathlib import Path
from typing import Tuple

import subprocess
import meshio
import numpy as np

try:
    import yaml
    _YAML_AVAILABLE = True
except ImportError:
    _YAML_AVAILABLE = False


def load_config(config_path):
    if not _YAML_AVAILABLE:
        print("Warning: PyYAML not installed — ignoring --config")
        return {}
    with open(config_path) as f:
        return yaml.safe_load(f) or {}

"""
Taken from EFlesh paper
Author Lucy Harris
"""


# ------------------------------------------------------------
# utilities
# ------------------------------------------------------------


def change_extension(path: Path, new_ext: str) -> Path:
    return path.with_suffix(f".{new_ext}")


def compute_grid_corners(bbox, cell_size) -> tuple:
    corner0 = np.ceil(bbox[0] / cell_size).astype(int)
    corner1 = np.ceil(bbox[1] / cell_size).astype(int)
    return corner0, corner1


def young(E, i, j, k):
    """return same youngs modulus through the layers"""
    return E


# ------------------------------------------------------------
# main
# ------------------------------------------------------------
def main():

    parser = argparse.ArgumentParser()
    parser.add_argument("--input", required=True, help="Input STL file")
    parser.add_argument("--config", default=None, help="Path to thimble_config.yaml")
    parser.add_argument("--relative-cell-size", type=float, default=None)
    parser.add_argument("--E", type=float, default=None)
    parser.add_argument("--nu", type=float, default=None)
    parser.add_argument("--only-cube-cells", action="store_true")
    parser.add_argument("--microstructure-repo", default="./")
    parser.add_argument("--matopt-repo", default="../matopt")

    args = parser.parse_args()

    cfg = load_config(args.config) if args.config else {}
    mesh_cfg = cfg.get("mesh", {})

    relative_cell_size = args.relative_cell_size or mesh_cfg.get("relative_cell_size", 0.2)
    E  = args.E  or mesh_cfg.get("E",  0.01)
    nu = args.nu or mesh_cfg.get("nu", 0.09)

    # reassign onto args-like namespace for the rest of the script
    args.relative_cell_size = relative_cell_size
    args.E  = E
    args.nu = nu

    input_path = Path(args.input).resolve()
    micro_repo = Path(args.microstructure_repo).resolve()
    matopt_repo = Path(args.matopt_repo).resolve()

    if not input_path.exists():
        raise FileNotFoundError(f"Input STL not found: {input_path}")

    if not micro_repo.exists():
        raise FileNotFoundError(f"Microstructure repo not found: {micro_repo}")

    if not matopt_repo.exists():
        raise FileNotFoundError(f"Matopt repo not found: {matopt_repo}")

    stitch_cli = micro_repo / "build/isosurface_inflator/cut_cells_cli"

    if not stitch_cli.exists():
        raise FileNotFoundError(f"cut_cells_cli not found: {stitch_cli}")

    # --- load mesh
    print("Loading mesh...")
    mesh = meshio.read(str(input_path))
    vertices = mesh.points.astype(float)

    bbox = np.array([vertices.min(axis=0), vertices.max(axis=0)])
    print("Bounding box:", bbox)
    extent = bbox[1] - bbox[0]
    max_dim = np.max(extent)
    print(f"Max dim: {max_dim}")

    # cell_size = args.cell_size
    cell_size = int(max_dim * args.relative_cell_size)
    corner0, corner1 = compute_grid_corners(bbox, cell_size)

    print(f"cell size: {cell_size}")

    dims = corner1 - corner0 + 1
    print("Grid dimensions:", dims)

    if np.any(dims <= 0):
        raise RuntimeError("Grid collapsed — cell size too large for geometry.")

    # --- convert stl to obj

    obj_path = change_extension(input_path, "obj")
    mesh.write(str(obj_path))
    print("Converted to OBJ:", obj_path)

    # --- load Material2Geometry

    sys.path.insert(0, str(matopt_repo / "tools/material2geometry"))

    from material2geometry import Material2Geometry

    mat2geo = Material2Geometry(
        in_path=str(
            matopt_repo / "tools/material2geometry/0646_geo_1_coeffs.txt"
        )
    )

    pattern_path = (
        micro_repo / "data/patterns/3D/reference_wires/pattern0646.wire"
    )

    # --- generate patterns

    print("Generating cell patterns...")

    patterns = []

    for i in range(corner0[0], corner1[0] + 1):
        for j in range(corner0[1], corner1[1] + 1):
            for k in range(corner0[2], corner1[2] + 1):

                geo_params = mat2geo.evaluate(args.nu, young(args.E, i, j, k))

                patterns.append(
                    {
                        "params": geo_params,
                        "symmetry": "Cubic",
                        "pattern": str(pattern_path),
                        "index": [int(i), int(j), int(k)],
                    }
                )

    print(f"Total cells: {len(patterns)}")

    if len(patterns) == 0:
        raise RuntimeError("No cells generated.")

    # --- write json

    json_path = input_path.parent / "data.json"

    with open(json_path, "w") as f:
        json.dump(patterns, f)

    print("Wrote pattern JSON:", json_path)

    # --- run cli

    out_path = input_path.with_name(
        f"{input_path.stem}_{args.E}_{cell_size}.obj"
    )

    cmd = [
        str(stitch_cli),
        "-p", # patch
        str(json_path),
        "--gridSize",
        str(cell_size),
        "-o", # output
        str(out_path),
        "-r", # resolution
        "30",
    ] 


    if not args.only_cube_cells:
        cmd.extend(["--surface", str(obj_path)])

    print("Running:", " ".join(cmd))

    subprocess.run(cmd, check=True)

    print("Done.")
    print("Output:", out_path)


if __name__ == "__main__":
    main()
