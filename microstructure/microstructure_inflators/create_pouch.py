import bpy, argparse
import numpy as np

# Function to create a cylinder with a specified radius
def create_cylinder(name, radius, depth, location=(0, 0, 0)):
    bpy.ops.mesh.primitive_cylinder_add(radius=radius, location=location, depth=depth, vertices=128)
    obj = bpy.context.object
    obj.name = name
    return obj

# Function to perform a boolean difference operation
def boolean(obj1_name, obj2_name, operation_type = "DIFFERENCE"):
    # Make sure both objects are selected for boolean operation
    bpy.context.view_layer.objects.active = bpy.context.scene.objects[obj1_name]
    # bpy.context.scene.objects[obj1_name].select_set(True)
    # bpy.context.scene.objects[obj2_name].select_set(True)

    # Add a Boolean modifier to the first object
    mod = bpy.context.scene.objects[obj1_name].modifiers.new(name="Boolean", type='BOOLEAN')
    mod.operation = operation_type
    mod.use_self = False
    mod.object = bpy.context.scene.objects[obj2_name]  # Set the second object as the operand

    # Apply the modifier
    # print(mod.name)
    bpy.ops.object.modifier_apply(modifier=mod.name, report=True)

    # Delete the second object as it's no longer needed
    bpy.data.objects.remove(bpy.context.scene.objects[obj2_name])
    # bpy.context.scene.objects[obj2_name].select_set(False)

# Function to export the resulting mesh to a file (e.g., .obj or .stl)
def export_mesh(filepath):
    bpy.ops.wm.obj_export(filepath=filepath)

def create_pouch(magnet_diameter, magnet_thickness, center = [0, 0, 0], name = "pouch"):
    ref_magnet_thickness = 3.175
    ref_magnet_diameter = 9.525

    midXY = 5 * magnet_diameter / ref_magnet_diameter
    outerXY = midXY + 1
    midZ = 3 * magnet_thickness / ref_magnet_thickness
    outerZ = midZ + 2

    lidXY = 4 * magnet_diameter / ref_magnet_diameter
    lidZ = midZ + 1

    convex_hull = create_cylinder(name + "_convex", radius=outerXY * 0.99, depth=outerZ * 0.99, location=(0, 0, 0))
    pouch_obj = create_cylinder(name, radius=outerXY, depth=outerZ, location=(0, 0, 0))
    cylinder1 = create_cylinder(name + "_cylinder1", radius=midXY, depth=midZ, location=(0, 0, 0))
    cylinder3 = create_cylinder(name + "_cylinder3", radius=lidXY, depth=lidZ, location=(0, 0, 1.9 + midZ / 2.))

    # Perform the boolean difference operation
    boolean(name, name + "_cylinder1", "DIFFERENCE")
    boolean(name, name + "_cylinder3", "DIFFERENCE")

    bpy.context.scene.objects[name + "_convex"].location = [center[0], center[1], center[2] + outerZ / 2.]
    bpy.context.scene.objects[name].location = [center[0], center[1], center[2] + outerZ / 2.]

    return name + "_convex", name


if __name__ == "__main__":

    input_path = "micro.obj"
    output_path = "boolean.obj"
    list_of_magnets = [
        [15, 3.2, [15, 10, 1]], # [diameter, thickness, [centerX, centerY, centerZ]]
        [9 ,   4, [30, 0, 5]]
    ]

    # Clear existing objects (optional)
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete()

    bpy.ops.wm.obj_import(filepath=input_path)
    imported_object = bpy.context.view_layer.objects.active
    imported_object.name = "micro"

    for index, magnet in enumerate(list_of_magnets):
        convex_hull, pouch_obj = create_pouch(magnet[0], magnet[1], magnet[2], "pouch" + str(index))
        boolean("micro", convex_hull, "DIFFERENCE")
        boolean("micro", pouch_obj, "UNION")

    # Export the result to an .obj file
    export_mesh(output_path)
