# Prepare dependencies
#
# For each third-party library, if the appropriate target doesn't exist yet,
# download it via external project, and add_subdirectory to build it alongside
# this project.

### Configuration
set(MICRO_ROOT     "${CMAKE_CURRENT_LIST_DIR}/..")
set(MICRO_EXTERNAL "${MICRO_ROOT}/3rdparty")

# Download and update 3rdparty libraries
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})
list(REMOVE_DUPLICATES CMAKE_MODULE_PATH)
include(MicroDownloadExternal)

################################################################################
# Required libraries
################################################################################

# Eigen
if(NOT TARGET Eigen3::Eigen)
    add_library(micro_eigen INTERFACE)
    micro_download_eigen()
    target_include_directories(micro_eigen SYSTEM INTERFACE ${MICRO_EXTERNAL}/eigen)
    add_library(Eigen3::Eigen ALIAS micro_eigen)
endif()

# openvdb
if (MICRO_WITH_OPENVDB)
    if(NOT TARGET openvdb)
        include(openvdb)
        get_target_property(realTarget openvdb ALIASED_TARGET)
        target_compile_definitions(${realTarget} INTERFACE -DMICRO_WITH_OPENVDB)
    endif()
endif()

# TBB library; must be brought in before MeshFEM to override! We need tbbmalloc,
# which MeshFEM chooses not to build.
# There are also some segfaults on shutdown with TBB 2017 (the version in wjakob's
# repository), so we need to use a more recent version of TBB.

# While wjakob has been updated to TBB 2019 recently, it seems to hang Travis
# at the linking stage for some reason, so we'll just use the upstream version
# for now.
if(NOT TARGET TBB::tbb)
    micro_download_tbb()
    list(APPEND CMAKE_MODULE_PATH ${MICRO_EXTERNAL}/tbb/cmake)
    include(TBBBuild)
    tbb_build(TBB_ROOT ${MICRO_EXTERNAL}/tbb CONFIG_DIR TBB_DIR)
    find_package(TBB REQUIRED tbb tbbmalloc)
    add_library(tbb_tbb INTERFACE)
    add_library(TBB::tbb ALIAS tbb_tbb)
    target_link_libraries(tbb_tbb INTERFACE TBB::tbb TBB::tbbmalloc)
endif()

if(NOT TARGET TBB::tbb)
    set(TBB_BUILD_STATIC ON CACHE BOOL " " FORCE)
    set(TBB_BUILD_SHARED OFF CACHE BOOL " " FORCE)
    set(TBB_BUILD_TBBMALLOC ON CACHE BOOL " " FORCE)
    set(TBB_BUILD_TBBMALLOC_PROXY OFF CACHE BOOL " " FORCE)
    set(TBB_BUILD_TESTS OFF CACHE BOOL " " FORCE)
    set(TBB_NO_DATE ON CACHE BOOL " " FORCE)

    micro_download_tbb()
    add_subdirectory(${MICRO_EXTERNAL}/tbb tbb EXCLUDE_FROM_ALL)
    set_property(TARGET tbb_static tbb_def_files PROPERTY FOLDER "dependencies")
    set_target_properties(tbb_static PROPERTIES COMPILE_FLAGS "-Wno-implicit-fallthrough -Wno-missing-field-initializers -Wno-unused-parameter -Wno-keyword-macro")

    add_library(tbb_tbb INTERFACE)
    target_include_directories(tbb_tbb SYSTEM INTERFACE ${MICRO_EXTERNAL}/tbb/include)
    target_link_libraries(tbb_tbb INTERFACE tbb_static tbbmalloc_static)
    add_library(TBB::tbb ALIAS tbb_tbb)

    target_compile_definitions(tbb_tbb INTERFACE -DMICRO_WITH_TBB)

    micro_target_hide_warnings(tbb_tbb tbb_static tbbmalloc_static)
endif()

# if(NOT TARGET micro::tbb)
#     add_library(micro_tbb INTERFACE)
#     if(MICRO_WITH_TBB)
#         target_link_libraries(micro_tbb INTERFACE TBB::tbb)
#         target_compile_definitions(micro_tbb INTERFACE -DMICRO_WITH_TBB)
#     endif()
#     add_library(micro::tbb ALIAS micro_tbb)
# endif()

# C++11 threads
find_package(Threads REQUIRED) # provides Threads::Threads

# json library
if(NOT TARGET nlohmann_json::nlohmann_json)
    add_library(meshfem_json INTERFACE)
    micro_download_json()
    target_include_directories(meshfem_json SYSTEM INTERFACE ${MICRO_EXTERNAL}/json)
    target_include_directories(meshfem_json SYSTEM INTERFACE ${MICRO_EXTERNAL}/json/nlohmann)
    add_library(nlohmann_json::nlohmann_json ALIAS meshfem_json)
endif()

# Optional library
if(NOT TARGET optional::optional)
    micro_download_optional()
    add_library(optional_lite INTERFACE)
    target_include_directories(optional_lite SYSTEM INTERFACE ${MICRO_EXTERNAL}/optional/include)
    add_library(optional::optional ALIAS optional_lite)
