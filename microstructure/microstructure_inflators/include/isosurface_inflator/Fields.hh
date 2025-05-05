////////////////////////////////////////////////////////////////////////////////
// Fields.hh
////////////////////////////////////////////////////////////////////////////////
/*! @file
//  Classes implementing vector/scalar/tensor fields. Each class also specifies
//  how the quantities are flattened into a single 1D array. Field samples are
//  stored as columns of a dim x |D| 2D array that is then flattened in column
//  major format. Here |D| is the size of the discrete domain.
//
//  This means, for symmetric tensor fields, there are two flattenings: first
//  each sample is flattened into a 6-vector (in 3D) using Voigt notation, then
//  each 6-vector is stored as a column in a 6 x |D| array, which is flattened
//  into a 6 |D| vector.
//
//  For vector fields, the resulting flattened vector looks like:
//      [v_0x, v_0y, v_0z, v_1x, ..., v_|D|z]
//  This vector can be obtained with the getFlattened() method.
*/
//  Author:  Julian Panetta (jpanetta), julian.panetta@gmail.com
//  Company:  New York University
//  Created:  02/13/2013 16:27:14
////////////////////////////////////////////////////////////////////////////////
#ifndef FIELDS_HH
#define FIELDS_HH
#include <Eigen/Dense>
#include <vector>
#include <string>
#include <cassert>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <stdexcept>
#include <cmath>
#include <limits>

#include "Flattening.hh"
#include "SymmetricMatrix.hh"

#include "Algebra.hh"

typedef enum { FIELD_SCALAR, FIELD_VECTOR, FIELD_MATRIX} FieldType;
enum class DomainType  { PER_ELEMENT = 0, PER_NODE = 1, ANY = 3, GUESS = 3, UNKNOWN = -1};

template<typename Real, size_t t_dim>
class VectorField : public VectorSpace<Real, VectorField<Real, t_dim>> {
public:
    typedef Eigen::Matrix<Real, Eigen::Dynamic, 1> FlattenedType;
    typedef Eigen::Matrix<Real, t_dim, Eigen::Dynamic> ArrayType;
    typedef typename ArrayType::ColXpr         ValueType;
    typedef typename ArrayType::ConstColXpr    ConstValueType;

    // Copy and move constructors/assignment
    VectorField(const VectorField  &b) = default;
    VectorField(      VectorField &&b) = default;
    VectorField &operator=(const VectorField  &b) = default;
    VectorField &operator=(      VectorField &&b) = default;

    // Flattened data constructor
    // Note: copies data
    VectorField(const FlattenedType &values) {
        size_t domainSize = values.rows() / t_dim;
        assert(t_dim * domainSize == (size_t) values.rows());
        m_values = Eigen::Map<const ArrayType>(values.data(), t_dim,
                                               domainSize);
    }

    // Flattened data constructor (std::vector version)
    template<typename Real2>
    VectorField(const std::vector<Real2> &values) {
        size_t domainSize = values.size() / t_dim;
        if (t_dim * domainSize != values.size())
            throw std::runtime_error("Invalid flattened field size (not an " + std::to_string(t_dim) + "D field)");
        m_values = Eigen::Map<const Eigen::Matrix<Real2, t_dim, Eigen::Dynamic> >
            (&values[0], t_dim, domainSize);
    }

    // Uninitialized allocation constructor
    explicit VectorField(size_t domainSize = 0)
        : m_values(t_dim, domainSize) { }

    ConstValueType operator()(size_t i) const {
        assert(i < (size_t) m_values.cols());
        return m_values.col(i);
    }

    ValueType operator()(size_t i) {
        assert(i < (size_t) m_values.cols());
        return m_values.col(i);
    }

    void clear() { m_values = ArrayType::Zero(dim(), domainSize()); }

    ////////////////////////////////////////////////////////////////////////////
    // VectorSpace requirements
    ////////////////////////////////////////////////////////////////////////////
    void Add(const VectorField &b) { m_values += b.m_values; }
    void Scale(Real scalar)        { m_values *= scalar; }

