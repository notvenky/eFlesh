#! /bin/bash
#
# Submit job as (build defaults to Release):
#
#   sbatch compile.sh
#   sbatch --export=BUILD='Debug',ALL compile.sh
#
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=16
#SBATCH --time=2:00:00
#SBATCH --mem=32GB

# Load modules
module purge
module load gcc/10.2.0
module load boost/intel/1.74.0
module load mpfr/gcc/4.1.0

export GMP_DIR=$GMP_ROOT
export MPFR_DIR=$MPFR_ROOT


# Run job
cd "${SLURM_SUBMIT_DIR}"

# Compile main program
SOURCE_DIR=/scratch/${USER}/microstructure_inflators
#BUILD_DIR=build
BUILD_DIR=/scratch/${USER}/microstructure_inflators/build

# mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}

export CC=/share/apps/gcc/10.2.0/bin/gcc
export CXX=/share/apps/gcc/10.2.0/bin/g++

# mkdir -p ${BUILD}
# pushd ${BUILD}
cmake -DTBB_ROOT="/scratch/zh1476/oneTBB/tbb-install/" -DMPFR_INCLUDE_DIR="/share/apps/mpfr/4.1.0/gcc/include" -DMPFR_LIBRARIES="/share/apps/mpfr/4.1.0/gcc/lib/libmpfr.so" -DCMAKE_BUILD_TYPE=release -DLIBIGL_WITH_OPENGL=OFF ${SOURCE_DIR}
make -j16
# popd
