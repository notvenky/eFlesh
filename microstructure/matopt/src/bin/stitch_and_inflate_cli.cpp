#include <MeshFEM/MeshIO.hh>
#include <isosurface_inflator/PatternSignedDistance.hh>
#include <isosurface_inflator/MidplaneMesher.hh>
#include <isosurface_inflator/IGLSurfaceMesherMC.hh>
#include <isosurface_inflator/WireQuadMesh.hh>

#include <CLI/CLI.hpp>
#include <iostream>

int main(int argc, char * argv[]) {
    struct {
		std::string input_mesh;
		std::string params;
		std::string meshing_opt;
		std::string output_mesh;
	} args;

	CLI::App app{"Stitch and Inflate quad mesh with information from json file"};

	app.add_option("input_mesh",   args.input_mesh,  "Input triangle mesh");
	app.add_option("input_params", args.params,      "Input microstructure params info");
	app.add_option("meshing_opt",  args.meshing_opt, "Input meshing options");
	app.add_option("output_msh",   args.output_mesh, "Output triangle mesh");

	CLI11_PARSE(app, argc, argv);

    // load json for meshing and pattern information
	std::ifstream params_file(args.params);
    nlohmann::json pattern;
    params_file >> pattern;

    std::ifstream meshing_opt_file(args.meshing_opt);
    nlohmann::json meshing;
    meshing_opt_file >> meshing;

    // Create mesher and load meshing options
    std::unique_ptr <MesherBase> mesher = std::make_unique<MidplaneMesher>();
    if (!meshing.empty()) { mesher->meshingOptions.load(meshing); }

    std::cout << meshing.dump(4) << std::endl;

    // Load input quad mesh
    std::vector <MeshIO::IOVertex> V;
    std::vector <MeshIO::IOElement> F;
    MeshIO::MeshType type;
    type = load(args.input_mesh, V, F);

    // Setup SDF function
    WireQuadMesh wm(V, F, pattern);
    wm.setActiveQuad(-1); // TODO: what should we do here?

    PatternSignedDistance<double, WireQuadMesh, WireQuadMesh::MapToBaseUnit> sdf(wm);
    sdf.setUseAabbTree(true);
    sdf.setParameters(wm.params(), mesher->meshingOptions.jacobian, mesher->meshingOptions.jointBlendingMode);
    sdf.setMapFunctor(wm.mapFunctor());
    sdf.setBoundingBox(wm.boundingBox());

    std::vector <MeshIO::IOVertex> vertices;
    std::vector <MeshIO::IOElement> elements;

    mesher->meshInterfaceConsistently = true;
    mesher->mesh(sdf, vertices, elements);

    save(args.output_mesh, vertices, elements);

    return 0;
}