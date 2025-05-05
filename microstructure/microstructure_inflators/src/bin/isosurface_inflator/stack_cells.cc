////////////////////////////////////////////////////////////////////////////////
// Example run:
// ./isosurface_inflator/stitch_cells_cli $MICRO_DIR/isosurface_inflator/tests/patch.json -o out.obj
////////////////////////////////////////////////////////////////////////////////
#include <isosurface_inflator/StitchedWireMesh.hh>
#include <isosurface_inflator/PatternSignedDistance.hh>
#include <isosurface_inflator/IGLSurfaceMesherMC.hh>
#include <CLI/CLI.hpp>
#include <json.hpp>

#include <openvdb/tools/SignedFloodFill.h>
#include <openvdb/tools/LevelSetFilter.h>
#include <openvdb/tools/LevelSetPlatonic.h>
#include <openvdb/tools/MeshToVolume.h>
#include <openvdb/tools/VolumeToMesh.h>
#include <openvdb/tools/Composite.h>
#include <openvdb/tools/LevelSetMeasure.h>

#include <isosurface_inflator/VDBTools.hh>

////////////////////////////////////////////////////////////////////////////////

using json = nlohmann::json;
using WireMeshBasePtr = std::shared_ptr<WireMeshBase>;

////////////////////////////////////////////////////////////////////////////////

std::string lowercase(std::string data) {
    std::transform(data.begin(), data.end(), data.begin(), ::tolower);
    return data;
}

