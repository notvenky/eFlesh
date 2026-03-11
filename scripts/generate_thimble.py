import bpy # from blender
import sys
import os
import math
import argparse

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
    object_radius, magnet_diameter, min_wall, min_clearance=0.5, pouch_wall=1.0
):
    magnet_radius = magnet_diameter / 2.0
    # pouch center must be inset by magnet_radius + shell wall + min_wall
    available_radius = object_radius - magnet_radius - pouch_wall - min_wall

    if available_radius <= 0:
        return 0, 0

    # actual footprint of each pouch (magnet + shell on each side)
    pouch_diameter = magnet_diameter + 2 * pouch_wall

    for n in [4, 3, 2]:
        spacing = 2 * available_radius * math.sin(math.pi / n)
        if spacing > pouch_diameter + min_clearance:
            return n, available_radius

    return 1, available_radius


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
                   min_wall, min_clearance=1.0, pouch_wall=1.0, axis="Y"):
    """Single column of magnets stacked along the axis at maximum radial offset."""
    radial_offset = object_radius - magnet_diameter / 2.0 - pouch_wall - min_wall

    if radial_offset <= 0:
        print("Warning: object too small for stacked layout")
        return []

    step = magnet_thickness + min_clearance
    axis_length = axis_max - axis_min
    n = max(1, int(axis_length / step))

    # centre the stack along the axis
    axis_center = (axis_min + axis_max) / 2.0
    start = axis_center - (n - 1) * step / 2.0

    centers = []
    for i in range(n):
        a = start + i * step
        if axis == "Y":
            centers.append([radial_offset, a, 0])
        elif axis == "Z":
            centers.append([radial_offset, 0, a])
        elif axis == "X":
            centers.append([0, radial_offset, a])

    return centers


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
    parser.add_argument("--layout", default=None, choices=["circular", "stacked"])

    args = parser.parse_args(argv)

    # load config file defaults, then apply CLI overrides
    cfg = load_config(args.config) if args.config else {}
    magnet_cfg = cfg.get("magnet", {})
    sensor_cfg = cfg.get("sensor", {})

    magnet_diameter  = args.magnet_diameter  or magnet_cfg.get("diameter",  9.0)
    magnet_thickness = args.magnet_thickness or magnet_cfg.get("thickness", 3.0)
    min_wall         = args.min_wall         or magnet_cfg.get("min_wall",  1.0)
    tolerance        = args.tolerance        or magnet_cfg.get("tolerance", 0.25)
    fit_mode         = args.fit_mode         or magnet_cfg.get("fit_mode",  "press")
    cap_thickness    = args.cap_thickness    or sensor_cfg.get("cap_thickness", 1.0)
    axis             = args.axis             or sensor_cfg.get("axis", "Y")
    layout           = args.layout           or magnet_cfg.get("layout", "circular")

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

    if not args.no_pouches:
        if layout == "stacked":
            axis_vals = {"Y": (min(v.y for v in bbox), max(v.y for v in bbox)),
                         "Z": (min(v.z for v in bbox), max(v.z for v in bbox)),
                         "X": (min(v.x for v in bbox), max(v.x for v in bbox))}
            axis_min, axis_max = axis_vals[axis]
            centers = stacked_layout(
                object_radius, axis_min, axis_max,
                magnet_diameter, magnet_thickness,
                min_wall, axis=axis,
            )
            print(f"Stacked layout: {len(centers)} magnets")
        else:
            n, layout_radius = choose_magnet_count(
                object_radius,
                magnet_diameter,
                min_wall,
            )
            print(f"Circular layout: {n} magnets")
            centers = circular_layout(layout_radius, n, [0, center_y, 0], axis) if n > 0 else []

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


if __name__ == "__main__":
    main()
