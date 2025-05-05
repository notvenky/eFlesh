////////////////////////////////////////////////////////////////////////////////
// Geometry.hh
////////////////////////////////////////////////////////////////////////////////
/*! @file
//      Useful geometry-related features and data structures.
*/
//  Author:  Julian Panetta (jpanetta), julian.panetta@gmail.com
//  Company:  New York University
//  Created:  05/04/2012 05:51:46
////////////////////////////////////////////////////////////////////////////////
#ifndef GEOMETRY_HH
#define GEOMETRY_HH

#include "Types.hh"
#include <iostream>
#include <vector>
#include <array>
#include <algorithm>
#include <type_traits>

template<typename _Vector>
struct Region {
    typedef _Vector                 Vector;

    virtual ~Region() = default;

    virtual bool containsPoint(const Vector &/*p*/) const {
        std::cerr << "containsPoint not implemented" << std::endl;
        throw std::runtime_error("containsPoint not implemented");

        return false;
    }

    Vector dimensions() const {
        return this->maxCorner - this->minCorner;
    }

    Vector minCorner, maxCorner;
};

// Point region (corresponds to a very small disk centerd on constructor parameter)
template<typename _Vector>
struct PointRegion : Region<_Vector> {
    typedef _Vector               Vector;
    typedef typename Vector::Scalar Real;

    PointRegion(Vector center) : m_center(center) { }

    virtual bool containsPoint(const Vector &p) const override {
        bool result = false;

        Real distance = (p - m_center).norm();

        if (distance < 1e-5) {
            result = true;
        }

        return result;
    }

private:
    Vector m_center;
};


// Extruded path region
template<typename _Vector>
struct PathRegion : Region<_Vector> {
    typedef _Vector                 Vector;
    typedef typename Vector::Scalar Real;

    // You can construct the polygonal region using a list of points which it is assumed to be in order of edges and
    PathRegion(std::vector<Vector> path) : m_path(path) { }

    virtual bool containsPoint(const Vector &p) const override {
        bool result = false;

        for (size_t i=1; i<m_path.size(); i++) {
            Vector init = m_path[i-1];
            Vector end = m_path[i];

            Real distance = computeDistancePointEdge(init, end, p);

            if (distance < 1e-5) {
                result = true;
            }
        }

        return result;
    }

private:
    std::vector<Vector> m_path;

    Real computeDistancePointEdge(Vector e1, Vector e2, Vector p) const {
        Vector u = p - e1;  // e1 to p
        Vector v = e2 - e1; // e1 to e2

        Real dot = u.dot(v);

        if (v.norm() < 1e-10) {
            std::cerr << "Warning! edge on path region is to small" << std::endl;
        }

        // Term represents unitary projection component of u onto v
        Real uOntoV = dot / v.squaredNorm();

        Vector closest;
        if (uOntoV < 0.0) {
            closest = e1;
        }
        else if (uOntoV > 1.0) {
            closest = e2;
        }
        else {
            closest = e1 + uOntoV * v;
        }

        Vector d = p - closest;
        return d.norm();
    }
};

// General Polygonal region
template<typename _Vector>
struct PolygonalRegion : Region<_Vector> {
    typedef _Vector                 Vector;
    typedef typename Vector::Scalar Real;

    // You can construct the polygonal region using a list of points which it is assumed to be in order
    PolygonalRegion(std::vector<Vector> points) : m_polygonPoints(points) {
        Vector outsidePoint;

        // Find point that I know for sure is outside polygon
        Real smallestX = points[0][0];
        for (unsigned i=1; i < points.size(); i++) {
            if (smallestX > points[i][0]) {
                smallestX = points[i][0];
            }
        }

        m_outsidePoint[0] = smallestX - 1.0;
        m_outsidePoint[1] = 1.90588;
    }

    virtual bool containsPoint(const Vector &p) const override {
        int nIntersections = 0;

        // A point is inside if and only if the edge connecting to an outside point intersect the
        // polygon an odd number of times
        for (size_t i = 0; i<m_polygonPoints.size(); i++) {

            // Define edge we are verifying now
            Vector init = m_polygonPoints[i];
            Vector end = m_polygonPoints[(i+1) % m_polygonPoints.size()];

            if (doesIntersect(init, end, m_outsidePoint, p)) {
                nIntersections++;
            }
        }

        return (nIntersections % 2) == 1;
    }

private:
    // Computes determinant of 2x2 matrix
    Real inline determinant(Vector u, Vector v) const {
        return u[0]*v[1] - u[1]*v[0];
    }

