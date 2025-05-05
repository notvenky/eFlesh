#include "IsosurfaceInflator.hh"
#include <isosurface_inflator/IsosurfaceInflatorImpl.hh>
#include "MeshingOptions.hh"
#include "IsosurfaceInflatorConfig.hh"
#include <MeshFEM/GlobalBenchmark.hh>
#include <MeshFEM/Parallelism.hh>
#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <iostream>

namespace po = boost::program_options;
using namespace std;

using namespace std;

void usage(int status, const po::options_description &visible_opts) {
    cerr << "Usage: ./isosurface_cli mesher_name pattern [out.msh options]" << endl;
    cerr << "eg: ./isosurface_cli cubic pattern0746.wire \"0.25 0.5 0.25 0.25 0.5 0.25 0.25 0.25 0.25\" out.msh" << endl;
    cout << visible_opts << endl;
    exit(status);
}

po::variables_map parseCmdLine(int argc, char *argv[]) {
    po::options_description hidden_opts("Hidden Arguments");
    hidden_opts.add_options()
        ("mesher", po::value<string>(), "name of mesher to use")
        ("wire", po::value<string>(), "input wire file")
        ("outMSH", po::value<string>(), "output msh file")
        ;
    po::positional_options_description p;
    p.add("mesher", 1);
    p.add("wire", 1);
    p.add("outMSH", 1);

    // Options visible in the help message.
    po::options_description visible_opts;
    visible_opts.add_options()("help,h", "Produce this help message")
                              ("disablePostprocessing,d",                    "Disable post-processing of mesher output")
                              ("cheapPostprocessing",                        "Cheap post-processing of mesher output")
                              ("dumpBaseUnitGraph,B", po::value<string>(),   "Output the base unit inflation graph to the specified path")
                              ("dumpInflationGraph,D", po::value<string>(),  "Output the inflation graph to the specified path")
                              ("dumpReplicatedGraph,R", po::value<string>(), "Output the replicated pattern graph to the specified path")
                              ("mopts,m", po::value<string>(),               "Meshing options file")
                              ("params,p", po::value<string>(),              "Pattern parameters")
                              ("paramsFile", po::value<string>(),            "Pattern parameters file")
                              ("nonReflectiveInflator",                      "use non-reflective inflator (reflective by default)")
                              ("ortho_cell,O",                               "Generate the ortho cell only (for ortho-cell meshers)")
                              ("inflation_graph_radius", po::value<size_t>()->default_value(2),   "Number of edges to traverse outward from the symmetry cell when building the inflation graph")
                              ("dumpShapeVelocities,S", po::value<string>(), "Dump the shape velocities for debugging")
                              ("loadMesh,M",            po::value<string>(), "Skip meshing process, loading existing mesh instead (for debugging)")
                              ("assertPlanarNormals",                        "Verify that normals have a zero z component (relevant in 2D)")
                              ("rasterize,r",           po::value<string>(), "Rasterize the pattern to an indicator field on a regular grid")
                              ("rasterResolution",      po::value<string>()->default_value("20x20x20"),
                                                                             "Size of the rasterization grid (2D or 3D)")
#if MICRO_WITH_TBB
                              ("numProcs",              po::value<size_t>(), "Number of threads to use for TBB parallelism (CGAL mesher, etc.)")
#endif
                              ("blendingPolySize",      po::value<size_t>()->default_value(0), "Number of coefficients in the polynomial used in KS (smooth min)")
                              ("polyBasedBlending",                          "Blending based on polynomial function")
                              ("nonconvexBasedBlending",                     "Blending based on polynomial nonconvex function")
                              ("piecewiseBasedBlending",                     "Blending done splitting the blending region into multiple pieces")
                              ("defaultThickness", po::value<double>()->default_value(0.07), "Set the default value for thickness parameters to be used in inflation")
                              ;

    po::options_description cli_opts;
    cli_opts.add(visible_opts).add(hidden_opts);

    po::variables_map vm;
    try {
        po::store(po::command_line_parser(argc, argv).
                  options(cli_opts).positional(p).run(), vm);
        po::notify(vm);
    }
    catch (std::exception &e) {
        cout << "Error: " << e.what() << endl << endl;
        usage(1, visible_opts);
    }

    bool fail = false;
    if (vm.count("wire") == 0) {
        cout << "Error: must specify mesher and pattern" << endl;
        fail = true;
    }

    if (fail)
        usage(1, visible_opts);

    return vm;
}

