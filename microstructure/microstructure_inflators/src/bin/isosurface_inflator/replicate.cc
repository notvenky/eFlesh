////////////////////////////////////////////////////////////////////////////////
// replicate.cc
////////////////////////////////////////////////////////////////////////////////
/*! @file
//      Replicate the orthotropic meshing cell, propagating MSH fields.
*/
//  Author:  Julian Panetta (jpanetta), julian.panetta@gmail.com
//  Company:  New York University
//  Created:  01/07/2017 18:46:14
////////////////////////////////////////////////////////////////////////////////

#include <MeshFEM/MSHFieldParser.hh>
#include <MeshFEM/MSHFieldWriter.hh>
#include <MeshFEM/ComponentMask.hh>
#include <MeshFEM/filters/reflect.hh>

#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

namespace po = boost::program_options;
using namespace std;

void usage(int exitVal, const po::options_description &visible_opts) {
    cout << "Usage: replicate [options] in_base_cell.msh out.msh" << endl;
    cout << visible_opts << endl;
    exit(exitVal);
}

po::variables_map parseCmdLine(int argc, const char *argv[])
{
    po::options_description hidden_opts("Hidden Arguments");
    hidden_opts.add_options()
        ("ortho_cell", po::value<string>(), "Orthotropic base cell msh")
        ("output",     po::value<string>(), "Output msh path.")
        ;
    po::positional_options_description p;
    p.add("ortho_cell", 1)
     .add("output", 1);

    po::options_description visible_opts;
    visible_opts.add_options()("help",        "Produce this help message")
        ("reflect,r",      po::value<string>()->default_value("1x1x1"),            "Reflect into an 'Nx x Ny x Nz' tiling of the full period cell..")
        ;
    bool fail = false;

    po::options_description cli_opts;
    cli_opts.add(visible_opts).add(hidden_opts);

    po::variables_map vm;
    try {
        po::store(po::command_line_parser(argc, argv).
                  options(cli_opts).positional(p).run(), vm);
        po::notify(vm);
    }
    catch (exception &e) {
        cout << "Error: " << e.what() << endl << endl;
        usage(1, visible_opts);
    }

    if (vm.count("ortho_cell") + vm.count("output") != 2) {
        std::cerr << "ERROR: Missing positional options";
        fail = true;
    }

    if (fail || vm.count("help"))
        usage(fail, visible_opts);

    return vm;
}


template<typename F>
void transferField(const F &field, const std::string &name, DomainType dtype,
                   MSHFieldWriter &writer,
                   const std::vector<size_t> &origVertex,
                   const std::vector<size_t> &origElement,
                   const std::vector<ComponentMask> &vtxRefl,
                   const std::vector<ComponentMask> &elmRefl) {
    const size_t nv = writer.numVertices(),
                 ne = writer.numElements();
    if ((origVertex.size() != nv) || (origElement.size() != ne)) {
        throw std::runtime_error("Invalid origVertex/origElement arrays");
    }

    F outField;
    if (dtype == DomainType::PER_NODE) {
        outField.resizeDomain(nv);
        for (size_t i = 0; i < nv; ++i) {
            outField(i) = field(origVertex[i]);
            if ((outField.dim() > 1) && (outField.dim() < 4)) {
                // Reflect vectors
                for (size_t d = 0; d < outField.dim(); ++d)
                    outField(i)[d] *= vtxRefl.at(i).has(d) ? -1.0 : 1.0;
            }
        }
    }
    else if (dtype == DomainType::PER_ELEMENT) {
        outField.resizeDomain(ne);
        for (size_t i = 0; i < ne; ++i) {
            outField(i) = field(origElement[i]);
            if ((outField.dim() > 1) && (outField.dim() < 4)) {
                // Reflect vectors
                for (size_t d = 0; d < outField.dim(); ++d)
                    outField(i)[d] *= elmRefl.at(i).has(d) ? -1.0 : 1.0;
            }
        }
    }
    else { throw std::runtime_error("Unsupported domain dtype"); }

    writer.addField(name, outField, dtype);
}