    // Return true iff [a,b] intersects [c,d]
    bool doesIntersect(const Vector &a, const Vector &b, const Vector &c, const Vector &d) const {
        const Real eps = 1e-10; // small epsilon for numerical precision

        Real x = determinant(c - a, d - c);
        Real y = determinant(b - a, a - c);
        Real z = determinant(b - a, d - c);

        if (std::abs(z) < eps || x*z < 0 || x*z > z*z || y*z < 0 || y*z > z*z)
            return false;

        return true;
    }

    std::vector<Vector> m_polygonPoints;
    Vector m_outsidePoint;
};

// Warning: uninitialized/default bboxes are always a dimension-zero bbox around
// the origin. This may lead to unintended behavior if unions are performed
// without care.
template<typename _Vector>
struct BBox : Region<_Vector> {
    // Make sure min/max corner vectors are aligned during dynamic allocation...
    // (Needed for aligned vector types)
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    typedef _Vector                 Vector;
    typedef typename Vector::Scalar Real_;

    BBox() {
        this->minCorner = Vector::Zero();
        this->maxCorner = Vector::Zero();
    }

    BBox(const Vector &minC, const Vector &maxC) {
        this->minCorner = minC;
        this->maxCorner = maxC;
    }

    // Construct dimension-zero bbox around pt
    BBox(const Vector &pt) {
        this->minCorner = pt;
        this->maxCorner = pt;
    }

    // Construct bbox of a collection of points
    template<class _PointCollection>
    BBox(const _PointCollection &vectors) {
        this->minCorner.setZero(), this->maxCorner.setZero();
        size_t i = 0;
        for (const auto &v : vectors) {
            if (i++ == 0) this->minCorner = this->maxCorner = truncateFromND<Vector>(v);
            else          unionPoint(truncateFromND<Vector>(v));
        }
    }

    // Construct bbox of a subset of points. E.g.
    //  _PointCollection = std::vector<MeshIO::IOVertex>
    //  _IndexCollection = std::vector<MeshIO::IOElement>
    template<class _PointCollection, class _IndexCollection>
    BBox(const _PointCollection &pts, const _IndexCollection &subset) {
        this->minCorner.setZero(), this->maxCorner.setZero();
        size_t i = 0;
        for (size_t v : subset) {
            if (i++ == 0) this->minCorner = this->maxCorner = truncateFromND<Vector>(pts[v]);
            else          unionPoint(truncateFromND<Vector>(pts[v]));
        }
    }

    //Vector minCorner, maxCorner;

    void unionBox(const BBox &b) {
        this->minCorner = this->minCorner.cwiseMin(b.minCorner);
        this->maxCorner = this->maxCorner.cwiseMax(b.maxCorner);
    }

    void unionPoint(const _Vector &p) {
        this->minCorner = this->minCorner.cwiseMin(p);
        this->maxCorner = this->maxCorner.cwiseMax(p);
    }

    void intersectBox(const BBox &b) {
        this->minCorner = this->minCorner.cwiseMax(b.minCorner);
        this->maxCorner = this->maxCorner.cwiseMin(b.maxCorner);
    }

    Vector interpolatePoint(const Vector &v) const {
        return this->minCorner +
              (v.array() * (this->maxCorner - this->minCorner).array()).matrix();
    }

    Vector center() const { return 0.5 * (this->minCorner + this->maxCorner); }
    // Clamp a point to the coordinate-wise closest point in the box
    Vector clamp(const Vector &p) {
        return p.cwiseMax(this->minCorner).cwiseMin(this->maxCorner);
    }

    // Get the interpolation coordinates of a point.
    // These are inside [0, 1]^dim if the point is in the box.
    Vector interpolationCoordinates(const Vector &v) const {
        return ((v - this->minCorner).array() / this->dimensions().array()).matrix();
    }

    virtual bool containsPoint(const Vector &p) const override {
        return (p.array() >= this->minCorner.array()).all() &&
               (p.array() <= this->maxCorner.array()).all();
    }