    // Normalize data so that the maximum column magnitude is 1.
    void maxColumnNormalize() { m_values /= maxMag(); }

    ////////////////////////////////////////////////////////////////////////////
    // Non-VectorSpace operations.
    ////////////////////////////////////////////////////////////////////////////
    Real maxMag() const {
        Real maxNorm = 0;
        for (size_t i = 0; i < domainSize(); ++i)
            maxNorm = std::max(maxNorm, m_values.col(i).norm());
        return maxNorm;
    }

    Real minMag() const {
        Real minNorm = std::numeric_limits<Real>::max();
        for (size_t i = 0; i < domainSize(); ++i)
            minNorm = std::min(minNorm, m_values.col(i).norm());
        return minNorm;
    }

    // Component wise abs.
    VectorField cwiseAbs() const { auto r = VectorField(*this); r.m_values = r.m_values.cwiseAbs(); return r; }

    // Set all coefficients to a constant
    void setConstant(Real val) { m_values.setConstant(val); }

    // Sum of squared norms of each vector.
    Real frobeniusNormSq() const {
        Real normSq = 0;
        for (size_t i = 0; i < domainSize(); ++i)
            normSq += m_values.col(i).squaredNorm();
        return normSq;
    }

    // Scalar-valued inner product between vector field: apply dot product
    // pointwise and sum.
    Real innerProduct(const VectorField &b) const {
        assert(domainSize() == b.domainSize());
        Real result = 0;
        for (size_t i = 0; i < domainSize(); ++i)
            result += m_values.col(i).dot(b.m_values.col(i));
        return result;
    }

    // Unweighted mean vector.
    Eigen::Matrix<Real, t_dim, 1> mean() const {
        Eigen::Matrix<Real, t_dim, 1> result;
        result.setZero();
        for (size_t i = 0; i < domainSize(); ++i)
            result += m_values.col(i);
        result *= (1.0 / domainSize());
        return result;
    }

    const ArrayType &data() const { return m_values; }
          ArrayType &data()       { return m_values; }

    size_t dim() const { return t_dim; }
    size_t N()   const { return dim(); }
    size_t domainSize() const { return m_values.cols(); }
    FieldType fieldType() const { return FIELD_VECTOR; }

    void resizeDomain(size_t dSize) {
        m_values.resize(Eigen::NoChange, dSize);
        clear();
    }

    // Flattened access
    size_t size() const { return dim() * domainSize(); }
    void resize(size_t i) { assert(i % dim() == 0); resizeDomain(i / dim()); }
          Real &operator[](size_t i)       { assert(i < size()); return m_values.data()[i]; }
    const Real &operator[](size_t i) const { assert(i < size()); return m_values.data()[i]; }

    template<typename Real2>
    void getFlattened(std::vector<Real2> &v) const {
        v.resize(size());
        for (size_t i = 0; i < size(); ++i)
            v[i] = operator[](i);
    }

    void print(std::ostream &os, const std::string &componentSeparator = "\t",
               const std::string &elementPrefix = "",
               const std::string &elementSuffix = "\n",
               const std::string &elementSeparator = "") const {
        for (size_t i = 0; i < domainSize(); ++i) {
            if (i) os << elementSeparator;
            ConstValueType v = (*this)(i);
            os << elementPrefix << v[0];
            for (size_t j = 1; j < t_dim; ++j) {
                os << componentSeparator << v[j];
            }
            os << elementSuffix;
        }
    }

    void dump(const std::string &path) const {
        std::ofstream of(path);
        if (!of.is_open())
            throw std::runtime_error(std::string("Couldn't open '") +
                        path + "' for writing.");
        of << std::scientific << std::setprecision(16);
        print(of);
    }

protected:
    /** Data storage */
    ArrayType m_values;
};

