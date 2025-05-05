////////////////////////////////////////////////////////////////////////////////
// IsosurfaceInflatorConfig.hh
////////////////////////////////////////////////////////////////////////////////
/*! @file
//      Simple configuration system hack (I don't want to pass these options
//      through all levels of the heirarchy).
//      This is a simple singleton pattern implementation that is NOT
//      threadsafe.
*/ 
//  Author:  Julian Panetta (jpanetta), julian.panetta@gmail.com
//  Company:  New York University
//  Created:  11/14/2015 13:51:14
////////////////////////////////////////////////////////////////////////////////
#ifndef ISOSURFACEINFLATORCONFIG_HH
#define ISOSURFACEINFLATORCONFIG_HH
#include <string>

class IsosurfaceInflatorConfig {
private:
    IsosurfaceInflatorConfig() { }
public:
    std::string inflationGraphPath, replicatedGraphPath, baseUnitGraphPath;

    bool dumpInflationGraph()  const { return  inflationGraphPath.length(); }
    bool dumpReplicatedGraph() const { return replicatedGraphPath.length(); }
    bool dumpBaseUnitGraph()   const { return   baseUnitGraphPath.length(); }

    static IsosurfaceInflatorConfig &get() {
        static IsosurfaceInflatorConfig configSingleton;
        return configSingleton;
    }
};

#endif /* end of include guard: ISOSURFACEINFLATORCONFIG_HH */