    // Expands the bounding box around its center so that dimension i is
    // increased by factors[i].
    void expand(const Vector &factors) {
        Vector delta = .5 * (factors.array() * this->dimensions().array());
        this->minCorner -= delta;
        this->maxCorner += delta;
    }

    void translate(const Vector &t) {
        this->minCorner += t;
        this->maxCorner += t;
    }

    Real_ volume() const {
        Vector widths = this->maxCorner - this->minCorner;
        Real_ result = 1.0;
        for (int i = 0; i < widths.rows(); ++i)
            result *= widths[i];
        return result;
    }

    bool operator==(const BBox &b) const {
        return ((this->minCorner == b.minCorner) && (this->maxCorner == b.maxCorner));
    }
    bool operator!=(const BBox &b) const { return !(*this == b); }

    ////////////////////////////////////////////////////////////////////////////
    /*! Determine whether there is any overlap with a circle.
    //  Adapted from:
    //  http://stackoverflow.com/questions/401847/ ...
    //         circle-rectangle-collision-detection-intersection/402010#402010
    //  @param[in]  c   circle center
    //  @param[in]  r   circle radius
    //  @return     true if this box overlaps the circle.
    *///////////////////////////////////////////////////////////////////////////
    bool intersectsCircle(const Vector &c, Real_ r) const {
        // Transform so box center is at the origin and the circle is in the
        // first quadrant.
        Vector c_prime = (c - center()).cwiseAbs();

        Vector boxHalfDims = .5 * this->dimensions();
        if ((c_prime.array() > (boxHalfDims.array() + r)).any())
            return false;

        if ((c_prime.array() <= boxHalfDims.array()).any())
            return true;

        return (c_prime - boxHalfDims).squaredNorm() <= r * r;
    }
};

// BBox complement let's you define regions that are complement of others. For example, if
// you want to define a boundary condition on the entire shape except on a small box.
template<typename _Vector>
struct BBoxComplement : Region<_Vector> {
    typedef _Vector                 Vector;
    typedef typename Vector::Scalar Real;

    BBox<Vector> complement;

    BBoxComplement(const Vector &minCorner, const Vector &maxCorner)
            : complement(minCorner, maxCorner) { }

    virtual bool containsPoint(const Vector &p) const override {
        return !complement.containsPoint(p);
    }
};

template<typename T>
std::ostream &operator<<(std::ostream &os, const BBox<T> &b) {
    if (T::RowsAtCompileTime == 2)  {
        os << "[(" << b.minCorner[0] << ", " << b.minCorner[1] << "), "
           <<  '(' << b.maxCorner[0] << ", " << b.maxCorner[1] << ")]";
    }
    else if (T::RowsAtCompileTime == 3) {
        os << "[(" << b.minCorner[0] << ", " << b.minCorner[1] << ", " << b.minCorner[2] << "), "
           <<  '(' << b.maxCorner[0] << ", " << b.maxCorner[1] << ", " << b.maxCorner[2] << ")]";
    }
    else {
        assert(false);
    }

    return os;
}

struct TriangleIndex {
    TriangleIndex() {
        v[0] = v[1] = v[2] = 0;
    }
    TriangleIndex(unsigned int v0, unsigned int v1, unsigned int v2) {
        v[0] = v0; v[1] = v1; v[2] = v2;
    }
    size_t  operator[](unsigned int idx) const { return v[idx]; }
    size_t &operator[](unsigned int idx)       { return v[idx]; }

    template<typename PType>
    TriangleIndex &operator=(const PType &rhs)
    {
        assert(rhs.size() == 3);
        for (int i = 0; i < 3; ++i)
            v[i] = rhs[i];
        return *this;
    }

    unsigned int size() const { return 3; }

    private:
        size_t v[3];
};

// Triplets that compare equal if they hold the same 3 integers regardless of
// order. Useful for representing faces while determining half-face pairs.
struct UnorderedTriplet {
    UnorderedTriplet(int v0, int v1, int v2) {
        m_v[0] = std::min(v0, std::min(v1, v2));
        m_v[2] = std::max(v0, std::max(v1, v2));
        m_v[1] = v0 ^ v1 ^ v2 ^ m_v[0] ^ m_v[2]; // Get the middle
    }

