#ifndef INFLATORTYPES_HH
#define INFLATORTYPES_HH

#include "Fields.hh"
#include "Geometry.hh"
#include <Eigen/Dense>
#include <vector>

// Inflator needs templated vector types for automatic differentiation
template<typename Real> using Point3  = Eigen::Matrix<Real, 3, 1>;
template<typename Real> using Point2  = Eigen::Matrix<Real, 2, 1>;
template<typename Real> using Vector3 = Eigen::Matrix<Real, 3, 1>;
template<typename Real> using Vector2 = Eigen::Matrix<Real, 2, 1>;
using  Point3d =  Point3<double>;
using  Point3f =  Point3< float>;
using Vector3d = Vector3<double>;
using Vector3f = Vector3< float>;

using  Point2d =  Point2<double>;
using  Point2f =  Point2< float>;
using Vector2d = Vector2<double>;
using Vector2f = Vector2< float>;

template<size_t N> using  PointNd = Eigen::Matrix<double, N, 1>;
template<size_t N> using  PointNf = Eigen::Matrix< float, N, 1>;
template<size_t N> using VectorNd = Eigen::Matrix<double, N, 1>;
template<size_t N> using VectorNf = Eigen::Matrix< float, N, 1>;

#endif /* end of include guard: INFLATORTYPES_HH */
