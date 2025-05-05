# Polyfem Solvers (https://github.com/AcademySoftwareFoundation/openvdb.git)
# License: MIT

if(TARGET openvdb::openvdb)
    return()
endif()

message(STATUS "Third-party: creating target 'openvdb::openvdb'")

set(OPENVDB_BUILD_BINARIES OFF CACHE BOOL " " FORCE)
set(USE_BLOSC OFF CACHE BOOL " " FORCE)
set(OPENVDB_ENABLE_RPATH OFF CACHE BOOL " " FORCE)
set(USE_CCACHE OFF CACHE BOOL " " FORCE)
set(USE_PKGCONFIG OFF CACHE BOOL " " FORCE)
set(USE_EXR OFF CACHE BOOL " " FORCE)
set(USE_EXPLICIT_INSTANTIATION OFF CACHE BOOL " " FORCE)

option(OPENVDB_CORE_SHARED "Build dynamically linked version of the core library." OFF)
option(OPENVDB_ENABLE_UNINSTALL "Adds a CMake uninstall target." OFF)

include(CPM)
CPMAddPackage("gh:AcademySoftwareFoundation/openvdb#a4d4c5fe63cfe5dc3c7cdcc6439474af02490d92")
