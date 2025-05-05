################################################################################
include(DownloadProject)

# With CMake 3.8 and above, we can hide warnings about git being in a
# detached head by passing an extra GIT_CONFIG option
if(NOT (${CMAKE_VERSION} VERSION_LESS "3.8.0"))
    set(MICRO_EXTRA_OPTIONS "GIT_CONFIG advice.detachedHead=false")
else()
    set(MICRO_EXTRA_OPTIONS "")
endif()

# Shortcut function
function(micro_download_project name)
    download_project(
        PROJ         ${name}
        SOURCE_DIR   ${MICRO_EXTERNAL}/${name}
        DOWNLOAD_DIR ${MICRO_EXTERNAL}/.cache/${name}
        QUIET
        ${MICRO_EXTRA_OPTIONS}
        ${ARGN}
    )
endfunction()

################################################################################

## MeshFEM
# function(micro_download_meshfem)
#     micro_download_project(MeshFEM
#         GIT_REPOSITORY git@github.com:geometryprocessing/MeshFEM.git
#         GIT_TAG        e0d91a50b194ba2d14a32af4fe3c16bd7f825897
#     )
# endfunction()

## Eigen
function(micro_download_eigen)
    micro_download_project(eigen
        GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
        GIT_TAG tags/3.4.0
    )
endfunction()

## Json
function(micro_download_json)
    micro_download_project(json
        URL      https://github.com/nlohmann/json/releases/download/v3.1.2/include.zip
        URL_HASH SHA256=495362ee1b9d03d9526ba9ccf1b4a9c37691abe3a642ddbced13e5778c16660c
    )
endfunction()

## openvdb
function(micro_download_openvdb)
    micro_download_project(openvdb
        GIT_REPOSITORY  https://github.com/AcademySoftwareFoundation/openvdb.git
        GIT_TAG         a4d4c5fe63cfe5dc3c7cdcc6439474af02490d92
    )
endfunction()

## Optional
function(micro_download_optional)
    micro_download_project(optional
        URL     https://github.com/martinmoene/optional-lite/archive/v3.0.0.tar.gz
        URL_MD5 a66541380c51c0d0a1e593cc2ca9fe8a
    )
endfunction()

## Tinyexpr
function(micro_download_tinyexpr)
    micro_download_project(tinyexpr
        GIT_REPOSITORY https://github.com/codeplea/tinyexpr.git
        GIT_TAG        ffb0d41b13e5f8d318db95feb071c220c134fe70
    )
endfunction()

## Triangle
function(micro_download_triangle)
    micro_download_project(triangle
        GIT_REPOSITORY https://github.com/Huangzizhou/triangle.git
        GIT_TAG        c80ac9a5efd0c199d7cda4730489391f2ed40178
    )
endfunction()

## TBB
function(micro_download_tbb)
    # if(MICRO_WITH_UBUNTU)
        micro_download_project(tbb
            GIT_REPOSITORY https://github.com/01org/tbb.git
            GIT_TAG        2019_U1
        )
    # else()
    #     micro_download_project(tbb
    #         GIT_REPOSITORY https://github.com/wjakob/tbb.git
    #         GIT_TAG        b066defc0229a1e92d7a200eb3fe0f7e35945d95
    #     )
    # endif()
endfunction()

## CGAL
function(micro_download_cgal)
    micro_download_project(cgal
        URL     https://github.com/CGAL/cgal/releases/download/releases%2FCGAL-4.12/CGAL-4.12.tar.xz
        URL_MD5 b12fd24dedfa889a04abfaea565a88bd
    )
endfunction()

## Catch2
function(micro_download_catch)
    micro_download_project(Catch2
        URL     https://github.com/catchorg/Catch2/archive/v2.3.0.tar.gz
        URL_MD5 1fc90ff3b7b407b83057537f4136489e
    )
endfunction()

## CLI11
function(micro_download_cli11)
    micro_download_project(CLI11
        URL     https://github.com/CLIUtils/CLI11/archive/v1.6.0.tar.gz
        URL_MD5 c8e3dc70e3b7ebf6b01f618f7cdcc85f
    )
endfunction()

## nlopt
function(micro_download_nlopt)
    micro_download_project(nlopt
        GIT_REPOSITORY https://github.com/stevengj/nlopt.git
        GIT_TAG        37b74a8c2037eea5dc72fea7eeb9b850fa978913
    )
endfunction()

## libigl
function(micro_download_libigl)
    micro_download_project(libigl
        GIT_REPOSITORY https://github.com/libigl/libigl.git
        GIT_TAG        75d60e40a8edc6868571fbdca2e74f97d5dddab8
    )
endfunction()

## nanoflann
function(micro_download_nanoflann)
    micro_download_project(nanoflann
        GIT_REPOSITORY https://github.com/jlblancoc/nanoflann
        GIT_TAG v1.3.0
    )
endfunction()

## Sanitizers
function(micro_download_sanitizers)
    micro_download_project(sanitizers-cmake
        GIT_REPOSITORY https://github.com/arsenm/sanitizers-cmake.git
        GIT_TAG        99e159ec9bc8dd362b08d18436bd40ff0648417b
    )
endfunction()

## Cotire
function(micro_download_cotire)
    micro_download_project(cotire
        GIT_REPOSITORY https://github.com/sakra/cotire.git
        GIT_TAG        391bf6b7609e14f5976bd5247b68d63cbf8d4d12
    )
endfunction()