    // Lexicographic comparison
    bool operator<(const UnorderedTriplet &b) const {
        if (m_v[0] < b.m_v[0]) return true;
        if (m_v[0] > b.m_v[0]) return false;
        if (m_v[1] < b.m_v[1]) return true;
        if (m_v[1] > b.m_v[1]) return false;
        return m_v[2] < b.m_v[2];
    }
private:
    int m_v[3];
};

// Pairs that compare equal if they hold the same 2 integers regardless of
// order. Useful for representing edges while determining half-edge pairs.
struct UnorderedPair {
    UnorderedPair() : vmin(-1), vmax(-1) { }
    UnorderedPair(int v0, int v1) { set(v0, v1); }

    void set(int v0, int v1) {
        vmin = std::min(v0, v1);
        vmax = std::max(v0, v1);
    }

    // (accesses in sorted order)
    int operator[](size_t i) const {
        if (i == 0) return vmin;
        if (i == 1) return vmax;
        return -1;
    }

    bool operator==(const UnorderedPair &b) const {
        return (vmin == b.vmin) && (vmax == b.vmax);
    }

    // Lexicographic comparison
    bool operator<(const UnorderedPair &b) const {
        if (vmin < b.vmin) return true;
        if (vmin > b.vmin) return false;
        return vmax < b.vmax;
    }

private:
    int vmin, vmax;
};

// OrientedTriplet instances compare equal if they hold the same 3 integers up
// to cyclic permutation.
// This triplet is cyclically permuted at construction so that the minimum index
// comes first to simplify operations (comparisons, etc.)
// It must always remain this way for correct operation.
struct OrientedTriplet {
    OrientedTriplet(size_t v0, size_t v1, size_t v2) {
        size_t vmin = std::min(std::min(v0, v1), v2);
        if      (v0 == vmin) corners = { { v0, v1, v2 } };
        else if (v1 == vmin) corners = { { v1, v2, v0 } };
        else if (v2 == vmin) corners = { { v2, v0, v1 } };
    }

    bool operator==(const OrientedTriplet &b) const {
        // Note: equal triangles are assumed to have been cyclically
        // permuted so that the index arrays exactly equal
        return b.corners == corners;
    }

    // Check for (combinatorially) degenerate cases
    size_t dimension() const {
        if (corners[0] == corners[1]) {
            if (corners[1] == corners[2])
                return 0;
            return 1;
        }
        if (corners[0] == corners[2]) return 1;
        return (corners[1] == corners[2]) ? 1 : 2;
    }

    // Equivalent edge geometry
    // (Assumes this is a triangle that has degenerated to an edge).
    UnorderedPair degenerateAsEdge() const {
        if (corners[0] == corners[1])
            return UnorderedPair(corners[1], corners[2]);
        return UnorderedPair(corners[0], corners[1]);
    }

    bool containsEdge(const UnorderedPair &e) const {
        bool has_vmin = false, has_vmax = false;
        for (size_t i = 0; i < 3; ++i) {
            has_vmin |= (corners[i] == size_t(e[0]));
            has_vmax |= (corners[i] == size_t(e[1]));
        }
        return has_vmin && has_vmax;
    }

    // Note: minimum index must always come first!
    std::array<size_t, 3> corners;
};

struct UnorderedQuadruplet {
    UnorderedQuadruplet() : m_v{{-1, -1, -1}} { }

    template<typename T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
    UnorderedQuadruplet(const std::vector<T> &idxs) {
        assert(idxs.size() == 4);
        set(idxs[0], idxs[1], idxs[2], idxs[3]);
    }

    UnorderedQuadruplet(int v0, int v1, int v2, int v3) { set(v0, v1, v2, v3); }

    void set(int v0, int v1, int v2, int v3) {
        m_v = {{v0, v1, v2, v3}};
        std::sort(m_v.begin(), m_v.end());
    }

    // std::array has built-in lexicographic operator overloads
    bool operator==(const UnorderedQuadruplet &b) const { return m_v == b.m_v; }
    bool operator< (const UnorderedQuadruplet &b) const { return m_v <  b.m_v; }

private:
    std::array<int, 4> m_v;
};

