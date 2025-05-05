////////////////////////////////////////////////////////////////////////////////
// tile.cc
////////////////////////////////////////////////////////////////////////////////
/*! @file
//  Replicate the double/triply periodic meshing cell, propagating MSH fields.
//  In contrast to replicate.cc, this uses translations instead of reflections
//  to replicate geometry.
//  WARNING: for now the mesh is NOT stitched together (this is a hack to
//  finish the fast-forward video).
*/
//  Author:  Julian Panetta (jpanetta), julian.panetta@gmail.com
//  Company:  New York University
//  Created:  06/16/2017 08:12:35
////////////////////////////////////////////////////////////////////////////////

#include <MeshFEM/MSHFieldParser.hh>
#include <MeshFEM/MSHFieldWriter.hh>
#include "Geometry.hh"
#include <MeshFEM/SimplicialMesh.hh>
#include <MeshFEM/PeriodicBoundaryMatcher.hh>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <numeric>

namespace po = boost::program_options;
using namespace std;

void usage(int exitVal, const po::options_description &visible_opts) {
    cout << "Usage: tile [options] in_period_cell.msh out.msh" << endl;
    cout << visible_opts << endl;
    exit(exitVal);
}

po::variables_map parseCmdLine(int argc, const char *argv[])
{
    po::options_description hidden_opts("Hidden Arguments");
    hidden_opts.add_options()
        ("period_cell", po::value<string>(), "period cell msh")
        ("output",      po::value<string>(), "Output msh path.")
        ;
    po::positional_options_description p;
    p.add("period_cell", 1)
     .add("output", 1);

    po::options_description visible_opts;
    visible_opts.add_options()("help",        "Produce this help message")
        ("tile,t",      po::value<string>()->default_value("2x2x2"), "Tile into an 'Nx x Ny [x Nz]' repetition of the full period cell..")
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

    if (vm.count("period_cell") + vm.count("output") != 2) {
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
                   const std::vector<size_t> &origElement)
{
    const size_t nv = writer.numVertices(),
                 ne = writer.numElements();
    if ((origVertex.size() != nv) || (origElement.size() != ne)) {
        throw std::runtime_error("Invalid origVertex/origElement arrays");
    }

    F outField;
    if (dtype == DomainType::PER_NODE) {
        outField.resizeDomain(nv);
        for (size_t i = 0; i < nv; ++i)
            outField(i) = field(origVertex[i]);
    }
    else if (dtype == DomainType::PER_ELEMENT) {
        outField.resizeDomain(ne);
        for (size_t i = 0; i < ne; ++i)
            outField(i) = field(origElement[i]);
    }
    else { throw std::runtime_error("Unsupported domain dtype"); }

    writer.addField(name, outField, dtype);
}

template<size_t N>
void execute(const std::vector<std::string> &components, const std::string &in_mesh, const std::string &out_mesh) {
    std::array<size_t, N> repetitions;

    size_t nCopies = 1;
    for (size_t i = 0; i < N; ++i) {
        repetitions[i] = std::stoi(components[i]);
        nCopies *= repetitions[i];
    }

    MSHFieldParser<N> reader(in_mesh);

    std::vector<MeshIO::IOVertex > vertices = reader.vertices();
    std::vector<MeshIO::IOElement> elements = reader.elements();
    BBox<PointND<N>> bbox(vertices);
    auto dim = bbox.dimensions();

    SimplicialMesh<N> mesh(elements, vertices.size());
    std::vector<PointND<N>> bdryPoints;
    for (const auto &bv : mesh.boundaryVertices())
        bdryPoints.push_back(truncateFrom3D<PointND<N>>(vertices.at(bv.volumeVertex().index()).point));


    std::vector<PeriodicBoundaryMatcher::FaceMembership<N>> faceMembership;
    std::vector<std::vector<size_t>> nodeSets;
    std::vector<size_t>              nodeSetForNode;

    // Vertices are glued together searching only the equivalence class of
    // vertices corresponding to the same periodic degree of freedom (nodeSet).

    PeriodicBoundaryMatcher::determineCellBoundaryFaceMembership(bdryPoints,
            bbox, faceMembership);
    PeriodicBoundaryMatcher::match(bdryPoints, bbox, faceMembership, nodeSets, nodeSetForNode);

    // index of each (volume) vertex in an equivalence class
    std::vector<std::vector<size_t>> equivalenceClass(nodeSets.size());
    for (size_t i = 0; i < nodeSets.size(); ++i) {
        for (size_t bvi : nodeSets[i])
            equivalenceClass[i].push_back(mesh.boundaryVertex(bvi).volumeVertex().index());
    }

    // Track vertex and elements back to their originating entities.
    const size_t nVerts = vertices.size();
    const size_t nElems = elements.size();
    std::vector<size_t> origVertex(nVerts), origElement(nElems);
    origVertex.reserve(nVerts * nCopies);
    origElement.reserve(nElems * nCopies);

    std::iota(origVertex.begin(), origVertex.end(), 0);
    std::iota(origElement.begin(), origElement.end(), 0);

    std::vector<MeshIO::IOVertex > outVertices;
    std::vector<MeshIO::IOElement> outElements;
    outVertices.reserve(nVerts * nCopies);
    outElements.reserve(nElems * nCopies);
    outVertices = vertices;
    outElements = elements;

    // Replicate, only creating new vertices when gluing is impossible
    double epsilon = 1e-8;
    for (size_t d = 0; d < N; ++d) {
        // Loop invariant: outElements indexes into outVertices, which have already
        // been glued together into a single component.
        size_t nv = outVertices.size(),
               ne = outElements.size();
        for (size_t i = 1; i < repetitions[d]; ++i) {
            // New global index of the (glued) copies created of each of the
            // first nv vertices in outVertices.
            std::vector<size_t> gluedidxOfVtxCopy;
            gluedidxOfVtxCopy.reserve(nv);
            for (size_t vi = 0; vi < nv; ++vi) {
                PointND<3> pt = outVertices[vi].point;
                pt[d] += i * dim[d];

                size_t new_vidx = outVertices.size();
                size_t eci = equivalenceClass.size();
                auto bv = mesh.vertex(origVertex[vi]).boundaryVertex();
                // Determine if gluing is possible (only for boundary vertices)
                if (bv) {
                    eci = nodeSetForNode.at(bv.index());
                    auto &ec = equivalenceClass.at(eci);
                    // std::cout << "searching for " << pt.transpose() << " in equivalence class " << eci << std::endl;
                    for (size_t candidate_vidx : ec) {
                        // std::cout << "\tchecking " << outVertices.at(candidate_vidx).point.transpose() << std::endl;
                        if ((outVertices.at(candidate_vidx).point - pt).norm() < epsilon) {
                            // Found a vertex to glue with
                            new_vidx = candidate_vidx;
                            break;
                        }
                    }
                }
                // std::cout << "eci: " << eci << "/" << equivalenceClass.size() << std::endl;
                // std::cout << "new_vidx: " << new_vidx << "/" << outVertices.size() << std::endl;
                assert(new_vidx <= outVertices.size());
                if (new_vidx == outVertices.size()) {
                    // No gluing; we need to create a new vertex
                    outVertices.emplace_back(pt);
                    origVertex.push_back(origVertex[vi]);

                    // If the originating vertex is a boundary vertex (with a
                    // corresponding equivalence class), add the newly created
                    // vertex to its equivalence class
                    assert(eci <= equivalenceClass.size());
                    // std::cout << "eci: " << eci << "/" << equivalenceClass.size() << std::endl;
                    if (eci < equivalenceClass.size()) {
                        equivalenceClass[eci].push_back(new_vidx);
                        // std::cout << "added " << pt.transpose() << " to equivalenceClass class " << eci << std::endl;
                    }
                }

                gluedidxOfVtxCopy.push_back(new_vidx);
            }
            for (size_t ei = 0; ei < ne; ++ei) {
                outElements.emplace_back(outElements[ei]);
                for (size_t &vi : outElements.back())
                    vi = gluedidxOfVtxCopy.at(vi);
                origElement.push_back(origElement[ei]);
            }
        }
    }

    // std::cout << outVertices.size() << std::endl;
    // std::cout << outElements.size() << std::endl;

    MSHFieldWriter writer(out_mesh, outVertices, outElements);

    // Transfer fields (Note: higher degree per-element fields are not supported
    // and will be ignored).
    std::vector<string> fnames = reader.vectorFieldNames();
    DomainType type;
    for (const string &name: fnames) {
        const auto &vf = reader.vectorField(name, DomainType::ANY, type);
        transferField(vf, name, type, writer, origVertex, origElement);
    }
    fnames = reader.scalarFieldNames();
    for (const string &name: fnames) {
        const auto &sf = reader.scalarField(name, DomainType::ANY, type);
        transferField(sf, name, type, writer, origVertex, origElement);
    }
    fnames = reader.symmetricMatrixFieldNames();
    for (const string &name: fnames) {
        const auto &smf = reader.symmetricMatrixField(name, DomainType::ANY, type);
        transferField(smf, name, type, writer, origVertex, origElement);
    }
}

int main(int argc, const char *argv[]) {
    auto args = parseCmdLine(argc, argv);

    // Process "reflect" option to get number of reflections needed in each
    // dimension.
    vector<string> components;
    boost::split(components, args.at("tile").as<string>(), boost::is_any_of("x"));
    if      (components.size() == 2) execute<2>(components, args.at("period_cell").as<string>(), args.at("output").as<string>());
    else if (components.size() == 3) execute<3>(components, args.at("period_cell").as<string>(), args.at("output").as<string>());
    else throw runtime_error("Must have 2 or 3 components of the form Nx x Ny x Nz");

    return 0;
}

