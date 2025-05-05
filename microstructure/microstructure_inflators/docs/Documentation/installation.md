<!-- MarkdownTOC autolink="true" bracket="round" depth=3 -->
<!-- /MarkdownTOC -->

# On the HPC cluster

### Preparing the environment

First, you need to load the following modules (to either run or compile the code).
You can put this in your `.bash_profile`, or run a script every time you start a new job:

```bash
# Load modules
module purge
module load mercurial/intel/4.0.1
module load cmake/intel/3.7.1
module load gcc/6.3.0

# Microstructure code
module load tbb/intel/2017u3
module load suitesparse/intel/4.5.4
module load eigen/3.3.1
module load boost/intel/1.62.0
module load cgal/intel/4.10b1
module load guile/intel/2.0.14
module load flex/gnu/2.6.4
module load glog/intel/0.3.4
```

You also need to set the `C` and `CXX` compiler in your environment (e.g. in the `bash_profile`), or run CMake every time with the corresponding argument:

```bash
export CC=${GCC_ROOT}/bin/gcc
export CXX=${GCC_ROOT}/bin/g++
```

Or run every CMake call with:

```
cmake -DCMAKE_C_COMPILER=${$GCC_ROOT}/bin/gcc -DCMAKE_CXX_COMPILER=${$GCC_ROOT}/bin/g++ ..
```

### Cloning the mercurial repositories

Logout and log back in to load mercurial module and the rest.
Next, we will clone the mercurial repositories in the current working directory:

```
hg clone https://subversive.cims.nyu.edu/geonum/jpanetta/CSGFEM/
hg clone https://subversive.cims.nyu.edu/geonum/jpanetta/MeshFEM
hg clone https://subversive.cims.nyu.edu/geonum/3DPrint/microstructures/
```

Note that the CIMS mercurial server does not support SSH authentication, so you need if you do not want to type your login/password every time you do an operation, you need to either:

1. Store your CIMS login/password in clear text in your `~/.hgrc`
    ```
    [ui]
    username = My Name <foo.bar@nyu.edu>

    [auth]
    cims.prefix = https://subversive.cims.nyu.edu/geonum/
    cims.username = <login>
    cims.password = <password>
    ```
2. Or use the `mercurial_keyring` extension. You can install the extension using `pip`:
    ```
    pip2.7 install --user mercurial_keyring
    ```

    Or download the file directly:
    ```
    mkdir -p ~/.local/bin
    wget -O ~/.local/bin/mercurial_keyring http://bitbucket.org/Mekk/mercurial_keyring/raw/default/mercurial_keyring.py
    chmod o+x .local/bin/mercurial_keyring
    ```

    In both cases, add the following lines to your `~/.hgrc`:
    ```
    [extensions]
    mercurial_keyring =
    hgext.mercurial_keyring = ~/.local/bin/mercurial_keyring.py
    ```

### Building the program

There are a few dependencies which are not available as modules on the HPC cluster. For your convenience, they are provided as [CMake external projects](https://cmake.org/cmake/help/latest/module/ExternalProject.html). To compile and install them, simply run:

```
pushd microstructures/3rdparty
mkdir build
cd build
cmake ..
make -j2
popd
```

Then, you can compile the main project as usual:

```
cd microstructures
mkdir build
cd build
cmake ..
make -j2
```


# On Archlinux

Here are the packages that I had to install on Archlinux to make it work (`local` means the package is in the AUR, `git` means I manually downloaded a copy from the upstream git repository).

```
community/cgal 4.9-1
core/gmp 6.1.2-1
core/mpfr 3.1.5.p2-1
extra/boost 1.63.0-1
extra/eigen 3.3.3-1
extra/gd 2.2.3-3
extra/intel-tbb 2017_20161128-1
extra/mercurial 4.1.1-1
extra/openmpi 1.10.6-1
extra/python-numpy 1.12.0-1
extra/suitesparse 4.5.4-1
git/Adept
git/libigl
git/PyMesh
git/vcglib
local/ceres-solver 1.12.0-1
local/cork-git r24.5987de5-1
local/dlib 19.4-1
local/f2c 1.0-8
local/gmsh 2.16.0-1
local/levmar 2.6-3
local/libmatheval 1.1.11-1
local/nlopt 2.4.2-2
local/openblas-lapack 0.2.19-1
local/polyclipping 6.4.2-1
local/triangle 1.6-6
```

