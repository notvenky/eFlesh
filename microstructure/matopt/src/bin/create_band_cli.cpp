#include <convert.h>
#include <mesh_utils.h>
#include <MeshFEM/MeshIO.hh>

#include <CLI/CLI.hpp>
#include <iostream>

int main(int argc, char * argv[]) {
    struct {
		std::string input_mesh;
		std::string output_mesh;
		double offset = 0.0;
	} args;

	CLI::App app{"CreateBand"};

	app.add_option("input_mesh", args.input_mesh, "Input triangle mesh");
	app.add_option("offset", args.offset, "Offset");
	app.add_option("output_mesh", args.output_mesh, "Output triangle mesh");

	CLI11_PARSE(app, argc, argv);

    std::vector<MeshIO::IOVertex > in_vertices;
    std::vector<MeshIO::IOElement> in_elements;
    std::string in_path = args.input_mesh;

    MeshIO::MeshType type;
    type = load(in_path, in_vertices, in_elements);

    Eigen::MatrixXd V;
    Eigen::MatrixXi F;
    from_meshfem(in_vertices, in_elements, V, F);

    // offset
    Eigen::MatrixXd OV;
    Eigen::MatrixXi OF;
    std::cout << "offset: " << args.offset << std::endl;
    offset(V, F, OV, OF, args.offset);

    // difference
    Eigen::MatrixXd DV;
    Eigen::MatrixXi DF;
    mesh_difference(V, F, OV, OF, "", DV, DF);

	std::vector<MeshIO::IOVertex> final_vertices;
	std::vector<MeshIO::IOElement> final_faces;
    to_meshfem(DV, DF, final_vertices, final_faces);
    save(args.output_mesh, final_vertices, final_faces);

    return 0;
}