////////////////////////////////////////////////////////////////////////////////
/*! Uses a barycentric coordinate vector to interpolate three data values
//  @param[in]  coords      bary centric coordinates
//  @param[in]  d0, d1, d2  data values to interpolate
//  @return     interpolated data value
*///////////////////////////////////////////////////////////////////////////////
template<typename BaryCoords, typename DataType>
inline DataType BarycentricInterpolate(const BaryCoords &coords
        , const DataType &d0, const DataType &d1, const DataType &d2)
{
    // Use barycentric coordinates normalized w/ L1 norm
    return (coords[0] * d0  + coords[1] * d1 + coords[2] * d2) /
           (coords[0] + coords[1] + coords[2]);
}

////////////////////////////////////////////////////////////////////////////////
/*! Computes a 2D triangle's circumscribed circle
//  http://en.wikipedia.org/wiki/Circumscribed_circle
//  @param[in]  p0, p1, p2      triangle vertex positions
//  @param[out] center          incircle center
*///////////////////////////////////////////////////////////////////////////////
inline void Circumcircle(const Point2D &p0, const Point2D &p1,
         const Point2D &p2, Point2D &center, Point2D::Scalar &radius)
{
    typedef Point2D::Scalar Real_;
    Point2D e[3];
    e[0] = Point2D(p2 - p1);
    e[1] = Point2D(p0 - p2);
    e[2] = Point2D(p1 - p0);
    Real_ a2 = e[0].dot(e[0]);
    Real_ b2 = e[1].dot(e[1]);
    Real_ c2 = e[2].dot(e[2]);
    Real_ a = sqrt(a2);
    Real_ b = sqrt(b2);
    Real_ c = sqrt(c2);
    Real_ doubleA = e[0][0] * e[1][1] - e[1][0] * e[0][1];
    // Radius =  (a * b * c) / (4A)
    // (a, b, and c are edge lengths, A is area)
    radius = (a * b * c) / (2 * doubleA);
    // Circumcenter Barycentric Coordinates:
    //  (a^2 (b^2 + c^2 - a^2), b^2 (c^2 + a^2 - b^2), c^2 (a^2 + b^2 - c^2))
    Point3D centerBaryCoords(a2 * (b2 + c2 - a2), b2 * (c2 + a2 - b2),
                             c2 * (a2 + b2 - c2));
    center = BarycentricInterpolate(centerBaryCoords, p0, p1, p2);
}

////////////////////////////////////////////////////////////////////////////////
/*! Computes a 2D triangle's inscribed circle
//  http://en.wikipedia.org/wiki/Incircle
//  @param[in]  p0, p1, p2      triangle vertex positions
//  @param[out] center          incircle center
//  @param[out] radius          incircle radius
*///////////////////////////////////////////////////////////////////////////////
inline void Incircle(const Point2D &p0, const Point2D &p1,
         const Point2D &p2, Point2D &center, Point2D::Scalar &radius)
{
    using _Real = Point2D::Scalar;
    Point2D e[3];
    e[0] = Point2D(p2 - p1);
    e[1] = Point2D(p0 - p2);
    e[2] = Point2D(p1 - p0);
    _Real a = e[0].norm();
    _Real b = e[1].norm();
    _Real c = e[2].norm();
    _Real doubleA = e[0][0] * e[1][1] - e[1][0] * e[0][1];
    // Radius =  (2A) / (a + b + c)
    // (a, b, and c are edge lengths, A is area)
    radius = doubleA / (a + b + c);
    // Incenter Barycentric Coordinates: (a, b, c)
    Point3D centerBaryCoords(a, b, c);
    center = BarycentricInterpolate(centerBaryCoords, p0, p1, p2);
}

////////////////////////////////////////////////////////////////////////////
/*! Computes the condition number of a triangle
//  @param[in]  tri     triangle corner indices
//  @param[in]  verts   vertex positions
//  @return     condition number of tri
*///////////////////////////////////////////////////////////////////////////
inline double cond(const TriangleIndex &tri, const std::vector<Point2D> &verts)
{
    Point2D center;
    Point2D::Scalar R, r;
    Circumcircle(verts[tri[0]], verts[tri[1]], verts[tri[2]], center, R);
    Incircle(verts[tri[0]], verts[tri[1]], verts[tri[2]], center, r);
    return .5 * R / r;
}

