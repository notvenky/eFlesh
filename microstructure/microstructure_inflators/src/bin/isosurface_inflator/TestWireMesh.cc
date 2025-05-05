#include <isosurface_inflator/WireMesh.hh>
#include <iostream>

using namespace std;

////////////////////////////////////////////////////////////////////////////////
/*! Program entry point
//  @param[in]  argc    Number of arguments
//  @param[in]  argv    Argument strings
//  @return     status  (0 on success)
*///////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
    if (argc != 2) {
        std::cerr << "usage: TestWireMesh topology.wire" << std::endl;
        exit(-1);
    }

    WireMesh<Symmetry::Cubic<>> wmesh(argv[1]);
    cout << wmesh.numVertices() << endl;
    cout << wmesh.numEdges() << endl;
    cout << wmesh.numBaseVertices() << endl;
    cout << wmesh.numBaseEdges() << endl;

    wmesh.saveBaseUnit("unit.msh");
    wmesh.save("scaled.msh");
    wmesh.saveReplicatedBaseUnit("replicated.msh");
    wmesh.saveInflationGraph("igraph.msh");
    wmesh.savePeriodCellGraph("pcell.msh");
    return 0;
}
