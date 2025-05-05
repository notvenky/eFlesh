################################################################################
# Find VCGlib
# The following are set:
#
# VCGLIB_FOUND - Whether the VCGLIB library was found
# VCGlib::core - Imported target for VCGlib
#
# It searches the environment variable $VCGLIB_INC
################################################################################

find_path(VCGLIB_INCLUDE
		"vcg/complex/complex.h"
		PATHS
			ENV VCGLIB_INC
			${MICRO_EXTERNAL}/VCGLIB
			"C:/Program Files/VCGlib/"
			"$ENV{HOME}/external/git/vcglib"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(VCGLIB DEFAULT_MSG VCGLIB_INCLUDE)

if(VCGLIB_FOUND AND NOT TARGET VCGlib::core)
	# Imported interface target for VCG
	add_library(VCGlib::core INTERFACE IMPORTED)
	set_target_properties(VCGlib::core PROPERTIES
		INTERFACE_INCLUDE_DIRECTORIES ${VCGLIB_INCLUDE})
endif()
