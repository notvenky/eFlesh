////////////////////////////////////////////////////////////////////////////////
// IsosurfaceInflator.hh
////////////////////////////////////////////////////////////////////////////////
/*! @file
//      Interface to the isosurface inflator.
//      Everything is implemented in terms of opaque class
//      IsosurfaceInflatorImpl to speed up compilation of the code including us
//      (compiling anything depending on CGAL is slow!!).
//
//      Both 2D and 3D isosurface inflators are implemented (and can be selected
//      using the constructor's "type" argument), but the result is always
//      embedded in 3D.
*/
//  Author:  Julian Panetta (jpanetta), julian.panetta@gmail.com
//  Company:  New York University
//  Created:  09/29/2015 12:17:26
////////////////////////////////////////////////////////////////////////////////
#ifndef ISOSURFACEINFLATOR_HH
#define ISOSURFACEINFLATOR_HH
#include "MeshingOptions.hh"
#include "InflatorTypes.hh"
#include "MeshIO.hh"
#include "BaseCellType.hh"
#include <string>
#include <vector>

class IsosurfaceInflator {
public:
    typedef Eigen::Matrix<Real, 3, 1> Point;
    enum class ParameterType { Thickness, Position, Blending, Custom1, Custom2, Custom3, Custom4, Custom5, Custom6, Custom7, Custom8 };

    IsosurfaceInflator(const std::string &type, bool vertexThickness,
                       const std::string &wireMeshPath,
                       size_t inflationNeighborhoodEdgeDist = 2,
                       size_t blendingPolySize = 0);

    void inflate(const std::vector<Real> &params);
    std::vector<Real> defaultParameters(Real thickness = 0.07) const;

    void rasterize(const std::vector<Real> &params, const std::string &resolution, const std::string &outPath);

    // Configure whether the full period cell is generated. E.g. in the case of
    // orthotropic symmetry, controls whether the representative eighth-cell is
    // reflected into the full cell.
    void setGenerateFullPeriodCell(bool onoff);

    // When the full period cell is to be generated from a mesh with a smaller
    // symmetry base cell (generateFullPeriodCell), configure whether the
    // smaller base cell is meshed and then reflected (true) or whether the full
    // period cell is meshed.
    void setReflectiveInflator(bool onoff);

    BaseCellType baseCellType() const;

    // Access the inflation result
    // Vertex normals and shape velocity are ***per-volume-vertex***, taking value 0 on
    // non-boundary vertices.
    const std::vector<MeshIO::IOVertex>  &vertices() const;
    const std::vector<MeshIO::IOElement> &elements() const;
    void clear();
    const std::vector<std::vector<Real>> &normalShapeVelocities() const;
    const std::vector<Point>             &vertexNormals() const;
    const std::vector<Real>              &inflatedParams() const;

    // For debugging
    Point trackSignedDistanceGradient(const Point &evalPt) const;

    void dumpInflationGraph(const std::string &path, const std::vector<Real> &params) const;

    size_t numParams() const;

    bool isThicknessParam(size_t p) const;
    bool isPositionParam(size_t p) const;
    bool isBlendingParam(size_t p) const;

    // Answers which blending poly coefficient index p corresponds to. If index p does not correspond to blending poly
    // param, then returns -1.
    int  whichBlendingPolyParam(size_t p) const;

    bool hasOrthotropicSymmetry() const;

    // For debugging
    void disablePostprocess();
    void  enablePostprocess();

    void disableCheapPostprocess();
    void  enableCheapPostprocess();

    MeshingOptions &meshingOptions();

    bool isPrintable(const std::vector<Real> &params) const;

    Eigen::Matrix<Real, Eigen::Dynamic, Eigen::Dynamic>
    selfSupportingConstraints(const std::vector<Real> &params) const;

    BBox<Point3<Real>> meshingCell() const;

    ~IsosurfaceInflator();

public:
    class Impl;
    Impl *m_imp;
};

#endif /* end of include guard: ISOSURFACEINFLATOR_HH */
