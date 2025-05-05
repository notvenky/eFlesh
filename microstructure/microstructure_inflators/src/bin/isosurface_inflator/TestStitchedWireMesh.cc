#include <isosurface_inflator/StitchedWireMesh.hh>
#include <isosurface_inflator/PatternSignedDistance.hh>
#include <isosurface_inflator/MidplaneMesher.hh>
#include <isosurface_inflator/IGLSurfaceMesherMC.hh>

#include <memory>
#include <iostream>
#include <vector>
#include <string>

#include <cstdlib>
#include <ctime>

using namespace std;

using WMeshSPtr = std::shared_ptr<WireMeshBase>;

template<size_t N> struct DebugSymmetry    { using type = Symmetry::Square<>; };
template<>         struct DebugSymmetry<3> { using type = Symmetry::Orthotropic<>; };
template<size_t N> using DebugSymmetry_t = typename DebugSymmetry<N>::type;

template<size_t N>
void execute(const std::vector<std::string> &topologyPaths) {
    // Load all topologies passed on command line
    std::vector<WMeshSPtr> topologies;
    for (const auto &path : topologyPaths)
        topologies.emplace_back(make_shared<WireMesh<DebugSymmetry_t<N>>>(path));

    // Assign random topologies and parameters to each cell
    NDCubeArray<WMeshSPtr, N, 3> topologyGrid;
    NDCubeArray<std::vector<double>, N, 3> parameterGrid;

    using Edge = std::pair<size_t, size_t>;

    topologyGrid.visit([&](WMeshSPtr &wm, const NDArrayIndex<N> &idxs) {
        wm = topologies[rand() % topologies.size()];
        parameterGrid(idxs) = wm->defaultParameters();

        // Debug parametersForPeriodCellGraph
        std::vector<Point3<double>> points;
        std::vector<Edge> edges;
        std::vector<double> thicknesses;
        std::vector<double> blendingParams;
        std::vector<double> params, default_params = wm->defaultParameters();
        wm->savePeriodCellGraph("pcell.msh");
        wm->saveBaseUnit("base.msh");
        wm->periodCellGraph(default_params,
                points, edges, thicknesses, blendingParams);
        wm->parametersForPeriodCellGraph(points, edges, thicknesses, blendingParams, params);
        for (size_t i = 0; i < params.size(); ++i)
            assert(std::abs(params[i] - default_params[i]) < 1e-10);
    });

    auto swm = make_stitched_wire_mesh(topologyGrid);

    auto params = swm.paramsFromParamGrid(parameterGrid);

    PatternSignedDistance<double, StitchedWireMesh<N>> sdf(swm);

    // Note: JointBlendMode could be set differently in MeshingOptions
    sdf.setParameters(params, Eigen::Matrix3d::Identity(), JointBlendMode::FULL);

    std::vector<MeshIO::IOVertex > vertices;
    std::vector<MeshIO::IOElement> elements;

    if (N == 2) MidplaneMesher().mesh(sdf, vertices, elements);
    if (N == 3) IGLSurfaceMesherMC().mesh(sdf, vertices, elements);
    MeshIO::save("meshed_cell.msh", vertices, elements);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "usage: TestStitchedWireMesh topology1.wire [topology2.wire...]" << std::endl;
        exit(-1);
    }

    srand(time(NULL));

    std::vector<std::string> topologyPaths;
    for (int i = 1; i < argc; ++i)
        topologyPaths.emplace_back(argv[i]);

    // Deduce dimension from the first topology
    std::vector<MeshIO::IOVertex > vertices;
    std::vector<MeshIO::IOElement> elements;
    MeshIO::load(topologyPaths[0], vertices, elements);
    BBox<Point3D>bb(vertices);

    if (bb.dimensions()[2] < 1e-5) execute<2>(topologyPaths);
    else                           execute<3>(topologyPaths);

    return 0;
}
