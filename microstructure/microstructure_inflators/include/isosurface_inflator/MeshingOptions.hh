#ifndef MESHINGOPTIONS_HH
#define MESHINGOPTIONS_HH

#include "JointCommon.hh"
#include <Eigen/Dense>
#include <nlohmann/json.hpp>
#include <string>
#include <cmath>

struct MeshingOptions {
    MeshingOptions() { }
    MeshingOptions(const std::string &jsonPath) {
        load(jsonPath);
    }
    MeshingOptions(const nlohmann::json &config) {
        load(config);
    }

    // Jacobian
    Eigen::Matrix3d jacobian = Eigen::Matrix3d::Identity();

    // Joint blending mode and function:
    JointBlendMode jointBlendingMode = JointBlendMode::FULL;
    JointBlendFunction jointBlendingFunction = JointBlendFunction::EXPONENTIAL;

    // CGAL volume mesher options
    double domainErrorBound          = 1e-5;
    double facetAngle                = 30.0;
    double facetSize                 = 0.025;
    double facetDistance             = 2e-3;
    double cellSize                  = 0.15;
    double edgeSize                  = 0.025;
    double cellRadiusEdgeRatio       = 2.0;

    // VCG Marching cubes options
    size_t marchingCubesGridSize = 128;

    // 3D features extraction/2D mesher options
    size_t marchingSquaresGridSize = 256;

    // Coarsening levels used in adaptive marching squares:
    // Initially sample at a grid 2^marchingSquaresCoarsening times coarser,
    // and only refine where the contour is found.
    size_t marchingSquaresCoarsening = 0;

    // For MidplaneMesher, there is a single parameter: maxArea
    // The marchingSquaresGridSize and boundary max edge length are
    // automatically determined from this parameter:
    // The boundary max edge length is computed based on the edge length of an
    // equilateral triangle of area maxArea:
    //      maxLen = sqrt((4.0 / sqrt(3.0)) * maxArea)
    // Then, the marching squares grid size is chosen to ensure this maximal
    // edge length, even for diagonal edges. We aim for a square grid, so
    // we assume the diagonal is of length sqrt(2) * (side length) ==>
    //      sideLen = maxLen / sqrt(2)
    //      gridSize = ceil(domainLength / sideLen)
    double maxArea = 0.001;
    double featureAngleThreshold = M_PI / 4.0;
    // Force the marching squares grid resolution declared above, instead of the
    // one derived from the max area threshold.
    bool forceMSGridSize = false;
    // Use an adaptive edge length based on curvature (anticipating high
    // stresses in concave regions).
    bool curvatureAdaptive = false;
    // Whether an edge-collapse or smooth curve resampling is used to
    // post-process the marching squares result
    enum {COLLAPSE, RESAMPLE, NONE} curveSimplifier = RESAMPLE;
    // Derived parameters
    // Always determine max edge length from the maxArea param
    double maxEdgeLenFromMaxArea() const {
        return sqrt((4.0 / sqrt(3.0)) * maxArea);
    }
    // To avoid collapsing short edges when requested, base the collapse
    // threshold on marchingSquaresGridSize in forceMSGridSize case.
    // Domain length only actually needed in the forceMSGridSize case...
    double minEdgeLenFromMaxArea(double domainLength) const {
        if (forceMSGridSize) return (domainLength / marchingSquaresGridSize) * sqrt(2.0) / 6.0;
        else return maxEdgeLenFromMaxArea() / 4.0;
    }

    size_t msGridSizeFromMaxArea(double domainLength) const {
        if (forceMSGridSize) return marchingSquaresGridSize;
        return ceil((domainLength * sqrt(2.0)) / maxEdgeLenFromMaxArea());
    }

    // Allow user to override the maximum boundary edge length (resolution) to
    // allow even coarser interior tesselation while still resolving the
    // boundary. (Without this, the maximum boundary edge length would be tied
    // to the interior tesselation resolution).
    double maxBdryEdgeLen() const {
        if (m_forceMaxBdryEdgeLen) return m_forcedMaxBdryEdgeLen;
        else return maxEdgeLenFromMaxArea();
    }

    // Manually enforce a consistent mesh on the cell interface, ensuring that
    // the boundary mesh on the interface depends only on the interface
    // geometry and can stitched with a neighbor whenever the geometry matches.
    // This option is only supported for 2D patterns and is  NOT needed for
    // homogenization; it should only be needed, e.g., when one wishes to
    // stitch a square-symmetric cell with a 90-degree-rotated copy of itself.
    bool forceConsistentInterfaceMesh = false;

    void load(const std::string &jsonPath);
    void load(const nlohmann::json &config);

    static Eigen::Matrix3d read_jacobian(const nlohmann::json &entry);

    // Set nonempty to dump shape velocity debugging fields upon inflation.
    std::string debugSVelPath;
    // Don't inflate; instead, load pre-existing mesh.
    std::string debugLoadMeshPath;

private:
    bool    m_forceMaxBdryEdgeLen = false;
    double m_forcedMaxBdryEdgeLen = 1.0;
};

#endif /* end of include guard: MESHINGOPTIONS_HH */