int main(int argc, char *argv[])
{
    auto args = parseCmdLine(argc, argv);

#if MICRO_WITH_TBB
    size_t np = tbb::task_scheduler_init::default_num_threads();
    if (args.count("numProcs")) {
        size_t manualNP = args["numProcs"].as<size_t>();
        if (manualNP > np)
            std::cerr << "WARNING: specifying more than the default number of TBB threads." << std::endl;
        np = manualNP;
    }
    tbb::task_scheduler_init init(np);
#endif

    IsosurfaceInflator inflator(args["mesher"].as<string>(), true, args["wire"].as<string>(),
            args["inflation_graph_radius"].as<size_t>(), args["blendingPolySize"].as<size_t>());

    if (args.count("polyBasedBlending")) {
        inflator.meshingOptions().jointBlendingFunction = JointBlendFunction::POLY_SYMMETRIC;
    }

    if (args.count("nonconvexBasedBlending")) {
        inflator.meshingOptions().jointBlendingFunction = JointBlendFunction::POLY_NONCONVEX;
    }

    if (args.count("piecewiseBasedBlending")) {
        inflator.meshingOptions().jointBlendingFunction = JointBlendFunction::POLY_PIECEWISE;
    }

    double defaultThickness = args["defaultThickness"].as<double>();

    vector<Real> params(inflator.defaultParameters(defaultThickness));
    if (args.count("params")) {
        // Split up params.
        string paramString(args["params"].as<string>());
        boost::trim(paramString);
        std::vector<string> pStrings;
        boost::split(pStrings, paramString, boost::is_any_of("\t "), boost::token_compress_on);
        params.clear();
        for (const auto &p : pStrings)
            params.push_back(std::stod(p));
    }
    else if (args.count("paramsFile")) {
        string line;
        std::ifstream paramsFile (args["paramsFile"].as<string>(), std::ifstream::in);
        if (paramsFile.is_open()) {
            while (getline(paramsFile, line)) {
                istringstream stringStream(line);
                vector<string> tokens;
                copy(istream_iterator<string>(stringStream), istream_iterator<string>(), back_inserter(tokens));

                params.clear();
                for (const auto &p : tokens)
                    params.push_back(std::stod(p));
            }
            paramsFile.close();
        }
    }
    else {
        cout << "Inflating default parameters: " << endl;
        for (Real p : params) cout << p << "\t";
        cout << endl;

        int thickness = 0, position = 0, blending = 0;
        for (size_t i = 0; i < params.size(); i++)
        {
            thickness += inflator.m_imp->isThicknessParam(i);
            position += inflator.m_imp->isPositionParam(i);
            blending += inflator.m_imp->isBlendingParam(i);
        }
        cout << position << "\t" << thickness << "\t" << blending << "\t";
        cout << endl;
    }

    auto &config = IsosurfaceInflatorConfig::get();
    if (args.count("dumpInflationGraph")) // dump inflation graph directly without running inflation
        inflator.dumpInflationGraph(args["dumpInflationGraph"].as<string>(), params);
    if (args.count("dumpReplicatedGraph"))
        config.replicatedGraphPath = args["dumpReplicatedGraph"].as<string>();
    if (args.count("dumpBaseUnitGraph"))
        config.baseUnitGraphPath = args["dumpBaseUnitGraph"].as<string>();

    if (args.count("mopts")) inflator.meshingOptions().load(args["mopts"].as<string>());
    if (args.count("disablePostprocessing")) inflator.disablePostprocess();
    if (args.count("cheapPostprocessing")) inflator.enableCheapPostprocess();
    inflator.setReflectiveInflator(args.count("nonReflectiveInflator") == 0);

    if (args.count("dumpShapeVelocities")) inflator.meshingOptions().debugSVelPath = args["dumpShapeVelocities"].as<string>();
    if (args.count("loadMesh")) inflator.meshingOptions().debugLoadMeshPath = args["loadMesh"].as<string>();

    if (args.count("outMSH")) {
        inflator.setGenerateFullPeriodCell(args.count("ortho_cell") == 0);
        inflator.inflate(params);

        MeshIO::save(args["outMSH"].as<string>(), inflator.vertices(), inflator.elements());

        if (args.count("assertPlanarNormals")) {
            const auto &n = inflator.vertexNormals();
            double maxZMag = 0;
            size_t maxZMagVtx = 0;
            for (size_t vi = 0; vi < n.size(); ++vi) {
                Real zmag = std::abs(n[vi][2]);
                if (zmag > maxZMag) {
                    maxZMag = zmag;
                    maxZMagVtx = vi;
                }
            }

            if (maxZMag > 0.1) {
                std::cerr << "Large normal z component: " << maxZMag << std::endl;
                auto n = inflator.trackSignedDistanceGradient(inflator.vertices().at(maxZMagVtx).point);
                std::cout << "normal: " << n.transpose() << std::endl;
            }
        }
    }

    if (args.count("rasterize")) {
        inflator.rasterize(params, args["rasterResolution"].as<string>(),
                                   args["rasterize"].as<string>());
    }

    BENCHMARK_REPORT();
}