template<typename Real>
class ScalarField : public VectorField<Real, 1> {
public:
    using typename VectorField<Real, 1>::FlattenedType;
    typedef Real value_type;

    // ScalarField's value type should act both like a vector (to mimic
    // base class VectorField<Real, 1>) and like a scalar (via typecasts)
    class ValueType {
    public:
        ValueType(Real &val) : m_val(val) { }
              Real &operator[](size_t i)       { (void) (i); assert(i == 0); return m_val; }
        const Real &operator[](size_t i) const { (void) (i); assert(i == 0); return m_val; }
        operator Real&()      { return m_val; }
        operator Real() const { return m_val; }
        ValueType &operator=(Real val) { m_val = val; return *this; }
    private:
        Real &m_val;
    };
    class ConstValueType {
    public:
        ConstValueType(const Real &val) : m_val(val) { }
        Real  operator[](size_t i) const { (void) i; assert(i == 0); return m_val; }
        operator Real() const { return m_val; }
    private:
        const Real &m_val;
    };

    ScalarField(const FlattenedType &values)
        : VectorField<Real, 1>(values) { }
    explicit ScalarField(size_t domainSize = 0)
        : VectorField<Real, 1>(domainSize) { }
    template<typename Real2>
    ScalarField(const std::vector<Real2> &values)
        : VectorField<Real, 1>(values) { }
    // Allow construction from 1-dim vector.
    template<typename Real2>
    ScalarField(const VectorField<Real2, 1> &values)
        : VectorField<Real, 1>(values) { }

    FieldType fieldType() const { return FIELD_SCALAR; }

    Real squaredNorm() const { return m_values.squaredNorm(); }
    Real norm() const { return m_values.norm(); }
    Real  sum() const { return m_values.sum(); }
    Real  min() const { return m_values.minCoeff(); }
    Real  max() const { return m_values.maxCoeff(); }

    // Return the entry with maximum/minimum magnitude
    Real minMag() const { Real m = min(), M = max(); return (std::abs(m) < M) ? m : M; }
    Real maxMag() const { Real m = min(), M = max(); return (std::abs(m) > M) ? m : M; }

    // Component wise abs.
    ScalarField cwiseAbs() const { auto r = ScalarField(*this); r.m_values = r.m_values.cwiseAbs(); return r; }

    // operator() should return numbers rather than column vectors...
    ConstValueType operator()(size_t i) const {
        assert(i < (size_t) m_values.cols());
        return ConstValueType(m_values(0, i));
    }
    ValueType operator()(size_t i) {
        assert(i < (size_t) m_values.cols());
        return ValueType(m_values(0, i));
    }

    const Real *data() const { return m_values.data(); }
          Real *data()       { return m_values.data(); }
    template<size_t dim>
    VectorField<Real, dim> unflatten() const {
        return VectorField<Real, dim>(m_values);
    }

    const typename VectorField<Real, 1>::ArrayType &values() const { return m_values; }


    void minRelax(const ScalarField<Real> &b) { m_values = m_values.cwiseMin(b.m_values); }
    void maxRelax(const ScalarField<Real> &b) { m_values = m_values.cwiseMax(b.m_values); }
    void minRelax(Real b) { m_values = m_values.cwiseMin(b); }
    void maxRelax(Real b) { m_values = m_values.cwiseMax(b); }

private:
    using VectorField<Real, 1>::m_values;
};

// Handles both VectorField and ScalarField output.
template<typename Real, size_t N>
std::ostream &operator<<(std::ostream &os, const VectorField<Real, N> &vf) {
    for (size_t i = 0; i < vf.domainSize(); ++i) {
        for (size_t c = 0; c < N; ++c) {
            os << (c ? "\t" : "") << vf(i)[c];
        }
        os << std::endl;
    }

    return os;
}