endif()

# Triangle library
# include(triangle)
if(NOT TARGET triangle::triangle)
    micro_download_triangle()
    add_subdirectory(${MICRO_EXTERNAL}/triangle triangle)
    target_include_directories(triangle SYSTEM INTERFACE ${MICRO_EXTERNAL}/triangle)
    add_library(triangle::triangle ALIAS triangle)
endif()

# MeshFEM library
# if(NOT TARGET MeshFEM)
#     micro_download_meshfem()
#     option(MESHFEM_WITH_CERES "Compile MeshFEM with Ceres" ${MICRO_WITH_CERES})
#     add_subdirectory(${MICRO_EXTERNAL}/MeshFEM MeshFEM)
# endif()

# CLI11 library
if(MICRO_BUILD_BINARIES)
 if(NOT TARGET CLI11::CLI11)
     micro_download_cli11()
     add_library(CLI11 INTERFACE)
     target_include_directories(CLI11 SYSTEM INTERFACE ${MICRO_EXTERNAL}/CLI11/include)
     add_library(CLI11::CLI11 ALIAS CLI11)
 endif()
endif()

# CGAL library
if(NOT TARGET CGAL::CGAL)
    micro_download_cgal()
    set(CGAL_DIR ${MICRO_EXTERNAL}/cgal)
    find_package(CGAL CONFIG REQUIRED COMPONENTS PATHS ${CGAL_DIR} NO_DEFAULT_PATH)
endif()

# Boost library
if(MICRO_BUILD_BINARIES)
    find_package(Boost 1.54 REQUIRED COMPONENTS filesystem system program_options QUIET)
    if(NOT TARGET micro::boost)
        add_library(meshfem_boost INTERFACE)
        if(TARGET Boost::filesystem AND TARGET Boost::system AND TARGET Boost::program_options)
            target_link_libraries(meshfem_boost INTERFACE
                Boost::filesystem
                Boost::system
                Boost::program_options)
        else()
            # When CMake and Boost versions are not in sync, imported targets may not be available... (sigh)
            target_include_directories(meshfem_boost SYSTEM INTERFACE ${Boost_INCLUDE_DIRS})
            target_link_libraries(meshfem_boost INTERFACE ${Boost_LIBRARIES})
        endif()
        add_library(micro::boost ALIAS meshfem_boost)
    endif()
endif()

# Nanoflann
if(NOT TARGET nanoflann::nanoflann)
    micro_download_nanoflann()
    add_library(nanoflann INTERFACE)
    add_library(nanoflann::nanoflann ALIAS nanoflann)
    target_include_directories(nanoflann INTERFACE ${MICRO_EXTERNAL}/nanoflann/include)
endif()

# Accelerate framework
# if(NOT TARGET micro::accelerate)
#     if(APPLE)
#         find_library(AccelerateFramework Accelerate)
#         add_library(micro_accelerate INTERFACE)
#         target_link_libraries(micro_accelerate INTERFACE ${AccelerateFramework})
#         add_library(micro::accelerate ALIAS micro_accelerate)
#     else()
#         add_library(micro::accelerate INTERFACE IMPORTED)
#     endif()
# endif()

# Catch2
# if(NOT TARGET Catch2::Catch2 AND (CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR))
#     micro_download_catch()
#     add_subdirectory(${MICRO_EXTERNAL}/Catch2)
#     list(APPEND CMAKE_MODULE_PATH ${MICRO_EXTERNAL}/Catch2/contrib)
# endif()

# Cotire
if(MICRO_WITH_COTIRE)
    micro_download_cotire()
endif()

################################################################################
# Optional libraries
################################################################################

# libigl library
if(NOT TARGET libigl)
    if(NOT TARGET igl::core)
        micro_download_libigl()
        find_package(LIBIGL QUIET)
    endif()
    if(LIBIGL_FOUND)
        add_library(micro_libigl INTERFACE)
        target_link_libraries(micro_libigl INTERFACE igl::core)
        target_compile_definitions(micro_libigl INTERFACE -DHAS_LIBIGL)
        add_library(libigl ALIAS micro_libigl)
    else()
        add_library(libigl INTERFACE IMPORTED)
    endif()
endif()

# VCG library
if(NOT TARGET micro::vcglib)
    find_package(VCGlib QUIET)
    if(VCGLIB_FOUND)
        add_library(micro_vcglib INTERFACE)
        target_link_libraries(micro_vcglib INTERFACE VCGlib::core)
        target_compile_definitions(micro_vcglib INTERFACE -DHAS_VCGLIB)
        add_library(micro::vcglib ALIAS micro_vcglib)
    else()
        message(STATUS "VCGLib not found; disabling Luigi's Inflator")
        add_library(micro::vcglib INTERFACE IMPORTED)
    endif()
endif()

# Sanitizers
if(MICRO_WITH_SANITIZERS)
    if(NOT COMMAND add_sanitizers)
        micro_download_sanitizers()
    endif()
    find_package(Sanitizers)
endif()
