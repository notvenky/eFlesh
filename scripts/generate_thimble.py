import bpy # from blender
import sys
import os
import math
import argparse
import json

from datetime import datetime
from mathutils import Vector
from pathlib import Path

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

# ============================================================
# general utilities
# ============================================================
def simplify_object(obj, file_path, target_faces=1000000):
    file_path = Path(file_path)
    file_dir = file_path.parent
    file_name = file_path.stem

    simplified_name = f"simplified-{file_name}.obj"
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.mode_set(mode="OBJECT")

    poly_count = len(obj.data.polygons)
    if poly_count <= target_faces:
        print(f"No simplification needed ({poly_count} faces).")
        return

    ratio = target_faces / poly_count
    print(
        f"Applying Decimate: {poly_count} → {target_faces} faces (ratio={ratio:.4f})"
    )

    mod = obj.modifiers.new(name="Decimate", type="DECIMATE")
    mod.ratio = ratio
    bpy.ops.object.modifier_apply(modifier=mod.name)

    # export simplified obj
    bpy.ops.wm.obj_export(filepath=os.path.join(file_dir, simplified_name))

    return

# ============================================================
# utility Geometry Functions
# ============================================================


def create_cylinder(name, radius, depth, location, axis="Y", vertices=128):
    rot = (0, 0, 0)
    if axis == "X":
        rot = (0, math.radians(90), 0)
    elif axis == "Y":
        rot = (math.radians(90), 0, 0)

    bpy.ops.mesh.primitive_cylinder_add(
        radius=radius,
        depth=depth,
        location=location,
        vertices=vertices,
        rotation=rot,
    )

    obj = bpy.context.object
    obj.name = name
    return obj

def create_chamfer(name, cavity_radius, chamfer_depth, location, axis="Y"):
    bpy.ops.mesh.primitive_cone_add(
        radius1=cavity_radius + chamfer_depth,
        radius2=cavity_radius,
        depth=chamfer_depth,
        location=location,
    )

    obj = bpy.context.object
    obj.name = name

    if axis == "Y":
        obj.rotation_euler = (math.radians(90), 0, 0)

    return obj


def boolean(obj_name, tool_obj, operation):
    target = bpy.data.objects[obj_name]
    bpy.context.view_layer.objects.active = target

    mod = target.modifiers.new(name="Bool", type="BOOLEAN")
    mod.operation = operation
    mod.object = tool_obj
    bpy.ops.object.modifier_apply(modifier=mod.name)

    bpy.data.objects.remove(tool_obj)


# ============================================================
# geometry analysis
# ============================================================


def get_object_radius(obj, axis="Y"):
    bbox = [obj.matrix_world @ Vector(c) for c in obj.bound_box]

    min_x = min(v.x for v in bbox)
    max_x = max(v.x for v in bbox)
    min_z = min(v.z for v in bbox)
    max_z = max(v.z for v in bbox)

    radius = max((max_x - min_x), (max_z - min_z)) / 2.0

    return radius, bbox


# ============================================================
# magnet packing
# ============================================================


def choose_magnet_count(
    object_radius, magnet_diameter, min_wall, min_clearance=0.5, pouch_wall=1.0,
    max_count=4
):
    magnet_radius = magnet_diameter / 2.0
    # pouch center must be inset by magnet_radius + shell wall + min_wall
    available_radius = object_radius - magnet_radius - pouch_wall - min_wall

    if available_radius <= 0:
        return 0, 0

    # actual footprint of each pouch (magnet + shell on each side)
    pouch_diameter = magnet_diameter + 2 * pouch_wall

    for n in [4, 3, 2]:
        if n > max_count:
            continue
        spacing = 2 * available_radius * math.sin(math.pi / n)
        if spacing > pouch_diameter + min_clearance:
            return n, available_radius

    print(f"Warning: object too small to fit 2 non-overlapping pouches "
          f"(object_radius={object_radius:.1f}, pouch_diameter={pouch_diameter:.1f}). "
          f"Use --no-pouches or a larger design.")
    return 0, 0


def circular_layout(radius, count, center, axis="Y"):
    centers = []

    for i in range(count):
        angle = 2 * math.pi * i / count
        x = radius * math.cos(angle)
        z = radius * math.sin(angle)

        if axis == "Y":
            centers.append([x, center[1], z])
        elif axis == "Z":
            centers.append([x, z, center[2]])
        elif axis == "X":
            centers.append([center[0], x, z])

    return centers


