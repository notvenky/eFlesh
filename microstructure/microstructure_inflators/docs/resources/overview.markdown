Overview
==================

[Algorithm pipeline](images/pipeline.png)  
In this diagram:  
Rectangles represent code/action.  
Parallelograms represent input/output.

Homogenization
--------------

[Numerical Coarsening](http://www.geometry.caltech.edu/pubs/KMOD09.pdf) (Caltech)  
[Periodic homogenization](../periodic_homogenization/periodic_homogenization.pdf)  

Material Property Optimization
------------------------------

[Material Gradient Derivation](https://dl.dropboxusercontent.com/u/29899857/material_opt.pdf)  
Local/Global optimization: TODO

Pattern Optimization
--------------------

[Objective Functions For Pattern Optimization](../pattern_optimization/objective/objective.pdf)  
Material Property to Pattern Parameter Lookup: TODO  
[Shape Derivative for Functions of Homogenized Elasticity Tensors](../pattern_optimization/shape_derivative/shape_derivative.pdf)

Wire Inflator
-------------

Inspired by the "Strut Algorithm" in
[Sculptural Forms from Hyperbolic Tessellations](http://georgehart.com/echinoderms/hart.pdf)

Pattern parameters are extracted to retain orthotropic symmetries.  
Brick5: [symmetry orbits](https://dl.dropboxusercontent.com/u/29899857/brick5_parameters.pdf)  
Double star: [symmetry orbits](https://dl.dropboxusercontent.com/u/29899857/star_parameters.pdf)

Implementation
--------------
[Tensor Flattening](external_writeups/TensorFlattening.pdf) (Describes flattening conventions used in all of Julian's code)  

Old Writeups
------------
[Low Dimensional Shape Optimization](external_writeups/shape_opt.pdf)
