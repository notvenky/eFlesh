#include <convert.h>
#include <mesh_utils.h>
#include <MeshFEM/MeshIO.hh>

#include <CLI/CLI.hpp>
#include <iostream>

int main(int argc, char * argv[]) {
    struct {
		std::vector<std::string> input_meshes;
		std::string output_mesh;
	} args;

	CLI::App app{"MergeMeshes"};

	app.add_option("--input", args.input_meshes, "Input triangle meshes");
	app.add_option("--output", args.output_mesh, "Output triangle mesh");

	CLI11_PARSE(app, argc, argv);

	std::vector<Eigen::MatrixXd> meshes_vertices;
	std::vector<Eigen::MatrixXi> meshes_faces;
	for (int i=0; i<args.input_meshes.size(); i++) {
	    std::vector<MeshIO::IOVertex > in_vertices;
        std::vector<MeshIO::IOElement> in_elements;
        std::string in_path = args.input_meshes[i];

        MeshIO::MeshType type;
        type = load(args.input_meshes[i], in_vertices, in_elements);

        Eigen::MatrixXd V;
        Eigen::MatrixXi F;
        from_meshfem(in_vertices, in_elements, V, F);

        meshes_vertices.push_back(V);
        meshes_faces.push_back(F);
	}

	Eigen::MatrixXd final_vertices = meshes_vertices[0];
	Eigen::MatrixXi final_faces = meshes_faces[0];;
	for (int i=1; i<meshes_vertices.size(); i++) {
	    Eigen::MatrixXd FV;
	    Eigen::MatrixXi FF;
	    mesh_intersection(final_vertices, final_faces, meshes_vertices[i], meshes_faces[i], "", FV, FF);
	    final_vertices = FV;
	    final_faces = FF;
	}

	std::vector<MeshIO::IOVertex> FMV;
	std::vector<MeshIO::IOElement> FMF;

    to_meshfem(final_vertices, final_faces, FMV, FMF);
    save(args.output_mesh, FMV, FMF);

    return 0;
}