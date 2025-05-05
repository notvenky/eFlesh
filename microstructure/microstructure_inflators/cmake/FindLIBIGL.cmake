# Find libigl
# -----------
#
# http://igl.ethz.ch/projects/libigl/
#
# The following variables are set
#
#   LIBIGL_FOUND
#   LIBIGL_INCLUDE_DIR
if(TARGET igl::core)
    return()
endif()

find_path(LIBIGL_INCLUDE_DIR igl/readOBJ.h
    HINTS
        # ENV LIBIGL
        # ENV LIBIGLROOT
        # ENV LIBIGL_ROOT
        # ENV LIBIGL_DIR
    PATHS
        ${MICRO_EXTERNAL}/libigl
        ${CMAKE_SOURCE_DIR}/../..
        ${CMAKE_SOURCE_DIR}/..
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/libigl
        ${CMAKE_SOURCE_DIR}/../libigl
        ${CMAKE_SOURCE_DIR}/../../libigl
        /usr
        /usr/local
        /usr/local/igl/libigl
    PATH_SUFFIXES include
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LIBIGL
    "libigl not found; you can download it using:\n\tgit clone --recursive https://github.com/libigl/libigl.git ${CMAKE_SOURCE_DIR}/../libigl"
    LIBIGL_INCLUDE_DIR)
mark_as_advanced(LIBIGL_INCLUDE_DIR)

if(LIBIGL_FOUND)
   list(APPEND CMAKE_MODULE_PATH "${LIBIGL_INCLUDE_DIR}/../cmake")
   include(libigl)
endif()
