# Prepare dependencies
#
# For each third-party library, if the appropriate target doesn't exist yet,
# download it via external project, and add_subdirectory to build it alongside
# this project.

### Configuration
set(MATOPT_ROOT     "${CMAKE_CURRENT_LIST_DIR}/..")
set(MATOPT_EXTERNAL "${MATOPT_ROOT}/3rdparty")

# Download and update 3rdparty libraries
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})
list(REMOVE_DUPLICATES CMAKE_MODULE_PATH)
include(MatoptDownloadExternal)

################################################################################
# Required libraries
################################################################################

# Geogram predicates
add_subdirectory(lib/predicates)

# Clipper
add_subdirectory(lib/clipper)

# MeshFEM library
if(NOT TARGET MeshFEM)
    matopt_download_meshfem()
    add_subdirectory(${MATOPT_EXTERNAL}/MeshFEM MeshFEM)
endif()

# CLI11 library
if(NOT TARGET CLI11::CLI11)
    matopt_download_cli11()
    add_library(CLI11 INTERFACE)
    target_include_directories(CLI11 SYSTEM INTERFACE ${MATOPT_EXTERNAL}/CLI11/include)
    add_library(CLI11::CLI11 ALIAS CLI11)
endif()

# Ipopt solver
if(NOT TARGET ipopt::ipopt)
    find_package(IPOPT REQUIRED)
endif()

# Eigen
if(NOT TARGET Eigen3::Eigen)
    add_library(matopt_eigen INTERFACE)
    matopt_download_eigen()
    target_include_directories(matopt_eigen SYSTEM INTERFACE ${MATOPT_EXTERNAL}/eigen)
    add_library(Eigen3::Eigen ALIAS matopt_eigen)
endif()

# libigl
 if(NOT TARGET igl::core)
    matopt_download_libigl()
    find_package(LIBIGL)
    target_include_directories(triangle SYSTEM INTERFACE ${MATOPT_EXTERNAL}/libigl/external/triangle)
    add_library(triangle::triangle ALIAS triangle)

    add_library(matopt_libigl INTERFACE)
    target_link_libraries(matopt_libigl INTERFACE igl::core)
    target_include_directories(matopt_libigl SYSTEM INTERFACE ${MATOPT_EXTERNAL}/libigl/)
    target_compile_definitions(matopt_libigl INTERFACE -DHAS_LIBIGL)
    add_library(matopt::libigl ALIAS matopt_libigl)
endif()

# fmt
if(NOT TARGET fmt::fmt)
	matopt_download_fmt()
	add_subdirectory(${MATOPT_EXTERNAL}/fmt)
endif()

# spdlog
if(NOT TARGET spdlog::spdlog)
	matopt_download_spdlog()
	add_library(spdlog INTERFACE)
	add_library(spdlog::spdlog ALIAS spdlog)
	target_include_directories(spdlog INTERFACE ${MATOPT_EXTERNAL}/spdlog/include)
	target_compile_definitions(spdlog INTERFACE -DSPDLOG_FMT_EXTERNAL)
	target_link_libraries(spdlog INTERFACE fmt::fmt)
endif()


# Microstructure repository
if(NOT TARGET micro::isosurface_inflator)
	matopt_download_microstructures()
  option(MESHFEM_BUILD_BINARIES "" OFF)
	option(MICRO_COPY_HEADERS "" ON)
	option(MICRO_BUILD_BINARIES "" ON)
  #set(MESHFEM_DISABLE_CXX11_ABI_GCC ${QUADFOAM_NO_CXX11_ABI} CACHE BOOL "" FORCE)
  #set(MICRO_WITH_SANITIZERS ${QUADFOAM_WITH_SANITIZERS} CACHE BOOL "" FORCE)
  add_subdirectory(${MATOPT_EXTERNAL}/microstructures microstructures)
endif()


# LBFGS library
if(NOT TARGET lbfgspp::lbfgspp)
	#matopt_download_lbfgspp()
    #add_subdirectory(${MATOPT_EXTERNAL}/LBFGSpp lbfgspp)
    add_library(matopt_lbfgspp INTERFACE)
    target_include_directories(matopt_lbfgspp SYSTEM INTERFACE ${MATOPT_EXTERNAL}/LBFGSpp/include)
endif()