template<size_t N>
void execute(const std::vector<std::string> &components, const std::string &in_mesh, const std::string &out_mesh) {
    std::array<size_t, 3> nReflections;
    nReflections.fill(0);

    for (size_t i = 0; i < N; ++i) {
        size_t nTiles = std::stoi(components[i]);
        if (nTiles == 0) continue; // no reflections needed to tile 0 times.
        {
            size_t nt = nTiles;
            while (nt >>= 1) ++nReflections[i];
        }
        if ((size_t(1) << nReflections[i]) != nTiles)
            throw runtime_error("Only power of 2 tilings are supported");
        // It takes one reflection to reach 1x1x1 period cell.
        ++nReflections[i];
    }

    MSHFieldParser<N> reader(in_mesh);

    std::vector<MeshIO::IOVertex > vertices = reader.vertices();
    std::vector<MeshIO::IOElement> elements = reader.elements();

    // Track vertex and elements back to their originating entities.
    std::vector<size_t> origVertex(vertices.size()), origElement(elements.size());
    std::vector<ComponentMask> vtxRefl, elmRefl;

    for (size_t i = 0; i < vertices.size(); ++i) { origVertex[i] = i; }
    for (size_t i = 0; i < elements.size(); ++i) { origElement[i] = i; }

    while (nReflections[0] + nReflections[1] + nReflections[2]) {
        ComponentMask mask;
        for (size_t i = 0; i < N; ++i) {
            if (nReflections[i] > 0) {
                mask.set(i);
                --nReflections[i];
            }
        }

        std::vector<size_t> newOrigVertex, newOrigElement;
        reflect(N, vertices, elements, vertices, elements, mask,
                &newOrigVertex, &newOrigElement, &vtxRefl, &elmRefl);

        // Propagate originating vertex/element info
        for (size_t &ov : newOrigVertex ) ov = origVertex .at(ov);
        for (size_t &oe : newOrigElement) oe = origElement.at(oe);

        origVertex  = std::move(newOrigVertex);
        origElement = std::move(newOrigElement);

        // TODO: propagate reflection info; for now we can only handle 1x1x1
        // case.
    }

    MSHFieldWriter writer(out_mesh, vertices, elements);

    // Transfer fields (Note: higher degree per-element fields are not supported
    // and will be ignored).
    std::vector<string> fnames = reader.vectorFieldNames();
    DomainType type;
    for (const string &name: fnames) {
        const auto &vf = reader.vectorField(name, DomainType::ANY, type);
        std::cerr << "WARNING: Vector transforms only implemented for 1x1x1 tiling" << std::endl;
        transferField(vf, name, type, writer, origVertex, origElement, vtxRefl, elmRefl);
    }
    fnames = reader.scalarFieldNames();
    for (const string &name: fnames) {
        const auto &sf = reader.scalarField(name, DomainType::ANY, type);
        transferField(sf, name, type, writer, origVertex, origElement, vtxRefl, elmRefl);
    }
    fnames = reader.symmetricMatrixFieldNames();
    for (const string &name: fnames) {
        const auto &smf = reader.symmetricMatrixField(name, DomainType::ANY, type);
        std::cerr << "WARNING: Tensor transforms under reflection unimplemented for replication" << std::endl;
        transferField(smf, name, type, writer, origVertex, origElement, vtxRefl, elmRefl);
    }

    ScalarField<double> highlightOrthoBase(vertices.size());
    for (size_t i = 0; i < vertices.size(); ++i) {
        highlightOrthoBase[i] = (origVertex[i] == i) ? 1.0 : 0.0;
    }
    writer.addField("ortho_base_indicator", highlightOrthoBase);

}

int main(int argc, const char *argv[]) {
    auto args = parseCmdLine(argc, argv);

    // Process "reflect" option to get number of reflections needed in each
    // dimension.
    vector<string> components;
    boost::split(components, args.at("reflect").as<string>(), boost::is_any_of("x"));
    if      (components.size() == 2) execute<2>(components, args.at("ortho_cell").as<string>(), args.at("output").as<string>());
    else if (components.size() == 3) execute<3>(components, args.at("ortho_cell").as<string>(), args.at("output").as<string>());
    else throw runtime_error("Must have 2 or 3 components of the form Nx x Ny x Nz");

    return 0;
}
