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
        std::string object_surface = "";
        std::string output = "out.obj";
        double gridSize = 0.1;
        int resolution = 50;
        double final_adaptivity = 0;
    } args;

    // Parse arguments
    CLI::App app{"stitch_cells_cli"};
    app.add_option("--gridSize", args.gridSize, "Grid size.")->required();
    app.add_option("--surface", args.object_surface, "Object surface.");
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

    openvdb::initialize();

    /* create sdf */

    FloatGrid::Ptr grid = mesh2sdf(args.object_surface, args.gridSize / (resolution - 1));

    /* remove top */
    {
        Vec3f corner1(16,-5,-5);
        Vec3f corner2(20,5,5);
        openvdb::math::BBox<Vec3f> bbox(corner1 * (resolution - 1), corner2 * (resolution - 1));
        math::Transform::Ptr xform = math::Transform::createLinearTransform(1);
        auto tmp_grid = openvdb::tools::createLevelSetBox<FloatGrid>(bbox, *xform);
        openvdb::tools::csgDifference(*grid, *tmp_grid);
    }
    {
        Vec3f corner1(-2,-5,-5);
        Vec3f corner2(0,5,5);
        openvdb::math::BBox<Vec3f> bbox(corner1 * (resolution - 1), corner2 * (resolution - 1));
        math::Transform::Ptr xform = math::Transform::createLinearTransform(1);
        auto tmp_grid = openvdb::tools::createLevelSetBox<FloatGrid>(bbox, *xform);
        openvdb::tools::csgDifference(*grid, *tmp_grid);
    }

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