#define TRY_SYMMETRY(s, x, p)                                  \
    if (lowercase(x) == lowercase(#s))                         \
    {                                                          \
        return std::make_shared<WireMesh<Symmetry::s<>>>((p)); \
    }

#define TRY_KEY_VAL(s, a, x, p)                                \
    if (lowercase(x) == lowercase(#a))                         \
    {                                                          \
        return std::make_shared<WireMesh<Symmetry::s<>>>((p)); \
    }

WireMeshBasePtr load_wire_mesh(const std::string &sym, const std::string &path) {
    TRY_SYMMETRY(Square, sym, path);
    TRY_SYMMETRY(Cubic, sym, path);
    TRY_SYMMETRY(Orthotropic, sym, path);
    TRY_SYMMETRY(Diagonal, sym, path);
    TRY_KEY_VAL(DoublyPeriodic, Doubly_Periodic, sym, path);
    TRY_KEY_VAL(TriplyPeriodic, Triply_Periodic, sym, path);
    return nullptr;
}

////////////////////////////////////////////////////////////////////////////////

/*
patch json format
[
    {
        "params": [
            0.5,
            0.333333,
            0.666667,
            0.333333,
            ...
        ],
        "symmetry": "Orthotropic",
        "pattern": "./data/patterns/3D/reference_wires/pattern0646.wire",
        "index": [2,2,3]
    },
    ...
]
*/

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char * argv[]) {
    // Default arguments
    struct {
        std::string patch_config;
        std::string output = "out.obj";
        double gridSize = 0.1;
        int resolution = 50;
        double final_adaptivity = 0;
        // int xrange = 1;
        // int yrange = 1;
        // int zrange = 1;
    } args;

    // Parse arguments
    CLI::App app{"stack_cells"};
    app.add_option("patch,-p,--patch", args.patch_config, "Patch description (json file).")->required();
    app.add_option("--gridSize", args.gridSize, "Grid size.")->required();
    // app.add_option("-x,--xrange", args.xrange, "Grid span in X direction.")->required();
    // app.add_option("-y,--yrange", args.yrange, "Grid span in Y direction.")->required();
    // app.add_option("-z,--zrange", args.zrange, "Grid span in Z direction.")->required();
    app.add_option("-o,--output", args.output, "Output triangle mesh.");
    app.add_option("-r,--resolution", args.resolution, "Density field resolution.");
    app.add_option("--final_adaptivity", args.final_adaptivity, "adaptivity of final mesh.");

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError &e) {
        return app.exit(e);
    }

    // execute<3>(args.patch_config, args.output, args.resolution);
    const int resolution = args.resolution;

    // Load patch config
    json patch;
    std::ifstream patchFile(args.patch_config);
    try {
        patchFile >> patch;
    } catch (...) {
        std::cerr << "Error parsing the json file" << std::endl;
        return 0;
    }

    openvdb::initialize();


    std::map<Eigen::Vector3i, json, myless> material_patterns;
    for (auto entry : patch)
    {
        Eigen::Vector3i x;// = entry["index"];
        std::copy_n(entry["index"].begin(), 3, x.data());
        material_patterns[x] = entry;
    }

    /* create sdf with internal microstructure cells empty */

    const double bg_val = 3.0 / resolution;

    FloatGrid::Ptr surf_grid = openvdb::FloatGrid::create(bg_val);
    // {
    //     // math::Transform::Ptr xform = math::Transform::createLinearTransform(args.gridSize / (resolution - 1));
    //     // surf_grid = tools::meshToLevelSet<FloatGrid>(*xform, SV, SF, 3);
    //     surf_grid = mesh2sdf(args.object_surface, args.gridSize / (resolution - 1));

    //     for (auto const& it : material_patterns)
    //     {
    //         Eigen::Vector3i id = it.first;
    //         Vec3f center(id(0) + 0.5,id(1) + 0.5,id(2) + 0.5);
    //         auto tmp_grid = openvdb::tools::createLevelSetCube<FloatGrid>((resolution - 1), center * (resolution - 1), 1);
    //         openvdb::tools::csgDifference(*surf_grid, *tmp_grid);
    //     }
    // }

    /* create sdf for internal microstructure cells */

    openvdb::FloatGrid::Ptr grid = openvdb::FloatGrid::create(bg_val);
    grid->setTransform(math::Transform::createLinearTransform(args.gridSize / (resolution - 1)));
    openvdb::FloatGrid::Accessor accessor = grid->getAccessor();

    int cur = 0;
    for (auto const& it : material_patterns)
    {
        // Assign topologies and parameters to each cell
        NDCubeArray<WireMeshBasePtr, 3, 3> topologyGrid;
        NDCubeArray<std::vector<double>, 3, 3> parameterGrid;

        for (int i = -1; i <= 1; i+=1)
        for (int d = 0; d < 3; d++)
        {
            if (d != 0 && i == 0)
                continue;
            Eigen::Vector3i id = it.first;
            id[d] += i;
            json entry;
            if (auto search = material_patterns.find(id); search != material_patterns.end())
                entry = search->second;
            else
                entry = it.second;
            
            NDArrayIndex<3> index;
            Eigen::Vector3i local_id = (id - it.first).array() + 1;
            std::copy_n(local_id.data(), 3, index.idxs.begin());

            topologyGrid(index) = load_wire_mesh(entry["symmetry"], entry["pattern"]);
            parameterGrid(index) = entry["params"].get<std::vector<double>>();
        }

        auto swm = make_stitched_wire_mesh<3, true>(topologyGrid);

        auto params = swm.paramsFromParamGrid(parameterGrid);

        PatternSignedDistance<double, StitchedWireMesh<3, true>> sdf(swm);
        sdf.setUseAabbTree(true);
        std::unique_ptr<MesherBase> mesher = std::make_unique<IGLSurfaceMesherMC>();
        sdf.setParameters(params, mesher->meshingOptions.jacobian, mesher->meshingOptions.jointBlendingMode);
        
        const auto &bbox = sdf.boundingBox();
        Point3d minCorner = bbox.minCorner;
        Point3d maxCorner = bbox.maxCorner;

        const size_t nsamples = resolution * resolution * resolution;

        // Evaluation point locations;
        // flattened to be accessed as:
        // xi + resolution * (yi + resolution * zi)
        Eigen::MatrixXd sampleLocations(nsamples, 3);
        Eigen::MatrixXd ijks(nsamples, 3);
        {
            size_t i = 0;
            for (size_t zi = 0; zi < resolution; ++zi) {
                for (size_t yi = 0; yi < resolution; ++yi) {
                    for (size_t xi = 0; xi < resolution; ++xi) {
                        sampleLocations.row(i) = bbox.interpolatePoint(
                                Point3D(xi / Real(resolution - 1.0),
                                        yi / Real(resolution - 1.0),
                                        zi / Real(resolution - 1.0)));
                        ijks.row(i) << xi, yi, zi;
                        ++i;
                    }
                }
            }
        }

        // Evaluate signed distances at each grid point
        Eigen::VectorXd signedDistances(nsamples);
    #if MICRO_WITH_TBB
        tbb::parallel_for(tbb::blocked_range<size_t>(0, nsamples),
                [&](const tbb::blocked_range<size_t> &r) {
                    for (size_t i = r.begin(); i < r.end(); ++i)
                        signedDistances(i) = sdf.signedDistance(sampleLocations.row(i));
                }
            );
    #else
        for (size_t i = 0; i < nsamples; ++i)
            signedDistances(i) = sdf.signedDistance(sampleLocations.row(i));
    #endif

        int compressed = 0;
        Eigen::Vector3i offset = it.first * (resolution - 1);
        for (int i = 0; i < nsamples; i++)
        {
            if (signedDistances(i) < bg_val)
                accessor.setValue(openvdb::Coord(ijks(i, 0) + offset(0), ijks(i, 1) + offset(1), ijks(i, 2) + offset(2)), signedDistances(i));
            else
                compressed++;
        }

        std::cout << "Saved space " << compressed / (double) nsamples * 100 << " %\n";
        std::cout << "Completed " << ((++cur) * 100.0) / material_patterns.size() << " %\n";
    }

    openvdb::tools::csgUnion(*grid, *surf_grid);

    /* sdf to mesh */

    // openvdb::tools::signedFloodFill(grid->tree());
    grid->setName("density");
    grid->setGridClass(openvdb::GRID_LEVEL_SET);

    std::vector<Vec3s> Ve;
    std::vector<Vec3I> Tri;
    std::vector<Vec4I> Quad;
    tools::volumeToMesh(*grid, Ve, Tri, Quad, 0, args.final_adaptivity, true);
    clean_quads(Tri, Quad);
    write_mesh(args.output, Ve, Tri);

    return 0;
}
