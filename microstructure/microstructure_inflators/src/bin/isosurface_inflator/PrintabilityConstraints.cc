////////////////////////////////////////////////////////////////////////////////
// PrintabilityConstraints.cc
////////////////////////////////////////////////////////////////////////////////
/*! @file
//      Determines the self-supporting printability constraints automatically
//      from the graph structure
//
//      (eventually also taking radius/offset bounds into consideration?)
//      Now that optimizer is robust, we shouldn't need the offset bounds (I
//      think...). Those were just used to prevent topology changes, which
//      shouldn't be the end of the world now that we have smooth joining.
*/
//  Author:  Julian Panetta (jpanetta), julian.panetta@gmail.com
//  Company:  New York University
//  Created:  12/20/2016 19:31:21
////////////////////////////////////////////////////////////////////////////////

#include <isosurface_inflator/WireMesh.hh>
#include <boost/algorithm/string.hpp>

int main(int argc, const char *argv[]) {
    if ((argc != 2) && (argc != 3)) {
        std::cerr << "Usage: ./PrintabilityConstraints pattern.obj [\"p1 ...\"]" << std::endl;
        exit(-1);
    }
    std::string wirePath = argv[1];
    std::vector<double> params;
    if (argc == 3) {
        std::string pstring = argv[2];
        boost::trim(pstring);
        std::vector<std::string> tokens;
        boost::split(tokens, pstring, boost::is_any_of("\t "),
                     boost::token_compress_on);
        for (const std::string &s : tokens) params.push_back(std::stod(s));
    };


    using PSym = Symmetry::Orthotropic<>;
    using WMesh = WireMesh<PSym>;
    WMesh wmesh(wirePath);

    if (params.size() == 0)
        params = wmesh.defaultParameters();

    std::vector<Point3<Real>> points;
    std::vector<std::pair<size_t, size_t>> edges;
    std::vector<size_t> thicknessVars;
    std::vector<Eigen::Matrix3Xd> positionMaps;

    wmesh.printabilityGraph(params, points, edges, thicknessVars, positionMaps);

    _OutputGraph("printability.msh", points, edges);

    // Construct "homogeneous" parameter vector for applying maps
    Eigen::VectorXd paramVec(params.size() + 1);
    for (size_t i = 0; i < params.size(); ++i) paramVec[i] = params[i];
    paramVec[params.size()] = 1.0;

# if 0
    std::vector<Point3<Real>> points_mapped;
    points_mapped.reserve(positionMaps.size());

    for (const auto &pm : positionMaps) {
        points_mapped.push_back(pm * paramVec);
        std::cerr << pm << std::endl;
        std::cerr << std::endl;
    }

    _OutputGraph("printability_mapped.msh", points_mapped, edges);
#endif

    auto C = wmesh.selfSupportingConstraints(params);

    std::cout << "Self supporting constraints:" << std::endl
              << C << std::endl;

    std::cout << "Violation:" << std::endl;
    std::cout << (C * paramVec).transpose() << std::endl;

    return 0;
}