// Symmetric matrix NxN fields need only store the upper triangle of the NxN
// matrix. This triangle is flattened into a 1D vector following Voigt notation.
//  [ 0 2 ]   [ 0 5 4 ]  ...  [ 0  N*(N+1)/2 -1  ]
//  [   1 ]   [   1 3 ]       [    1             ]
//            [     2 ]       [        2     ... ]
//                            [         ..   N+1 ]
//                            [           .. N   ]
//                            [              N-1 ]
// This is the typical stress/strain flattening that
// collects the diagonal xx, yy, ... entries at the beginning
// The total number of entries is sum_{i=1}^N i = (N * (N + 1)) / 2
// (because there are i entries in the ith column).
template<typename Real, size_t t_N>
class SymmetricMatrixField : public VectorSpace<Real, SymmetricMatrixField<Real, t_N>> {
public:
    typedef Eigen::Matrix<Real, Eigen::Dynamic, 1> FlattenedType;
    typedef Eigen::Matrix<Real, flatLen(t_N), Eigen::Dynamic> ArrayType;

    typedef SymmetricMatrixRef<t_N, typename ArrayType::ColXpr,
            const typename ArrayType::ColXpr> ValueType;
    typedef ConstSymmetricMatrixRef<t_N,
            typename ArrayType::ConstColXpr> ConstValueType;

    SymmetricMatrixField(size_t domainSize, const FlattenedType &values) {
        assert(dim() * domainSize == values.rows());
        m_values = Eigen::Map<const ArrayType>(values.data(), dim(),
                                               domainSize);
    }

    // Eigen ArrayType constructor
    SymmetricMatrixField(const ArrayType values) : m_values(values) { }

    SymmetricMatrixField(size_t domainSize = 0)
        : m_values(dim(), domainSize) { }

    constexpr size_t dim() const { return flatLen(t_N); }
    size_t N()   const { return t_N; }
    size_t domainSize() const { return m_values.cols(); }
    FieldType fieldType() const { return FIELD_MATRIX; }

    void clear() { m_values = ArrayType::Zero(dim(), domainSize()); }
    void resizeDomain(size_t dSize) {
        m_values.resize(Eigen::NoChange, dSize);
        clear();
    }

    ConstValueType operator()(size_t i) const {
        return ConstValueType(m_values.col(i));
    }

    ValueType operator()(size_t i) {
        return ValueType(m_values.col(i));
    }

    ////////////////////////////////////////////////////////////////////////////
    // VectorSpace requirements
    ////////////////////////////////////////////////////////////////////////////
    void Add(const SymmetricMatrixField &b) { m_values += b.m_values; }
    void Scale(Real scalar)                 { m_values *= scalar; }

    ////////////////////////////////////////////////////////////////////////////
    // Non-VectorSpace operations.
    ////////////////////////////////////////////////////////////////////////////
    // MHS on Nov 3 3015
    SymmetricMatrixField &operator/=(const ScalarField<Real> &scalars) {
        assert(scalars.domainSize() == size_t(m_values.cols()));
        for (size_t i = 0; i < scalars.domainSize(); ++i)
            m_values.col(i) /= scalars(i);
        return *this;
    }

    // Component wise abs.
    SymmetricMatrixField cwiseAbs() const { return SymmetricMatrixField(m_values.cwiseAbs()); }
    // Set all coefficients to a constant
    void setConstant(Real val) { m_values.setConstant(val); }

    SymmetricMatrixField &operator=(const SymmetricMatrixField &b) {
        if (this == &b) return *this;
        m_values = b.m_values;
        return *this;
    }

    const ArrayType &data() const { return m_values; }
          ArrayType &data()       { return m_values; }

    void dump(const std::string &path) const {
        std::ofstream of(path);
        if (!of.is_open())
            throw std::runtime_error(std::string("Couldn't open '") +
                        path + "' for writing.");
        of << std::scientific << std::setprecision(16);
        for (size_t i = 0; i < domainSize(); ++i) {
            ConstValueType v = (*this)(i);
            of << v[0];
            for (size_t j = 1; j < dim(); ++j) {
                of << '\t' << v[j];
            }
            of << std::endl;
        }
    }

