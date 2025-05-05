<!-- MarkdownTOC autolink="true" bracket="round" depth=3 -->
<!-- /MarkdownTOC -->

# MeshFEM package

Here are some examples that could be run in order to make sure the package is working fine and which also gives some intuition about the functions it can provide.

Try the following:

```bash
cd MeshFEM
cd examples/cantilever
zsh gen.sh # Create meshed bar at various resolutions
../../Simulate_cli -m $MICRO_DIR/materials/B9Creator.material  bar_tet_0.msh -b cantilever.bc -o
 sim.msh
```

This simulates a bar clamped on one end and loaded with a downward force on the other (see cantilever.bc for the boundary conditions). To view the result, install Gmsh, create the symlink `/usr/local/bin/gmsh -> /Applications/Gmsh.app/Contents/MacOS/gmsh` and run:

```bash
gmsh -n sim.msh
```

(The `-n` prevents Gmsh from displaying all fields at once.)

`msh_processor` is mainly intended to run on meshes with solution fields like `sim.msh`. E.g., you can
use it to extract the maximum principal stress on each element:

```bash
msh_processor sim.msh -e "stress" --eigenvalues --maxMag # print to stdout
msh_processor sim.msh -e "stress" --eigenvalues --maxMag -o stress_eigenvalues.msh # write to
  scalar field.
```

One can also try the periodic homogenization code:

```bash
./PeriodicHomogenization_cli examples/meshes/cube_cross.msh
```

`PeriodicHomogenization_cli` will fail on `examples/meshes/cylinder_cross.msh` since that mesh is not perfectly periodic.

Another nice function presented in this package is the conversion between a planar graph structure (normally used here with `.wire` or `.obj` extension) to the file type used in Gmsh (`.msh`). This let's you visualize the microstructure pattern that you have and which can be used later in microstructure package. This can be done through the simple command:

```bash
./mesh_convert <input (.wire)> <output (.msh)>
```

Now, run the following example:

```bash
./mesh_convert $MICRO_DIR/patterns/3D/reference_wires/pattern0283.wire pattern0283.msh
gmsh pattern0283.msh
```

(When in Gmsh, use keys <kbd>alt+shift+P</kbd> and <kbd>alt+shift+L</kbd> to show points and lines of the graph).

# Microstructures package

Now, we present the package containing the main applications used for pattern optimization and minimization of worst case stress.

**TODO:**
- [ ] Describe `Isosurface_cli`

## Worst-case stress optimization

**Executable:** `worst_case_stress/WCSOptimization_cli`

This package implements the worst-case microstructure stress analysis and minimization, besides having also the same tools presented in the initial pattern optimization technique.

### Main parameters

