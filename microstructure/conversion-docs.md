# CAD-to-eFlesh Conversion Documentation

#####
<div align="center">
    <a href="https://github.com/notvenky/eFlesh/blob/main/microstructure/microstructure_inflators/cut-cell.ipynb"><img src="https://img.shields.io/static/v1?label=Stage%201&message=CAD-to-Lattice&color=white"></a> &ensp;
    <a href="https://www.tinkercad.com/things/aekbuRLt2Mz-fantabulous-bigery/edit?returnTo=https%3A%2F%2Fwww.tinkercad.com%2Fdashboard%2Fdesigns%2F3d&sharecode=zKVxHblWRNvldYdELXpionVA-Yl_7HfavL0uF0SSqkA"><img src="https://img.shields.io/static/v1?label=Stage%202&message=Add%20Pouches&color=lightblue"></a> &ensp;
    <a href=""><img src="https://img.shields.io/static/v1?label=Stage%203&message=Add%20Slot&color=skyblue"></a> &ensp;
    <a href="https://github.com/SoftFever/OrcaSlicer"><img src="https://img.shields.io/static/v1?label=Stage%204&message=3D-Print&color=blue"></a> &ensp;
</div>

The conversion from an input CAD file to a fully fabricated eFlesh sensor goes through 4 stages: 

## Stage 1: Get Lattice

- Use our <a href="https://github.com/notvenky/eFlesh/blob/main/microstructure/microstructure_inflators/cut-cell.ipynb">notebook</a> - ```cut-cell.ipynb```, and specify the path to your input .obj or .stl file in the ```input_surface```.

- We recommend that the Poisson's ratio ```nu``` is kept constant at 0.09. However, feel free to change the ```cell_size``` and Young's modulus ```E``` based on the desired application and sensor response.

- The Young's modulus ```E``` can be varied per layer under the ```def young``` function. ```k``` represents the layers through depth, where 0 denotes the bottommost layer.

- Following this, proceed with completing the conversion. The output is a lattice in the shape of the input file.

## Stage 2: Add Pouches

### Option 1: Blender
- Requires Blender installation: Use ```create_pouch.py```

- Instructions to run this script: 
    - Blender -b -P create_pouch.py
    - The arguments to be specified are 
    ```
    input_path = "/path/to/input.obj"
    output_path = "/desired/output.obj"
    list_of_magnets = [
        [15, 3.2, [15, 10, 1]], # [diameter, thickness, [centerX, centerY, centerZ]]
        [9 ,   4, [30, 0, 5]]
    ]
    ```

### Option 2: TinkerCAD
- Use TinkerCAD, a browser-based application.

- In this TinkerCAD <a href="https://www.tinkercad.com/things/aekbuRLt2Mz-fantabulous-bigery/edit?returnTo=https%3A%2F%2Fwww.tinkercad.com%2Fdashboard%2Fdesigns%2F3d&sharecode=zKVxHblWRNvldYdELXpionVA-Yl_7HfavL0uF0SSqkA">workplane</a>, we provide two different pouches for magnets of two different dimensons. 

- Click on 'Import' on the top right, and select your lattice. 
    - If the size or number of triangles exceeds the limit, simplify the mesh on OrcaSlicer (Decimate $\rightarrow$ <300,000). Once the number of triangles is less than 300,000, save that mesh and you should now be able to import the file.

- As per your desired magnet placement, make copies of the pouches and place them relative the lattice. After all pouches are correctly positioned with respect to the lattice. Select the full geoemtry and click on 'Group' (Shortcut: Ctrl+G).

- The output is now a lattice with the press-fit pouches at the chosen spots.
: 
## Stage 3: Add Slot for Hall Sensor

- We strongly recommend that the slot is either fabricated with a rigid filament like PLA, ABS, etc or in a 90-100% infill region if TPU. The slot should definitely not be within the lattice region.

### Option 1: OnShape
- We provide a simple sketch of our magnetometer circuit board on <a href="">OnShape</a>. You can copy the sketch and attach it to the circuit region of your eFlesh design. 

### Option 2: TinkerCAD
- In the same TinkerCAD <a href="https://www.tinkercad.com/things/aekbuRLt2Mz-fantabulous-bigery/edit?returnTo=https%3A%2F%2Fwww.tinkercad.com%2Fdashboard%2Fdesigns%2F3d&sharecode=zKVxHblWRNvldYdELXpionVA-Yl_7HfavL0uF0SSqkA">workplane</a>, we also provide a negative Boolean shape, in the dimensions of the magnetometer PCB we use. While this is for an easy reference, we recommend using OnShape or any CAD software of your choice to design this portion of the eFlesh sensor.

## Stage 4: 3D Print

- We use <a href="https://github.com/SoftFever/OrcaSlicer">OrcaSlicer</a>, an open-source slicing software to slice the generated .stl file, and to 3D print it.

- Correctly configure your 3D printer and TPU filament (we use Bambu X1C (0.4mm) and Polymaker 95A TPU Blue). We use a custom preset for filament settings as shown in the image, to minimize TPU stringing.

- Although the lattice is 3D printable without supports, if you need supports for the magnetometer region, turn on 'Supports (Manual)' and 'Paint Supports' on the regions that are overhangs. Following this, click on 'Slice'.

- The Slicer will automatically switch to the print preview, and the print layers will be visible. Scroll to identify the last layer of the pouch. Go one layer higher from this, and right click on the scroll-bar. Click on 'Add Pause' as shown in the following image, This inserts a pause at the start of that particular layer.

- Start the print. OrcaSlicer notifies upon the pause. You can then insert the magnets and hit 'Resume' to complete the eFlesh fabrication!