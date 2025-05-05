#include <isosurface_inflator/SignedDistanceRegion.hh>
#include <isosurface_inflator/SignedDistance.hh>
#include <isosurface_inflator/MidplaneMesher.hh>
#include <isosurface_inflator/PatternSignedDistance.hh>
#include <isosurface_inflator/Joint.hh>
#include <isosurface_inflator/WireMesh.hh>
#include <stdexcept>

using Point = Point3<double>;
using Edge = std::pair<size_t, size_t>;

struct VisSDFunc : public SignedDistanceRegion<3> {
    using Sphere = SD::Primitives::Sphere<double>;
    using IEdge = SD::Primitives::InflatedEdge<double>;
    VisSDFunc(const std::string &path, std::vector<double> params)
        : m_wmesh(path, 100), m_psd(m_wmesh),
          m_bb(Point(-1, -1, -1), Point(1, 1, 1))
    {
        if (params.size() == 0) {
            params = m_wmesh.defaultParameters();
            std::cerr << "Using default params" << std::endl;
        }

        m_psd.setParameters(params, Eigen::Matrix3d::Identity(), JointBlendMode::HULL);
        std::vector<std::vector<Real>> blendingPolyParams;
        m_wmesh.inflationGraph(params, points, edges, thicknesses, blendingParams, blendingPolyParams);

        for (size_t i = 0; i < points.size(); ++i)
            m_spheres.emplace_back(points[i], thicknesses[i]);
        for (const auto & e : edges) {
            m_inflatedEdges.emplace_back(points[e.first], points[e.second],
                    thicknesses[e.first], thicknesses[e.second]);
        }

        {
            std::vector<Point>  pcell_points;
            std::vector<Edge>   pcell_edges;
            std::vector<double> pcell_thicknesses;
            std::vector<double> pcell_blendingParams;

            m_wmesh.periodCellGraph(params, pcell_points, pcell_edges, pcell_thicknesses, pcell_blendingParams);
            _OutputGraph("pcell_graph.msh", pcell_points, pcell_edges);
        }

        m_customEdges.emplace_back(Point(0, 0.6, 0.0), Point(-0.7, -0.7, 0.0), 0.3, 0.15);
        m_customEdges.emplace_back(Point(0, 0.6, 0.0), Point( 0.7, -0.7, 0.0), 0.3, 0.15);
    }

    Real signedDistance(const Point3<Real> &p) const override {
        if (renderEdge >= 0) return m_inflatedEdges.at(renderEdge).signedDistance(p);
        if (renderJoint >= 0) {
            auto edgeDistances = MeshFEM::apply(m_inflatedEdges, [&](const IEdge &e) { return e.signedDistance(p); });
            std::vector<double> jointEdgeDists;
            return m_psd.distToVtxJoint(renderJoint, p, edgeDistances, jointEdgeDists,
                m_psd.incidentEdges(), [](size_t e) { return e; }).smooth;
        }

        if (renderCustomEdge >= 0) {
            if (renderCustomEdge == 2) return std::min(m_customEdges[0].signedDistance(p),
                                                       m_customEdges[1].signedDistance(p));
            if (renderCustomEdge == 3) return SD::exp_smin_reparam_accurate(
                                                       m_customEdges[0].signedDistance(p),
                                                       m_customEdges[1].signedDistance(p), 0.05);
            return m_customEdges.at(renderCustomEdge).signedDistance(p);
        }

        auto sphereDistances = MeshFEM::apply(m_spheres, [&](const Sphere &s) { return s.signedDistance(p); });
        return *std::min_element(sphereDistances.begin(), sphereDistances.end());
    }

    virtual const BBox<Point> &boundingBox() const override {
        return m_bb;
    }

    // Inflation graph
    std::vector<Point> points;
    std::vector<Edge> edges;
    std::vector<double> thicknesses;
    std::vector<double> blendingParams;
    int renderEdge = -1;
    int renderJoint = -1;
    int renderCustomEdge = -1;
private:
    using WMesh = WireMesh<Symmetry::Square<>>;
    WMesh m_wmesh;
    PatternSignedDistance<double, WMesh> m_psd;
    std::vector<Sphere> m_spheres;
    std::vector<IEdge> m_inflatedEdges, m_customEdges;
    BBox<Point> m_bb;
};

int main(int argc, const char *argv[]) {
    std::vector<double> params;
    for (int i = 2; i < argc; ++i)
        params.push_back(std::stod(argv[i]));

    VisSDFunc sdfunc(argv[1], params);
    MidplaneMesher mesher;
    mesher.meshingOptions.marchingSquaresGridSize = 512;
    mesher.meshingOptions.forceMSGridSize = true;
    auto bb = sdfunc.boundingBox();

    std::vector<MeshIO::IOVertex > vertices;
    std::vector<MeshIO::IOElement> elements;
#if 1
    mesher.mesh(sdfunc, vertices, elements);

    MeshIO::save("spheres.msh", vertices, elements);

    for (size_t i = 0; i < sdfunc.edges.size(); ++i) {
        sdfunc.renderEdge = i;
        const auto e = sdfunc.edges[i];
        if (!(bb.containsPoint(sdfunc.points.at(e.first)) && bb.containsPoint(sdfunc.points.at(e.second))))
            continue;
        std::cout << "meshing edge " << i << std::endl;
        mesher.mesh(sdfunc, vertices, elements);
        if (elements.size() > 0) MeshIO::save("edge" + std::to_string(i) + ".msh", vertices, elements);
    }
    sdfunc.renderEdge = -1;
    for (size_t i = 0; i < sdfunc.points.size(); ++i) {
        sdfunc.renderJoint = i;
        if (!bb.containsPoint(sdfunc.points.at(i))) continue;
        mesher.mesh(sdfunc, vertices, elements);
        MeshIO::save("joint" + std::to_string(i) + ".msh", vertices, elements);
    }
    sdfunc.renderJoint = -1;
#endif

    for (size_t i = 0; i < 4; ++i) {
        sdfunc.renderCustomEdge = i;
        mesher.msPolygonPath = "custom_edge_polygon" + std::to_string(i) + ".msh";
        mesher.mesh(sdfunc, vertices, elements);
        MeshIO::save("custom_edge" + std::to_string(i) + ".msh", vertices, elements);
        mesher.dumpSDF(sdfunc, "custom_edge_sdf" + std::to_string(i) + ".msh");
    }

    return 0;
}
