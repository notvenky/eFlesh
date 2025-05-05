#include <isosurface_inflator/WireMesh.hh>
#include <isosurface_inflator/PatternSignedDistance.hh>
#include "IsosurfaceInflatorConfig.hh"

#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>

#include <iostream>
#include <functional>

namespace po = boost::program_options;
using namespace std;

template<class WMesh>
void execute(const string &wirePath, string paramString, const Point3d &evalPt, const string &igraphPath) {
    WMesh wmesh(wirePath);
    PatternSignedDistance<Real, WMesh> psd(wmesh);

    vector<Real> params(wmesh.defaultParameters());
    if (paramString != "") {
        // Split up params.
        boost::trim(paramString);
        std::vector<string> pStrings;
        boost::split(pStrings, paramString, boost::is_any_of("\t "), boost::token_compress_on);
        params.clear();
        for (const auto &p : pStrings)
            params.push_back(std::stod(p));
    }
    else {
        cout << "Using default parameters: " << endl;
        for (Real p : params) cout << p << "\t";
        cout << endl;
    }

    if (igraphPath != "") wmesh.saveInflationGraph(igraphPath, params);

    psd.setParameters(params, Eigen::Matrix3d::Identity());
    std::cout << "Signed distance: " << psd.signedDistance(evalPt) << std::endl;
}

map<string, function<void(const string &, string, const Point3d &, const string &)>> execImpls =
    {{          "cubic", execute<WireMesh<Symmetry::Cubic<>>         >},
     {    "orthotropic", execute<WireMesh<Symmetry::Orthotropic<>>   >},
     {"triply_periodic", execute<WireMesh<Symmetry::TriplyPeriodic<>>>}};

void usage(int status, const po::options_description &visible_opts) {
    cerr << "Usage: ./SignedDistance_cli pattern_symmetry wire.obj evalPt -p parameters [options]" << endl;
    cerr << "eg: ./SignedDistance_cli cubic pattern0746.wire \"0.0 0.0 0.0\" -p \"0.25 0.5 0.25 0.25 0.5 0.25 0.25 0.25 0.25\"" << endl;
    cout << visible_opts << endl;
    exit(status);
}

po::variables_map parseCmdLine(int argc, char *argv[]) {
    po::options_description hidden_opts("Hidden Arguments");
    hidden_opts.add_options()
        ("symmetry",  po::value<string>(), "Pattern symmetry (triply_periodic,  orthotropic,  cubic)")
        ("wire"    ,  po::value<string>(), "input wire file")
        ("evalPt"  ,  po::value<string>(), "Evaluation point")
        ;
    po::positional_options_description p;
    p.add("symmetry", 1);
    p.add("wire", 1);
    p.add("evalPt", 1);

    // Options visible in the help message.
    po::options_description visible_opts;
    visible_opts.add_options()("help,h", "Produce this help message")
                              ("dumpInflationGraph,D", po::value<string>()->default_value(""),  " Output the inflation graph to the specified path")
                              ("params,p", po::value<string>(),              " Pattern parameters")
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
        cerr << "Error: " << e.what() << endl << endl;
        usage(1, visible_opts);
    }

    bool fail = false;
    if (vm.count("evalPt") == 0) {
        cerr << "Error: must specify all positional arguments" << endl;
        fail = true;
    }
    else {
        auto symmetry = vm["symmetry"].as<string>();
        if (execImpls.count(symmetry) == 0) {
            cerr << "Error: invalid symmetry option '" << symmetry << "'" << endl;
            fail = true;
        }
    }

    if (fail)
        usage(1, visible_opts);

    return vm;
}

int main(int argc, char *argv[])
{
    auto args = parseCmdLine(argc, argv);

    vector<Real> evalPt;
    if (args.count("evalPt")) {
        string ptString(args["evalPt"].as<string>());
        boost::trim(ptString);
        std::vector<string> components;
        boost::split(components, ptString, boost::is_any_of("\t "), boost::token_compress_on);
        for (const auto &p : components)
            evalPt.push_back(std::stod(p));
    }

    auto &config = IsosurfaceInflatorConfig::get();
    if (args.count("dumpInflationGraph"))
        config.inflationGraphPath = args["dumpInflationGraph"].as<string>();
    if (args.count("dumpReplicatedGraph"))
        config.replicatedGraphPath = args["dumpReplicatedGraph"].as<string>();

    if (evalPt.size() != 3) throw std::runtime_error("Expected whitespace-separated 3D point for sampling");

    auto &exec = execImpls.at(args["symmetry"].as<string>());

    string paramString;
    if (args.count("params")) paramString = args["params"].as<string>();

    exec(args["wire"].as<string>(), paramString, Point3d(evalPt[0], evalPt[1], evalPt[2]),
         args["dumpInflationGraph"].as<string>());
}