- Choose **target properties** of material, like desired elasticity tensor $$C^*$$, Young's module, Poisson's ration and if it is isotropic. The target file is the single input of the method.

  Also, this option let us set initial values for the parameters of the structure (like offset and vertices radius) and apply bound constraints on them.

  Here is an example of a `job.opt` serving as target:
  ```json
  {
    "dim": 2,
    "target": {
        "type": "isotropic",
        "young":   10,
        "poisson": 0.35
    },
    "initial_params": [0.6809516712393716, 0.6280027237433981, 0.5640910410666709, 0.4337430787476562,
        0.08129922564391019, 0.1842173643713042, 0.1325967598554578, 0.1750543806991497,
        0.1945952629563027, 0.009638340078114519, 0.01, 0.01, 0.00826325588295466, 0.009238195522149741],
    "radiusBounds": [0.04, 0.2],
    "translationBounds": [0.1, 0.8],
    "blendingBounds": [0.0078125, 0.2]
  }
  ```

  Notice that setting as target only the `young` and `poisson` values is enough because we are dealing with an isotropic target. However, we can also set a general elasticity tensor and make the type as `"anisotropic"`. See the two examples below, where one is for 2D cases and the second for 3D cases.

  Examples:
    - 2D:
    ```json
    {
        "dim": 2,
        "target": { "type": "anisotropic",
        "material_matrix": [[1, 2, 3],
                            [2, 1, 2],
                            [3, 2, 0.5]]
        },
        "initial_params": [0.6809516712393716, 0.6280027237433981, 0.5640910410666709,
            0.4337430787476562, 0.08129922564391019, 0.1842173643713042, 0.1325967598554578, 0.1750543806991497,
            0.1945952629563027, 0.009638340078114519, 0.01, 0.01, 0.00826325588295466, 0.009238195522149741],
        "radiusBounds": [0.04, 0.2],
        "translationBounds": [0.1, 0.8],
        "blendingBounds": [0.0078125, 0.2]
    }
    ```

    - 3D:
    ```json
    {
        "dim": 3,
        "target": { "type": "anisotropic",
        "material_matrix": [[1, 0, 0, 0, 0, 0],
                            [0, 1, 0, 0, 0, 0],
                            [0, 0, 1, 0, 0, 0],
                            [0, 0, 0, 0.5, 0, 0],
                            [0, 0, 0, 0, 0.5, 0],
                            [0, 0, 0, 0, 0, 0.5]]
        },
        "initial_params": [0.5, 0.3333333333333332593, 0.6666666666666665186, 0.5, 0.5, 0.5, 0.5, 0.5,
            0.3333333333333332593, 0.6666666666666665186, 0.5, 0.5, 0.5, 0.5, 0.5, 0.6666666666666665186,
            0.3333333333333332593, 0.5, 0.5, 0.6666666666666665186, 0.3333333333333332593,
            0.3333333333333332593, 0.6666666666666665186, 0.5, 0.5, 0.6666666666666665186,
            0.3333333333333332593, 0.07000000000000000666, 0.07000000000000000666, 0.07000000000000000666,
            0.07000000000000000666, 0.07000000000000000666, 0.07000000000000000666, 0.07000000000000000666,
            0.07000000000000000666, 0.07000000000000000666, 0.07000000000000000666, 0.07000000000000000666,
            0.07000000000000000666, 0.07000000000000000666, 0.07000000000000000666, 0.07000000000000000666,
            0.01000000000000000021, 0.01000000000000000021, 0.01000000000000000021, 0.01000000000000000021,
            0.01000000000000000021, 0.01000000000000000021, 0.01000000000000000021, 0.01000000000000000021,
            0.01000000000000000021, 0.01000000000000000021, 0.01000000000000000021, 0.01000000000000000021,
            0.01000000000000000021, 0.01000000000000000021, 0.01000000000000000021],
        "radiusBounds": [0.04000000000000000083, 0.2000000000000000111],
        "translationBounds": [0.1000000000000000056, 0.8000000000000000444],
        "blendingBounds": [0.005000000000000000104, 0.2000000000000000111]
    }
    ```


- Choose the **pattern** to be used with `-p <parameter>`. The parameter should be a file containing information about the topology of the structure, like a graph, containing vertices and how they are linked by edges. Examples can be found in `microstructures/patterns/3D/reference_wires`.

  In order to visualize the pattern, you can convert the file (`.wire`) extension into a `.msh` file, which makes it possible to use `gmsh` on it. To make the conversion, use the executable in `MeshFEM` repository called `mesh_convert` as shown below:
  ```bash
  ./mesh_convert input.wire output.msh
  ```

- Choose **base material** with `-m <parameter>`. Here, most of the times we will use the file `B9Creator.material`, which contains information about the base material used in the 3D printer, like density, Young's module and Poisson's ratio. Also, it says if the material is isotropic or not. This file can be found at `microstructures/materials` folder.

- Choose settings of **meshing** used in the process through `-M <parameter>`. With this option, we can define some parameters like limits on edge sizes, ratio between cell radius and edge size and max area of a face.

- Decide between using vertex thickness (`-V`) or edge thickness. The radius of each vertex affect directly the edge thickness, since the convex hull of the vertices (which are like balls) is used to define the edges.

- Set the **weights** for elements considered in the objective function. The most common parameters are:
  - `--WCSWeight <parameter>` decides the weight given for minimizing the worst case stress in the mesh.
    ($$J_{wcs} = (\int_\Omega s^p)^{\frac{1}{p}}$$, where $$p$$ can also be set with `--usePthRoot` and `--pnorm <parameter>` options. See below).

  - `--JSWeight <parameter>` decides the weight given to $$\lVert S(w) - S^* \rVert$$, where $$ S^* $$ is the desired compliance tensor. There is also a constraint $$C(w) = C^*$$ in the optimization problem that can be set using option `--TensorFitConstraint`.

