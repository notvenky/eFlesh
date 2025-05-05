<!-- MarkdownTOC autolink="true" bracket="round" depth=3 -->
<!-- /MarkdownTOC -->

[![Build Status](https://travis-ci.com/geometryprocessing/microstructures.svg?token=euzAY1sxC114E8ufzcZx&branch=master)](https://travis-ci.com/geometryprocessing/microstructures)

Dependencies included directly or as submodules:

- MeshFEM
- [NLopt](https://github.com/stevengj/nlopt), optional
- [Ceres](https://github.com/ceres-solver/ceres-solver)
- CGAL, required

Dependencies *not* included:

- Boost, required
- Dlib, optional
- Knitro, optional
- VCGLib, optional

TODOS:

- [ ] Replace bfgs from Dlib by [CppNumericalSolvers](https://github.com/PatWie/CppNumericalSolvers)
- [ ] Restore the gitbook doc and clean it up + use a single doc for both MeshFEM and this repo

### Autocover results in 2D

http://julianpanetta.com/2d_isosurface_hull_autocover_results/flipper.html

### Folders

| Folder | Description |
|--------|-------------|
| `Documentation/`        | Draft of documentation. |
| `isosurface_inflator/`  | Generate tet-mesh from a graph labeled with vertex thicknesses. |
| `pattern_optimization/` | Optimize the parameters of a pattern to attain target elastic properties. |
| `patterns/`             | List possible pattern topologies in 2D/3D. |
| `worst_case_stress/`    | Compute the worst-case stress for a given pattern. |
| `slice_supporter/`      | Generate raft + support structures below a microstructure (operates on slice images). |
| `topology_enumeration/` | Combinatorially generate cubic topologies on the base tetrahedron |