// Compute the centroid of a polygon in 2D:
// c = 1/A int_P \vec{x}  dx
// The formula used below follows from Green's theorem.
// Assumes that the polygon is non-self-intersecting.
template<class Point>
Point centroid(const std::vector<Point> &p) {
    Point c(Point::Zero());
    Real doubleA = 0;
    for (size_t i = 0; i < p.size(); ++i) {
        const Point &p_i   = p[i];
        const Point &p_ip1 = (i + 1 < p.size()) ? p[i + 1] : p[0];
        Real doubleAContrib = p_i[0] * p_ip1[1] - p_ip1[0] * p_i[1];
        c += (p_i + p_ip1) * doubleAContrib;
        doubleA += doubleAContrib;
    }

    return c / (3 * doubleA);
}

// Compute the signed area of a polygon in 2D (ccw = positive)
//     A = int 1 dV
// Assumes that the polygon is non-self-intersecting.
// The formula used below follows from Green's theorem.
//     A = int div [x, 0] dV = int [x, 0] . n dA
//       = sum_{e in edges} int_e [x, 0] dA . n_e
//       = sum_{e in edges} 1/2 * [x0 + x1, 0] * |e| . n_e
//       = sum_{e in edges} 1/2 * [x0 + x1, 0] . [y1 - y0, x0 - x1]
//       = sum_{e in edges} 1/2 * (x0 + x1) * (y1 - y0)
template<class Polygon>
Real area(const Polygon &poly) {
    assert(poly.size() >= 3);
    Real area = 0;
    for (auto it = poly.begin(); it != poly.end(); ++it) {
        auto next = it;
        ++next;
        if (next == poly.end()) next = poly.begin();
        const auto &p0 = *it;
        const auto &p1 = *next;
        area += 0.5 * (p0[0] + p1[0]) * (p1[1] - p0[1]);
    }
    return area;
}

// Compute the centroid of a point cloud, assuming each point as "mass" 1.
// I.e. compute mean position.
template<class Point>
Point pointCloudCentroid(const std::vector<Point> &points) {
    Point c(Point::Zero());
    for (const Point &p : points) c += p;
    return c / points.size();
}

// Unsigned angle between a and b (in [0, pi])
inline Real angle(const Vector2D &a, const Vector2D &b) { return atan2(std::abs(a[0] * b[1] - a[1] * b[0]), a.dot(b)); }
inline Real angle(const Vector3D &a, const Vector3D &b) { return atan2(a.cross(b).norm(),                   a.dot(b)); }

// Signed angle from a to b. The 3D version requires a normal to define sign,
// while the 2D uses the standard sign convention.
// (in [-pi, pi])
inline Real signedAngle(const Vector2D &a, const Vector2D &b)                    { return atan2(a[0] * b[1] - a[1] * b[0], a.dot(b)); }
inline Real signedAngle(const Vector3D &a, const Vector3D &b, const Vector3D &n) { return atan2(a.cross(b).dot(n),         a.dot(b)); }

// Compute the discrete curvature of a **closed** curve represented, e.g., by a
// list<Point> (neighboring vertices of c[0] are c[i + 1] and c[len - 1].
// The following discrete curvature measure is computed:
//      kappa[i] = (turning angle[i]) / (voronoi length[i])
// Must be called on planar polygons (Point2D) so that signed curvature can be
// computed.
// The standard sign convention is used: left turns (ccw) are positive and right
// turns (cw) are negative
template<class Curve>
std::vector<Real> signedCurvature(const Curve &c) {
    assert(c.size() > 2);
    std::vector<Real> kappa;
    kappa.reserve(c.size());
    auto i = c.end(); --i;
    auto j = c.begin();
    auto k = j; ++k;
    while (j != c.end()) {
        auto ep  = (*k - *j).eval(),
             em  = (*j - *i).eval();
        Real epl = ep.norm(), eml = em.norm();
        Real voronoiLen = 0.5 * (epl + eml);
        kappa.push_back(signedAngle(em, ep) / voronoiLen);

        i = j; ++j; ++k;
        if (k == c.end()) k = c.begin();
    }

    return kappa;
}

#endif // GEOMETRY_HH
