FAQ
===

##### Q1: Does the input pattern `foo.obj` need to have a unit bbox? What happens if it is a periodic structure but with vertices ranging from 0 ot 100 e.g.

Graphs are rescaled to the cube $$[-1, 1]^3$$ immediately after reading them.

##### Q2: How to set the `initial_params` as well as `radiusBounds` / `translationBounds` / `blendingBounds`? Do you really need the `translationBounds`?

I'd recommend using `pattern_optimization/GenIsosurfaceJob` to specify bounds/initial parameters:

```
GenIsosurfaceJob $MICRO_DIR/patterns/3D/reference_wires/pattern0646.wire  -r'0.04,0.2' -o'-0.3,0.3' -e '1,0'
```

Here I'm applying `offsetBounds` instead of `translationBounds`. These constrain each vertex coordinate to remain within 0.3 of its original value from the `.wire` file.
`GenIsosurfaceJob` translates this constraint into a pair of bounds on each translation variable, as you can see in the output. These per-variable bounds will override the `translationBounds`.

`GenIsosurfaceJob` also creates an initial parameter vector for you: it positions each vertex as in the `.wire` file and assigns all vertices a default radius and blending parameter of 0.07 and 0.01 respectively.

##### Q3: What is the effect of the parameter `--usePthRoot`?

Passing `--usePthRoot` changes the objective from $$\int_\Omega \mathrm{wcs}^p dx$$ to $$(\int_\Omega \mathrm{wcs}^p dx)^{1/p}$$. Using the p^th root prevents the objective from blowing up as the $$p$$ parameter is increased, simplifying parameter tuning for multi-objective optimization (e.g., if you wanted to add `--proximityRegularizationWeight`). But if you're simply simply minimizing the worst-case stress objective with an equality constraint on the elasticity tensor, omitting `-P` probably will work.

##### Q4: Do you need to fiddle around with the meshing options (-M), or did you use the same ones for all your tests?

I used the same meshing options for all 3D examples in the paper. Do you want the 2D or 3D ones?

##### Q5: Once you've run `WCSOptimization_cli`, you are left on `std::cout` with a `Final p: <whatever>`, which I am assuming are the final result produced by the optimizer.

I generally didn't run the optimization to completion (partially because the runtimes on HPC varied wildly; on the old cluster, some nodes would occasionally run 2-10x slower for no apparent reason). Instead, I searched through the optimizer output to find the best stress reduction among the iterates within some threshold distance of the target moduli.

##### Q6: It seems that `isosurface_inflator/` can be used to recover the final microstructure mesh (and not just the symmetry tet/triangle)? If so, could you indicate how you run it, given the results from `WCSOptimization` (for both 2D and 3D)?

Yes, you can use `isosurface_cli` to create a mesh for the optimal parameters (which you pass using the `--params` flag):

```
isosurface_cli 2D_orthotropic $MICRO_DIR//patterns/2D/topologies/0098.obj --params "<whitespace-separated params>" output_mesh.msh -m meshing_options.json
```

If you already have a mesh of the orthotropic cell (possibly with associated scalar/vector fields), you can use `isosurface_inflator/replicate` to produce the full period cell mesh. But, apart from visualization/printing, the orthotropic cell mesh should suffice for most operations (as long as you pass the `-O` flag to `PeriodicHomogenization_cli`, `WCS_cli`, `ConstStrainDisplacement_cli`, etc).

##### Q7: What is the difference between the options `reflectiveInflator` and `generateFullPeriodCell` in `IsosurfaceInflator`? Aren't they redundant?

They’re not redundant, but perhaps they should be named better.

- If `generateFullPeriodCell` and `reflectiveInflator` are true, then if the pattern has orthotropic symmetries, the positive quadrant/octant are meshed and then reflected to the full period cell.
- If `generateFullPeriodCell` is true and `reflectiveInflator` is false, then the full period cell is meshed directly.
- If `generateFullPeriodCell` is false and `reflectiveInflator` is true, then only the positive quadrant/octant mesh is returned.
- Can’t remember what happens in the false/false case.
