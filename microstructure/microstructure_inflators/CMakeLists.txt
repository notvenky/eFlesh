################################################################################
# General Informations
################################################################################

cmake_minimum_required(VERSION 3.2)
project(Microstructures)

# CMP0063: Honor visibility properties for all target types.
if(POLICY CMP0063)
    cmake_policy(SET CMP0063 NEW)
endif()

# Detects whether this is a top-level project
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(MICRO_TOPLEVEL_PROJECT ON)
else()
    set(MICRO_TOPLEVEL_PROJECT OFF)
endif()

################################################################################

set(MICRO_EXTERNAL ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(CMAKE_CXX_FLAGS_RELWITHASSERT        "-O3" CACHE STRING "Flags used during RelWithAssertions builds" FORCE)
set(CMAKE_C_FLAGS_RELWITHASSERT          "-O3" CACHE STRING "Flags used during RelWithAssertions builds" FORCE)
set(CMAKE_EXE_LINKER_FLAGS_RELWITHASSERT    "" CACHE STRING "Flags used during RelWithAssertions builds" FORCE)
set(CMAKE_SHARED_LINKER_FLAGS_RELWITHASSERT "" CACHE STRING "Flags used during RelWithAssertions builds" FORCE)
set(CMAKE_MODULE_LINKER_FLAGS_RELWITHASSERT "" CACHE STRING "Flags used during RelWithAssertions builds" FORCE)
set(CMAKE_STATIC_LINKER_FLAGS_RELWITHASSERT "" CACHE STRING "Flags used during RelWithAssertions builds" FORCE)

mark_as_advanced(
    CMAKE_CXX_FLAGS_RELWITHASSERT
    CMAKE_C_FLAGS_RELWITHASSERT
    CMAKE_EXE_LINKER_FLAGS_RELWITHASSERT
    CMAKE_SHARED_LINKER_FLAGS_RELWITHASSERT
    CMAKE_MODULE_LINKER_FLAGS_RELWITHASSERT
    CMAKE_STATIC_LINKER_FLAGS_RELWITHASSERT
)

if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, default to RelWithAssert")
    set(CMAKE_BUILD_TYPE "RelWithAssert")
endif()

set(CMAKE_BUILD_TYPE "${CMAKE_BUILD_TYPE}" CACHE STRING "Choose the type of build. Options are: None Debug Release RelWithDebInfo MinSizeRel RelWithAssert" FORCE)

# Color output
include(UseColors)

# C++11/14 features
include(CXXFeatures)

# Export compile flags(used for autocompletion of the C++ code)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

# CMake plugin for vscode
include(CMakeToolsHelpers OPTIONAL)

# Helper functions
include(MicroUtils)

################################################################################

# Find out if we are on Ubuntu
set(OS_IS_UBUNTU OFF)
if(UNIX AND NOT APLE)
    if(EXISTS "/etc/issue")
        file(READ "/etc/issue" ETC_ISSUE)
        string(REGEX MATCH "Debian|Ubuntu" DIST ${ETC_ISSUE})
        if(DIST STREQUAL "Ubuntu")
            set(OS_IS_UBUNTU ON)
        endif()
    endif()
endif()

# Microstructure options
option(MICRO_WITH_OPENVDB    "Use openvdb"                                                          ON)
option(MICRO_COPY_HEADERS    "Copy headers into build directory; for debugging purposes"            OFF)
option(MICRO_WITH_SANITIZERS "Enable sanitizers in compilation targets"                             OFF)
option(MICRO_FAST_COMPILE    "Speed up compilation by excluding all targets from the default build" OFF)
option(MICRO_WITH_COTIRE     "Use precompiled headers to speed up the build"                        OFF)
option(MICRO_WITH_TBB        "Compile with TBB"                                                     ON)
option(MICRO_WITH_CERES      "Compile with Ceres"                                                   ON)
option(MICRO_BUILD_TOOLS     "Build tools, e.g. voxel2mesh"                                         OFF)
option(MICRO_BUILD_BINARIES  "Build executable programs" ${MICRO_TOPLEVEL_PROJECT})
option(MICRO_WITH_UBUNTU     "Enable hack around GCC bug on Ubuntu wtih wjacob/tbb 2017" ${OS_IS_UBUNTU})

# Options for libigl modules
option(LIBIGL_USE_STATIC_LIBRARY          "Use libigl as static library" OFF)
option(LIBIGL_WITH_ANTTWEAKBAR            "Use AntTweakBar"              OFF)
option(LIBIGL_WITH_CGAL                   "Use CGAL"                     OFF)
option(LIBIGL_WITH_COMISO                 "Use CoMiso"                   OFF)
option(LIBIGL_WITH_CORK                   "Use Cork"                     OFF)
option(LIBIGL_WITH_EMBREE                 "Use Embree"                   OFF)
option(LIBIGL_WITH_LIM                    "Use LIM"                      OFF)
option(LIBIGL_WITH_MATLAB                 "Use Matlab"                   OFF)
option(LIBIGL_WITH_MOSEK                  "Use MOSEK"                    OFF)
option(LIBIGL_WITH_OPENGL                 "Use OpenGL"                   OFF)
option(LIBIGL_WITH_OPENGL_GLFW            "Use GLFW"                     OFF)
option(LIBIGL_WITH_OPENGL_GLFW_IMGUI      "Use ImGui"                    OFF)
option(LIBIGL_WITH_PNG                    "Use PNG"                      OFF)
option(LIBIGL_WITH_PYTHON                 "Use Python"                   OFF)
option(LIBIGL_WITH_TETGEN                 "Use Tetgen"                   ON)
option(LIBIGL_WITH_TRIANGLE               "Use Triangle"                 ON)
option(LIBIGL_WITH_VIEWER                 "Use OpenGL viewer"            ON)
option(LIBIGL_WITH_XML                    "Use XML"                      OFF)

option(SANITIZE_ADDRESS   "Sanitize Address"       OFF)
option(SANITIZE_MEMORY    "Sanitize Memory"        OFF)
option(SANITIZE_THREAD    "Sanitize Thread"        OFF)
option(SANITIZE_UNDEFINED "Sanitize Undefined"     OFF)

################################################################################

# set(MICRO_WITH_TBB OFF CACHE BOOL "" FORCE)
# set(MICRO_FAST_COMPILE ON CACHE BOOL "" FORCE)
# set(MICRO_WITH_SANITIZERS ON CACHE BOOL "" FORCE)
# set(SANITIZE_ADDRESS   ON  CACHE BOOL "" FORCE)
# set(SANITIZE_MEMORY    OFF CACHE BOOL "" FORCE)
# set(SANITIZE_THREAD    OFF CACHE BOOL "" FORCE)
# set(SANITIZE_UNDEFINED OFF  CACHE BOOL "" FORCE)

################################################################################
# Dependencies
################################################################################

# Sanitizers
if(MICRO_WITH_SANITIZERS)
	list(APPEND CMAKE_MODULE_PATH ${MICRO_EXTERNAL}/sanitizers-cmake/cmake)
endif()

# Other dependencies
include(MicroDependencies)

# Cotire
if(MICRO_WITH_COTIRE)
    list(APPEND CMAKE_MODULE_PATH ${MICRO_EXTERNAL}/cotire/CMake)
    include(cotire)
endif()

################################################################################
# Subfolders
################################################################################

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/isosurface_inflator)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/MeshFEM)

add_subdirectory(src/lib)

if(MICRO_BUILD_BINARIES)
    add_subdirectory(src/bin)
endif()

if(MICRO_BUILD_TOOLS)
    add_subdirectory(tools/voxel2mesh)
    add_subdirectory(tools/rbf2mesh)
    add_subdirectory(tools/clean_mesh)
endif()