def stacked_layout(object_radius, axis_min, axis_max, magnet_diameter, magnet_thickness,
                   min_wall, min_clearance=1.0, pouch_wall=1.0, axis="Y", max_count=3):
    """Single column of magnets stacked along the axis at maximum radial offset."""
    step = magnet_thickness + min_clearance
    axis_length = axis_max - axis_min
    n = min(max_count, max(1, int(axis_length / step)))

    # Spread n magnets equidistantly along the central axis with margins at both ends
    if n == 1:
        positions = [(axis_min + axis_max) / 2.0]
    else:
        spacing = axis_length / (n + 1)
        positions = [axis_min + spacing * (i + 1) for i in range(n)]

    centers = []
    for a in positions:
        if axis == "Y":
            centers.append([0, a, 0])
        elif axis == "Z":
            centers.append([0, 0, a])
        elif axis == "X":
            centers.append([a, 0, 0])

    return centers


def rings_layout(object_radius, axis_min, axis_max, magnet_diameter, magnet_thickness,
                 min_wall, n_rings=2, ring_spacing=None, ring_rotation_offset=90.0,
                 min_clearance=1.5, pouch_wall=1.0, axis="Y", max_magnets=4):
    """
    Multiple circular rings of magnets at evenly-spaced heights along the axis.
    Rings are rotated relative to each other (ring_rotation_offset degrees),
    which improves spatial coverage of the magnetic field and gives a richer signal.
    :param max_magnets caps the total number of magnets placed across all rings.
    :param
    Returns (centers, meta) where meta records the resolved layout parameters for the manifest.
    """
    max_per_ring = max(1, max_magnets // n_rings)
    n_per_ring, layout_radius = choose_magnet_count(
        object_radius, magnet_diameter, min_wall, min_clearance, pouch_wall,
        max_count=max_per_ring
    )
    if n_per_ring == 0:
        return [], {}

    # Place rings equidistant between the center axis and the outer surface.
    # This keeps magnets well inside the body rather than flush against the skin.
    equidistant_radius = object_radius / 2.0
    min_radial = magnet_diameter / 2.0 + pouch_wall + min_wall
    pouch_diameter = magnet_diameter + 2 * pouch_wall
    if equidistant_radius >= min_radial:
        circumferential_spacing = 2 * equidistant_radius * math.sin(math.pi / n_per_ring)
        if circumferential_spacing > pouch_diameter + min_clearance:
            layout_radius = equidistant_radius

    axis_length = axis_max - axis_min
    axis_center = (axis_min + axis_max) / 2.0

    if ring_spacing is None:
        if n_rings == 1:
            ring_spacing = 0.0
        else:
            # spread rings over 30% of the available axis length
            ring_spacing = (axis_length * 0.3) / (n_rings - 1)

    total_span = ring_spacing * (n_rings - 1)
    ring_start = axis_center - total_span / 2.0

    all_centers = []
    for ring_idx in range(n_rings):
        axis_pos = ring_start + ring_idx * ring_spacing
        angle_offset = math.radians(ring_rotation_offset * ring_idx)
        for i in range(n_per_ring):
            angle = 2 * math.pi * i / n_per_ring + angle_offset
            r_cos = layout_radius * math.cos(angle)
            r_sin = layout_radius * math.sin(angle)
            if axis == "Y":
                all_centers.append([r_cos, axis_pos, r_sin])
            elif axis == "Z":
                all_centers.append([r_cos, r_sin, axis_pos])
            elif axis == "X":
                all_centers.append([axis_pos, r_cos, r_sin])

    meta = {
        "n_rings": n_rings,
        "n_per_ring": n_per_ring,
        "ring_spacing_mm": round(ring_spacing, 3),
        "ring_rotation_offset_deg": ring_rotation_offset,
        "layout_radius_mm": round(layout_radius, 3),
        "total_magnets": n_rings * n_per_ring,
    }
    return all_centers, meta


# ============================================================
# magnet pouch generator
# ============================================================


def create_magnet_pouch(
    name, magnet_diameter, magnet_thickness, location, axis="Y", tolerance=0.2, chamfer=0.5
):
    cavity_radius = (magnet_diameter - tolerance) / 2.0
    cavity_depth = magnet_thickness + 0.1 # extra depth

    outer_radius = magnet_diameter / 2 + 1.0
    outer_depth = magnet_thickness + 1.0

    cavity = create_cylinder(
        name + "_cavity",
        cavity_radius,
        cavity_depth,
        location,
        axis,
    )

    chamfer = create_chamfer(
        name + "_chamfer",
        cavity_radius,
        chamfer,
        location,
        axis,
    )

    shell = create_cylinder(
        name + "_shell",
        outer_radius,
        outer_depth,
        location,
        axis,
    )

    return cavity, shell, chamfer


# ============================================================
# base cap generator
# ============================================================


def add_base_cap(obj_name, thickness, axis="Y"):
    print(f"adding base cap of {thickness} [mm] to {obj_name}")
    obj = bpy.data.objects[obj_name]
    bbox = [obj.matrix_world @ Vector(c) for c in obj.bound_box]

    min_y = min(v.y for v in bbox)
    max_x = max(v.x for v in bbox)
    min_x = min(v.x for v in bbox)
    max_z = max(v.z for v in bbox)
    min_z = min(v.z for v in bbox)

    radius = max((max_x - min_x), (max_z - min_z)) / 2.0

    cap_location = (0, min_y - thickness / 2, 0)

    cap = create_cylinder(
        "base_cap",
        radius,
        thickness,
        cap_location,
        axis,
    )

    boolean(obj_name, cap, "UNION")
    #mod = cap.modifiers.new("Bevel", "BEVEL")

    #mod.width = 0.5
    #mod.segments = 3
    #bpy.ops.object.modifier_apply(modifier=mod.name)


# ============================================================
# main
# ============================================================


def main():

    argv = sys.argv
    argv = argv[argv.index("--") + 1 :]

    parser = argparse.ArgumentParser()
    parser.add_argument("--input", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--config", default=None, help="Path to thimble_config.yaml")
    parser.add_argument("--magnet-diameter", type=float, default=None)
    parser.add_argument("--magnet-thickness", type=float, default=None)
    parser.add_argument("--min-wall", type=float, default=None)
    parser.add_argument("--cap-thickness", type=float, default=None)
    parser.add_argument("--axis", default=None)
    parser.add_argument("--tolerance", type=float, default=None)
    parser.add_argument("--fit-mode", default=None, choices=["press", "slip"])
    parser.add_argument("--no-pouches", action="store_true", help="Skip magnet pouch generation")
    parser.add_argument("--layout", default=None, choices=["circular", "stacked", "rings"])
    parser.add_argument("--n-rings", type=int, default=None, help="Number of rings (rings layout)")
    parser.add_argument("--ring-spacing", type=float, default=None, help="mm between rings; auto if omitted")
    parser.add_argument("--ring-rotation-offset", type=float, default=None, help="degrees of rotation between rings (default 45)")
    parser.add_argument("--max-magnets", type=int, default=None, help="Maximum total number of magnets (default 4)")
    parser.add_argument("--print-id", default=None, help="Short identifier for this print variant (used in manifest)")
    parser.add_argument("--notes", default=None, help="Free-text notes recorded in the manifest")

    args = parser.parse_args(argv)

    # load config file defaults, then apply CLI overrides
    cfg = load_config(args.config) if args.config else {}
    magnet_cfg = cfg.get("magnet", {})
    sensor_cfg = cfg.get("sensor", {})
    mesh_cfg = cfg.get("mesh", {})

    magnet_diameter  = args.magnet_diameter  or magnet_cfg.get("diameter",  9.0)
    magnet_thickness = args.magnet_thickness or magnet_cfg.get("thickness", 3.0)
    min_wall         = args.min_wall         or magnet_cfg.get("min_wall",  1.0)
    tolerance        = args.tolerance        or magnet_cfg.get("tolerance", 0.25)
    fit_mode         = args.fit_mode         or magnet_cfg.get("fit_mode",  "press")
    cap_thickness    = args.cap_thickness    or sensor_cfg.get("cap_thickness", 1.0)
    axis             = args.axis             or sensor_cfg.get("axis", "Y")
    layout           = args.layout           or magnet_cfg.get("layout", "circular")

    stacked_cfg = magnet_cfg.get("stacked", {})
    stacked_max_count     = stacked_cfg.get("max_count", 3)

    rings_cfg = magnet_cfg.get("rings", {})
    n_rings               = args.n_rings               or rings_cfg.get("n_rings", 2)
    ring_spacing          = args.ring_spacing          or rings_cfg.get("ring_spacing", None)
    ring_rotation_offset  = args.ring_rotation_offset  or rings_cfg.get("ring_rotation_offset", 45.0)
    max_magnets           = args.max_magnets           or magnet_cfg.get("max_magnets", 4)

    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete()

    bpy.ops.wm.obj_import(filepath=args.input)
    obj = bpy.context.view_layer.objects.active
    if obj is None:
        raise RuntimeError(f"no active obj after import, {args.input}")
    obj.name = "micro"

    simplify_object(obj, args.input)

    object_radius, bbox = get_object_radius(obj, axis)
    obj.rotation_euler[0] += math.pi
    bpy.ops.object.transform_apply(rotation=True)

    center = obj.location
    center_y = center.y

    layout_meta = {}  # extra resolved params written to manifest

    if not args.no_pouches:
        axis_vals = {"Y": (min(v.y for v in bbox), max(v.y for v in bbox)),
                     "Z": (min(v.z for v in bbox), max(v.z for v in bbox)),
                     "X": (min(v.x for v in bbox), max(v.x for v in bbox))}
        axis_min, axis_max = axis_vals[axis]

        if layout == "stacked":
            centers = stacked_layout(
                object_radius, axis_min, axis_max,
                magnet_diameter, magnet_thickness,
                min_wall, axis=axis, max_count=stacked_max_count,
            )
            print(f"Stacked layout: {len(centers)} magnets")
        elif layout == "rings":
            centers, layout_meta = rings_layout(
                object_radius, axis_min, axis_max,
                magnet_diameter, magnet_thickness,
                min_wall, n_rings=n_rings, ring_spacing=ring_spacing,
                ring_rotation_offset=ring_rotation_offset, axis=axis,
                max_magnets=max_magnets,
            )
            print(f"Rings layout: {layout_meta.get('n_rings')}x{layout_meta.get('n_per_ring')} "
                  f"= {layout_meta.get('total_magnets')} magnets, "
                  f"spacing={layout_meta.get('ring_spacing_mm')} mm, "
                  f"rotation offset={layout_meta.get('ring_rotation_offset_deg')}°")
        else:
            n, layout_radius = choose_magnet_count(
                object_radius,
                magnet_diameter,
                min_wall,
                max_count=max_magnets,
            )
            print(f"Circular layout: {n} magnets")
            centers = circular_layout(layout_radius, n, [0, center_y, 0], axis) if n > 0 else []
            layout_meta = {"n_magnets": n, "layout_radius_mm": round(layout_radius, 3)} if n > 0 else {}

        for i, c in enumerate(centers):
            cavity, shell, chamfer = create_magnet_pouch(
                f"magnet{i}",
                magnet_diameter,
                magnet_thickness,
                c,
                axis,
                tolerance,
            )
            boolean("micro", cavity, "DIFFERENCE")
            boolean("micro", chamfer, "DIFFERENCE")
            boolean("micro", shell, "UNION")
    else:
        print("Skipping magnet pouches (--no-pouches)")

    if cap_thickness > 0:
        add_base_cap("micro", cap_thickness, axis)

    if args.output.lower().endswith(".stl"):
        bpy.ops.export_mesh.stl(filepath=args.output)
    elif args.output.lower().endswith(".obj"):
        bpy.ops.wm.obj_export(filepath=args.output)
    else:
        raise ValueError("Output must be .stl or .obj")
    print("Exported:", args.output)

    # --- write manifest ---
    manifest = {
        "print_id": args.print_id or Path(args.output).stem,
        "notes": args.notes or "",
        "generated_at": datetime.utcnow().strftime("%Y-%m-%dT%H:%M:%SZ"),
        "input": args.input,
        "output": args.output,
        "config_file": args.config or "(none)",
        "sensor": {
            "axis": axis,
            "cap_thickness_mm": cap_thickness,
        },
        "magnet": {
            "diameter_mm": magnet_diameter,
            "thickness_mm": magnet_thickness,
            "tolerance_mm": tolerance,
            "fit_mode": fit_mode,
            "min_wall_mm": min_wall,
            "layout": layout if not args.no_pouches else "none",
            **layout_meta,
        },
        "mesh": {
            "E": mesh_cfg.get("E"),
            "nu": mesh_cfg.get("nu"),
            "relative_cell_size": mesh_cfg.get("relative_cell_size"),
        },
    }
    manifest_path = str(Path(args.output).with_suffix(".manifest.json"))
    with open(manifest_path, "w") as f:
        json.dump(manifest, f, indent=2)
    print("Manifest:  ", manifest_path)


if __name__ == "__main__":
    main()