    void load(const std::string &path) {
        std::ifstream is(path);
        if (!is.is_open())
            throw std::runtime_error(std::string("Couldn't open '") + path);

        std::string line;
        std::vector<Real> data;
        while (std::getline(is >> std::ws, line)) {
            std::vector<Real> v;
            std::istringstream iss(line);
            Real c;
            size_t i = 0;
            while (iss >> c) { data.push_back(c); ++i; }
            if (i != dim()) throw std::runtime_error("Read wrong number of components.");
        }
        assert(data.size() % dim() == 0);
        int domSize = data.size() / dim();
        m_values = Eigen::Map<const ArrayType>(&data[0], dim(), domSize);
    }

private:
    /** Data storage */
    ArrayType m_values;
};

template<typename Real, size_t N>
std::ostream &operator<<(std::ostream &os, const SymmetricMatrixField<Real, N> &smf)
{
    for (size_t i = 0; i < smf.domainSize(); ++i) {
        for (size_t c = 0; c < smf.dim(); ++c) {
            os << (c ? "\t" : "") << smf(i)[c];
        }
        os << std::endl;
    }
    return os;
}

// Simple field class that can change dimension but is less efficient/statically
// checked.
// Stores in flattened x0 y0 x1 y1 ... format
template<typename _Real>
class DynamicField {
public:
    DynamicField(size_t dimensions, size_t domSize) {
        resize(dimensions, domSize);
    }

    DynamicField(const DynamicField &b) {
        m_dim = b.m_dim;
        m_storage = b.m_storage;
    }

    template<size_t _N>
    DynamicField(const VectorField<_Real, _N> &vf) {
        resize(vf.dim(), vf.domainSize());
        for (size_t i = 0; i < vf.dim(); ++i)
            for (size_t j = 0; j < vf.domainSize(); ++j)
                (*this)(i, j) = vf(j)[i];
    }

    void resize(size_t domSize) { m_storage.resize(domSize * m_dim); }
    void resize(size_t dim, size_t domSize) { m_dim = dim; resize(domSize); }

    size_t domainSize() const {
        assert(m_storage.size() % m_dim == 0);
        return m_storage.size() / m_dim;
    }

    size_t dim() const { return m_dim; }

    // Flattened access
          _Real &operator[](size_t i)       { return m_storage.at(i); }
    const _Real &operator[](size_t i) const { return m_storage.at(i); }

    _Real &operator()(size_t i, size_t j) {
        if (i >= dim() || j >= domainSize()) throw std::runtime_error("out of bounds access");
        return m_storage.at(j * dim() + i);
    }

    _Real  operator()(size_t i, size_t j) const {
        if (i >= dim() || j >= domainSize()) throw std::runtime_error("out of bounds access");
        return m_storage.at(j * dim() + i);
    }

    // Casts to Field types.
    operator ScalarField<_Real>() const {
        if (m_dim != 1) throw std::runtime_error("Illegal cast of vector field to scalar field.");
        return ScalarField<_Real>(m_storage);
    }
    template<size_t _dim>
    operator VectorField<_Real, _dim>() const {
        if (m_dim != _dim) throw std::runtime_error("Vector field cast dimension mismatch.");
        return VectorField<_Real, _dim>(m_storage);
    }
    template<size_t _dim>
    operator SymmetricMatrixField<_Real, _dim>() const {
        if (m_dim != _dim) throw std::runtime_error("Vector field cast dimension mismatch.");
        return SymmetricMatrixField<_Real, _dim>(m_storage);
    }

    friend std::ostream &operator<<(std::ostream &os, const DynamicField &f) {
        for (size_t j = 0; j < f.domainSize(); ++j) {
            for (size_t i = 0; i < f.dim(); ++i)
                os << (i ? "\t" : "") << f(i, j);
            os << std::endl;
        }
        return os;
    }

private:
    size_t m_dim;
    std::vector<_Real> m_storage;
};

#endif // FIELDS_HH
