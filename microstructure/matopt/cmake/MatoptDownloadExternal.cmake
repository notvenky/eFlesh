################################################################################
include(DownloadProject)

# Shortcut function
function(matopt_download_project name)
    download_project(
        PROJ         ${name}
        SOURCE_DIR   ${MATOPT_EXTERNAL}/${name}
        DOWNLOAD_DIR ${MATOPT_EXTERNAL}/.cache/${name}
        ${ARGN}
    )
endfunction()

################################################################################

## MeshFEM
function(matopt_download_meshfem)
    matopt_download_project(MeshFEM
        GIT_REPOSITORY git@github.com:geometryprocessing/MeshFEM.git
        GIT_TAG        22661071dbd589240af21440ae4ddbcd4a04303b
    )
endfunction()

## CLI11
function(matopt_download_cli11)
    matopt_download_project(CLI11
        URL     https://github.com/CLIUtils/CLI11/archive/v1.6.0.tar.gz
        URL_MD5 c8e3dc70e3b7ebf6b01f618f7cdcc85f
    )
endfunction()

## Eigen
function(quadfoam_download_eigen)
    matopt_download_project(eigen
        URL     http://bitbucket.org/eigen/eigen/get/3.3.7.tar.gz
        URL_MD5 f2a417d083fe8ca4b8ed2bc613d20f07
    )
endfunction()

## libigl
function(matopt_download_libigl)
    matopt_download_project(libigl
        GIT_REPOSITORY https://github.com/jdumas/libigl.git
        GIT_TAG        7555b6b87c23daffbeb02d522bded4f8c827fdfa
    )
endfunction()

## fmt
function(matopt_download_fmt)
    matopt_download_project(fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG        5.3.0
    )
endfunction()

## spdlog
function(matopt_download_spdlog)
    matopt_download_project(spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG        v1.3.1
    )
endfunction()

## microstructures
function(matopt_download_microstructures)
	matopt_download_project(microstructures
		GIT_REPOSITORY git@github.com:geometryprocessing/microstructures.git
		GIT_TAG        3bccbddc5286982a5dd083f356160185948a82f3
	)
endfunction()