- Choose the **norm** used for the worst case stress, by using `--pnorm <parameter>` plus `--usePthRoot` options.

- Select the **solver** for the opt problem using `--solver <parameter>`. Usually, use `slsqp`.

- Set prefix name for **output** of each iteration with `-o <parameter>`. The idea is that you should be able to see the result of each iteration, for example, by running `gmsh` on the output. For each iteration, a msh file `<parameter>_X` (where `X` is the iteration number) is created.

### Complete example

```bash
./WCSOptimization_cli -p $MICRO_DIR/Documentation/examples/octa_cell.obj -m $MICRO_DIR/materials/B9Creator.material \
        target_tensor_job.opt  -M 2d_meshing_opts.opt --ortho_cell --vertexThickness \
        --WCSWeight 1e-300 --JSWeight 1.0 --TensorFitConstraint \
        --solver slsqp -o it
```

### Use cases

- In the example above, we use a very low worst-case stress objective weight and set the compliance tensor-fitting weight at 1.0. This ensures worst-case stress is still reported at each iterate, but
effectively ignores the stress levels when optimizing (focusing only on fitting the desired elasticity tensor).

- A different configuration would be the following:
```bash
./WCSOptimization_cli -p $MICRO_DIR/Documentation/examples/octa_cell.obj -m $MICRO_DIR/materials/B9Creator.material \
        target_tensor_job.opt  -M 2d_meshing_opts.opt \
        --ortho_cell --vertexThickness --WCSWeight 1.0 --JSWeight 0.0 --pnorm 8 --usePthRoot \
        --TensorFitConstraint  --solver slsqp -o itHighWCSWeight
```
  This example minimizes the worst-case stress only (the compliance tensor does
  not appear in the optimization objective). However, the target compliance
  tensor will still be achieved due to the equality constrained imposed by
  `--TensorFitConstraint`.

### Per-iteration output statistics

The optimizer outputs a list of statistics for each iterate it evaluates.
The precise collection of statistics it reports depends on what objetive terms/constraints are included in the optimization.

| Name | Description |
|------|-------------|
| `p`                             | Pattern parameter vector                                                                                                                                                                                                     |
| `moduli`                        | Elastic moduli (assuming an orthotropic material): Young's moduli, followed by Poisson's ratios, followed by shear moduli                                                                                                    |
| `anisotropy`                    | Anisotropy/Zener ratio (assuming a cubic material): the ratio of the actual shear modulus to the shear modulus of an isotropic material with the same Young's modulus and Poisson's ratio. This is 1 for isotropic tensors.  |
| `printable`                     | Whether the 3D pattern is printable (1) or not (0). 2D iterates always print 1                                                                                                                                               |
| `Rel elasticity tensor dist`    | $$\frac{\lVert E(\omega) - E^* \rVert_F}{\lVert E^* \rVert_F}$$                                                                                                                                                              |
| `JFull`                         | The full combined objective being optimized (a weighted combination of the objective terms listed below)                                                                                                                     |
| `TensorFit violation`           | If the tensor-fitting equality constraint is applied: residual $$S(\omega) - S^*$$ flattened into a 1D vector                                                                                                                |
| `jacobian TensorFit row norms`  | If the tensor-fitting constraint is applied: norm of the gradient of each component of residual $$S(\omega) - S^*$$                                                                                                          |

The following objective-specific terms may appear:

| Name | Description |
|------|-------------|
| `JS`                       | Value of the compliance tensor-fitting objective: $$\lVert S(\omega) - S^*\rVert_F^2$$  |
| `ProximityRegularization`  | Value of $$\lVert p - p^* \rVert^2$$, where $$p^*$$ are the "target" pattern parameters   |
| `Max Ptwise WCS`           | The greatest worst-case stress level appearing anywhere in the mesh (Linf norm)         |
| `WCS`                      | The worst-case stress objective value (Lp norm of worst-case stress field)              |

For each objective present (JFull, JS, ProximityRegularization, WCS, etc.), the following statistics are printed:

| Name | Description |
|------|-------------|
| `normalized <objective>`     | Objective value normalized by the value for the initial iterate.         |
| `grad_p <objective>`         | Gradient of the objective with respect to each pattern parameter         |
| <code>‖grad\_p &lt;objective&gt;‖</code>  | Norm of the objective's gradient with respect to the pattern parameters  |
