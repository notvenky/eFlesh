add_executable(matopt_cli
        matopt_cli.cpp
)

target_link_libraries(matopt_cli
	PRIVATE
		warnings::all
		ceres::ceres
	PUBLIC
		Eigen3::Eigen
		MeshFEM
		json::json
		matopt_lib
		CLI11::CLI11
)

add_executable(stitch_and_inflate_cli
		stitch_and_inflate_cli.cpp
)

target_link_libraries(stitch_and_inflate_cli
	PRIVATE
		warnings::all
	PUBLIC
		Eigen3::Eigen
		MeshFEM
		micro::isosurface_inflator
		CLI11::CLI11
)

add_executable(merge_meshes_cli
		merge_meshes_cli.cpp
)

target_link_libraries(merge_meshes_cli
	PRIVATE
		warnings::all
	PUBLIC
		Eigen3::Eigen
		MeshFEM
		matopt_lib
		CLI11::CLI11
		matopt::clipper
)

add_executable(test_lbfgspp
		test_lbfgspp.cpp
)

target_link_libraries(test_lbfgspp
	PRIVATE
		warnings::all
	PUBLIC
		Eigen3::Eigen
		matopt_lbfgspp
)

add_executable(check_star_polygon
		check_star_polygon.cpp
)

target_link_libraries(check_star_polygon
	PRIVATE
		warnings::all
	PUBLIC
		Eigen3::Eigen
		MeshFEM
		matopt_lib
		CLI11::CLI11
		json::json
)

add_executable(convex_hull_indices_2
		convex_hull_indices_2.cpp
)

target_link_libraries(convex_hull_indices_2
	PRIVATE
		warnings::all
	PUBLIC
		Eigen3::Eigen
		MeshFEM
		matopt_lib
		CLI11::CLI11
		json::json
		matopt::libigl
)

add_executable(box_to_path_bcs
		box_to_path_bcs.cpp
)

target_link_libraries(box_to_path_bcs
	PRIVATE
		warnings::all
	PUBLIC
		Eigen3::Eigen
		MeshFEM
		matopt_lib
		CLI11::CLI11
		json::json
		matopt::libigl
)

add_executable(create_band_cli
		create_band_cli.cpp
)

target_link_libraries(create_band_cli
	PRIVATE
		warnings::all
	PUBLIC
		Eigen3::Eigen
		MeshFEM
		matopt_lib
		CLI11::CLI11
		matopt::clipper
)

add_executable(create_offset_polygons
		create_offset_polygons.cpp
)

target_link_libraries(create_offset_polygons
	PRIVATE
		warnings::all
	PUBLIC
		Eigen3::Eigen
		MeshFEM
		matopt_lib
		CLI11::CLI11
		matopt::clipper
)

add_executable(create_boundary_holes
		create_boundary_holes.cpp
)

target_link_libraries(create_boundary_holes
	PRIVATE
		warnings::all
	PUBLIC
		Eigen3::Eigen
		MeshFEM
		matopt_lib
		CLI11::CLI11
		json::json
)