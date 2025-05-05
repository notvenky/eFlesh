////////////////////////////////////////////////////////////////////////////////
// SymmetricMatrix.hh
////////////////////////////////////////////////////////////////////////////////
/*! @file
//  Classes to wrap a flattened symmetric rank 2 tensor (a 6-vector in 3D),
//  allowing it to be treated as a plain symmetric matrix.
//
//  Nearly all implementation is done in ConstSymmetricMatrixBase and
//  SymmetricMatrixBase which encapsulate the flattened data and provide most of
//  the operations needed. These classes work with both reference types
//  (aliases SymmetricMatrixRef and ConstSymmetricMatrixRef) and value types
//  (subclass SymmetricMatrix)
*/
//  Author:  Julian Panetta (jpanetta), julian.panetta@gmail.com
//  Company:  New York University
//  Created:  06/18/2014 17:59:39
////////////////////////////////////////////////////////////////////////////////
#ifndef SYMMETRICMATRIX_HH
#define SYMMETRICMATRIX_HH

#include "Flattening.hh"
#include <Eigen/Dense>
#include <iostream>
#include <type_traits>
#include "utils.hh"

////////////////////////////////////////////////////////////////////////////////
// Forward declarations/aliases
////////////////////////////////////////////////////////////////////////////////
template<size_t t_N, typename Storage>
class SymmetricMatrix;

template<typename Real>
class DynamicSymmetricMatrix;

// The default storage-backed value type
template<typename _Real, size_t t_N>
using SymmetricMatrixValue =
        SymmetricMatrix<t_N, Eigen::Matrix<_Real, flatLen(t_N), 1>>;

// Compile-time mechanism for identifying symmetric matrix types.
class SymmetricMatrixType { };
template<class T>
struct is_symmetric_matrix : public std::is_base_of<SymmetricMatrixType, T> { };

// All symmetric matrix classes currently inherit from ConstSymmetricMatrixBase,
// which in turn inherits from SymmetricMatrixType to support the
// identification mechanism above
template<typename _Real, size_t t_N,
         typename _Storage_t, typename _ConstStorageRef_t>
class ConstSymmetricMatrixBase : public SymmetricMatrixType {
public:
    static_assert(t_N > 0, "Dimension must be positive");
    ConstSymmetricMatrixBase(const ConstSymmetricMatrixBase &b) : m_data(b.m_data) { }
    ConstSymmetricMatrixBase(ConstSymmetricMatrixBase &&b) : m_data(std::move(b.m_data)) { }
    ConstSymmetricMatrixBase(const _Storage_t &data) : m_data(data) { }
    static constexpr size_t N = t_N;
    static constexpr size_t flatSize() { return (N * (N + 1)) / 2; }
    static size_t size() { return N; }

    _Real operator()(size_t i, size_t j) const {
        assert((i < N) && (j < N));
        return operator[](flattenIndices<N>(i, j));
    }

    using EigenvaluesType  = Eigen::Matrix<_Real, N, 1>;
    using EigenvectorsType = Eigen::Matrix<_Real, N, N>;
    using EigenDecompositionType = std::pair<EigenvaluesType, EigenvectorsType>;
    EigenvaluesType eigenvalues() const {
        Eigen::Matrix<_Real, N, N> mat;
        for (size_t j = 0; j < N; ++j)
            for (size_t i = 0; i <= j; ++i)
                mat(i, j) = operator()(i, j);
        return mat.template selfadjointView<Eigen::Upper>().eigenvalues();
    }

    // Get the eigenvalues (first) and eigenvectors (second) of this symmetric
    // matrix. Returns (diag(Lambda), Q) so that this matrix is
    // Q Lambda Q^T
    EigenDecompositionType eigenDecomposition() const {
        Eigen::Matrix<_Real, N, N> mat;
        for (size_t i = 0; i < N; ++i)
            for (size_t j = 0; j < N; ++j)
                mat(i, j) = operator()(i, j);
        Eigen::SelfAdjointEigenSolver<Eigen::Matrix<_Real, N, N>> solver;
        solver.compute(mat);
        return { solver.eigenvalues(), solver.eigenvectors() };
    }

    // Gets the symmetric matrix's eigenvectors, but scaled so their norms are
    // their eigenvalue.
    // Returns a matrix with each scaled eigenvector as a column. The
    // eigenvectors are sorted in decreasing eigenvalue **magnitude** order.
    EigenvectorsType eigenvalueScaledEigenvectors() const {
        auto decomposition = eigenDecomposition();

        std::vector<size_t> perm;
        std::vector<_Real> evMags(N);
        for (size_t i = 0; i < N; ++i) evMags[i] = std::abs(decomposition.first[i]);
        sortPermutation(evMags, perm, true);

        EigenvectorsType result;
        for (size_t i = 0; i < N; ++i)
            result.col(i) = decomposition.second.col(perm[i]) * evMags[perm[i]];

        return result;
    }

    _Real maxEigenvalue() const { return eigenvalues().maxCoeff(); }
    _Real minEigenvalue() const { return eigenvalues().minCoeff(); }
    _Real maxMagnitudeEigenvalue() const {
        auto eigs = eigenvalues();
        _Real maxEig = eigs.maxCoeff();
        _Real minEig = eigs.minCoeff();
        return (maxEig > std::abs(minEig)) ? maxEig : minEig;
    }

    _Real frobeniusNormSq() const { return this->doubleContract(*this); }

    template<typename _R2, typename _S2, typename _CSR2>
    _Real doubleContract(const ConstSymmetricMatrixBase<_R2, t_N, _S2, _CSR2> &b) const {
        // Diagonals
        _Real result(m_data[0] * b[0]);
        for (size_t i = 1; i < t_N; ++i)
            result += m_data[i] * b[i];
        // Off diagonals
        for (size_t i = t_N; i < flatSize(); ++i)
            result += 2 * m_data[i] * b[i];
        return result;
    }

    // Single contraction with vector (apply this matrix to vector on left)
    template<class Derived>
    Eigen::Matrix<typename Derived::Scalar, t_N, 1>
    contract(const Eigen::MatrixBase<Derived> &v) const {
        static_assert((Derived::RowsAtCompileTime == t_N) && (Derived::ColsAtCompileTime == 1),
                      "Invalid vector dimension for single contraction operation.");
        Eigen::Matrix<typename Derived::Scalar, t_N, 1> result;
        result.setZero();
        for (size_t i = 0; i < t_N; ++i) {
            for (size_t j = 0; j < t_N; ++j) {
                result[i] += (*this)(i, j) * v[j];
            }
        }
        return result;
    }
    
    // Applies an change of coordinates using the transformation rule:
    // s_ij' = s_pq R_ip R_jq
    // (When R is a rotation or reflection, this is the correct transformation
    // rule for cartesian tensors).
    // Storage-backed result
    SymmetricMatrixValue<_Real, t_N> transform(const Eigen::Matrix<_Real, t_N, t_N> &R) {
        SymmetricMatrixValue<_Real, t_N> result;
        for (size_t i = 0; i < t_N; ++i) {
            for (size_t j = i; j < t_N; ++j) {
                _Real comp = 0.0;
                for (size_t p = 0; p < t_N; ++p)
                    for (size_t q = 0; q < t_N; ++q)
                        comp += (*this)(p, q) * R(i, p) * R(j, q);
                result(i, j) = comp;
            }
        }
        return result;
    }

    // Flattened accessors
    _ConstStorageRef_t flattened() const { return m_data; }
    _Real operator[](size_t i) const { return m_data[i]; }

    // Allow us to masquarade as an Eigen vector too.
    size_t rows() const { return flatSize(); }

    friend std::ostream &operator<<(std::ostream &os, const ConstSymmetricMatrixBase &m) {
        for (size_t i = 0; i < t_N; ++i) {
            os << m(i, 0);
            for (size_t j = 1; j < t_N; ++j)
                os << " " << m(i, j);
            os << std::endl;
        }
        return os;
    }
protected:
    _Storage_t m_data;

    // SymmetricMatrix needs access to our m_data for efficient copy
    // construction.
    template<size_t t_N2, typename Storage>
    friend class SymmetricMatrix;
};

template<typename _Real, size_t t_N,
         typename _Storage_t, typename _ConstStorageRef_t, typename _StorageRef_t>
class SymmetricMatrixBase : public ConstSymmetricMatrixBase<_Real, t_N, _Storage_t, _ConstStorageRef_t> {
    typedef ConstSymmetricMatrixBase<_Real, t_N, _Storage_t, _ConstStorageRef_t> Base;
public:
    SymmetricMatrixBase(const SymmetricMatrixBase &b) : Base(b) { }
    SymmetricMatrixBase(SymmetricMatrixBase &&b) : Base(std::move(b)) { }
    SymmetricMatrixBase(const _Storage_t &data) : Base(data) { }

    using Base::operator(); // prevent hiding
    _Real &operator()(size_t i, size_t j) {
        assert((i < t_N) && (j < t_N));
        return operator[](flattenIndices<t_N>(i, j));
    }

    template<typename FType>
    void assign(const FType &f) {
        assert(f.rows() == Base::flatSize());
        for (size_t i = 0; i < Base::flatSize(); ++i)
            operator[](i) = f[i];
    }

    // Warning: template hidden by derived class's default operator=, preventing
    // mixed derived assignments unless subclass has "using Base::operator="
    // Note: this can work both for flatened data types and
    // ConstSymmetricMatrixBase due to symmetric matrices' flattened access
    // operators.
    template<typename FType>
    SymmetricMatrixBase &operator=(const FType &f) { assign(f); return *this; }
    SymmetricMatrixBase &operator=(const SymmetricMatrixBase  &b) { m_data =           b.m_data;  return *this; }
    SymmetricMatrixBase &operator=(      SymmetricMatrixBase &&b) { m_data = std::move(b.m_data); return *this; }

    // Assignment from DynamicSymmetricMatrix is special.
    template<typename _R>
    SymmetricMatrixBase &operator=(const DynamicSymmetricMatrix<_R> &b) {
        if (b.size() != t_N) throw std::runtime_error("Dynamic-static size mismatch");
        for (size_t i = 0; i < t_N; ++i) {
            for (size_t j = 0; j <= i; ++j)
                (*this)(i, j) = b(i, j);
        }
        return *this;
    }

    SymmetricMatrixBase &operator*=(_Real s) {
        for (size_t i = 0; i < Base::flatSize(); ++i)
            operator[](i) *= s;
        return *this;
    }

    template<typename FType>
    SymmetricMatrixBase &operator+=(const FType &b) {
        assert(b.rows() == Base::flatSize());
        for (size_t i = 0; i < Base::flatSize(); ++i)
            operator[](i) += b[i];
        return *this;
    }

    // Addition with DynamicSymmetricMatrix is special.
    template<typename _R>
    SymmetricMatrixBase &operator+=(const DynamicSymmetricMatrix<_R> &b) {
        if (b.size() != t_N) throw std::runtime_error("Dynamic-static size mismatch");
        for (size_t i = 0; i < t_N; ++i) {
            for (size_t j = 0; j <= i; ++j)
                (*this)(i, j) += b(i, j);
        }
        return *this;
    }

    template<typename FType>
    SymmetricMatrixBase &operator-=(const FType &b) {
        assert(b.rows() == Base::flatSize());
        for (size_t i = 0; i < Base::flatSize(); ++i)
            operator[](i) -= b[i];
        return *this;
    }

    // Addition with DynamicSymmetricMatrix is special.
    template<typename _R>
    SymmetricMatrixBase &operator-=(const DynamicSymmetricMatrix<_R> &b) {
        if (b.size() != t_N) throw std::runtime_error("Dynamic-static size mismatch");
        for (size_t i = 0; i < t_N; ++i) {
            for (size_t j = 0; j <= i; ++j)
                (*this)(i, j) -= b(i, j);
        }
        return *this;
    }

    void clear() {
        for (size_t i = 0; i < Base::flatSize(); ++i)
            operator[](i) = 0.0;
    }

    // Flattened accessors
    // Bring in the base classes' definitions so they aren't hidden!!!
    using Base::operator[];
    using Base::flattened;
    _StorageRef_t flattened() { return m_data; }
    _Real &operator[](size_t i) { return m_data[i]; }

protected:
    using Base::m_data;
};

// SymmetricMatrixRef and ConstStorageRef are now just aliases to the bases...
template<size_t t_N, typename StorageRef, typename ConstStorageRef>
using SymmetricMatrixRef = SymmetricMatrixBase<typename StorageRef::Scalar, t_N,
                StorageRef, ConstStorageRef, StorageRef>;
template<size_t t_N, typename ConstStorageRef>
using ConstSymmetricMatrixRef = ConstSymmetricMatrixBase<typename
                ConstStorageRef::Scalar, t_N, ConstStorageRef, ConstStorageRef>;

// SymmetricMatrix needs to provide a few extra features that only make sense
// for storage-backed, non-reference-type matrices.
template<size_t t_N, typename Storage>
class SymmetricMatrix
    : public SymmetricMatrixBase<typename Storage::Scalar, t_N,
                                Storage, const Storage &, Storage &>
{
    typedef typename Storage::Scalar _Real;
    typedef SymmetricMatrixBase<_Real, t_N, Storage, const Storage &, Storage &> Base;
public:
    using Base::Base;
    SymmetricMatrix() : Base(Storage::Zero()) { }
    SymmetricMatrix(const SymmetricMatrix  &b) : Base(b) { }
    SymmetricMatrix(      SymmetricMatrix &&b) : Base(std::move(b)) { }
    SymmetricMatrix(size_t i) : Base(Storage::Zero()) {
        if (i >= Base::flatSize())
            throw std::runtime_error("Illegal basis element number.");
        this->operator[](i) = (i < t_N) ? 1.0 : 0.5;
    }
    template<typename _ST2, typename _CSRT2>
    SymmetricMatrix(const ConstSymmetricMatrixBase<_Real, t_N, _ST2, _CSRT2> &b) : Base(b.m_data) { }
    // Construction from general NxN matrix (validates symmetry)
    template<class Derived, typename std::enable_if<(Derived::RowsAtCompileTime == t_N) &&
                                                    (Derived::ColsAtCompileTime == t_N), int>::type = 0>
    SymmetricMatrix(const Eigen::MatrixBase<Derived> &mat): Base(Storage::Zero()) {
        // Build symmetric matrix from upper triangle
        for (size_t i = 0; i < t_N; ++i)
            for (size_t j = i; j < t_N; ++j)
                (*this)(i, j) = mat(i, j);

        // Validate symmetry by checking lower triangle
        for (size_t i = 0; i < t_N; ++i)
            for (size_t j = 0; j < i; ++j)
                if (std::abs((*this)(i, j) - mat(i, j)) > 1e-13)
                    throw std::runtime_error("Attempted to construct SymmetricMatrix from asymmetric matrix");
    }

    // Construct a unit canonical basis symmetric matrix:
    // e_ij = .5 * (e_i e_j^T + e_j e_i^T)
    static SymmetricMatrix CanonicalBasis(size_t i) {
        if (i >= Base::flatSize())
            throw std::runtime_error("Illegal basis element number.");
        SymmetricMatrix e_ij(Storage::Zero());
        e_ij[i] = (i < t_N) ? 1.0 : 0.5;
        return e_ij;
    }

    // Construct 0.5 * (a b^T + b a^T)
    template<class Vector>
    static SymmetricMatrix SymmetrizedOuterProduct(const Vector &a, const Vector &b) {
        static_assert(size_t(Vector::RowsAtCompileTime) == t_N,
                      "Axis vector dimensions must match matrix dimension.");
        SymmetricMatrix ab;
        for (size_t i = 0; i < t_N; ++i)
            for (size_t j = i; j < t_N; ++j)
                ab(i, j) = 0.5 * (a[i] * b[j] + b[i] * a[j]);
        return ab;
    }

    // Construct the symmetric matrix n n^T, which projects onto unit vector n.
    template<class Vector>
    static SymmetricMatrix ProjectionMatrix(const Vector &n) {
        static_assert(size_t(Vector::RowsAtCompileTime) == t_N,
                      "Axis vector dimension must match matrix dimension.");
        SymmetricMatrix nnt;
        for (size_t i = 0; i < t_N; ++i)
            for (size_t j = i; j < t_N; ++j)
                nnt(i, j) = n[i] * n[j];
        return nnt;
    }

    using Base::operator=;
    SymmetricMatrix &operator=(const SymmetricMatrix  &b) { Base::operator=(          b ); return *this; }
    SymmetricMatrix &operator=(      SymmetricMatrix &&b) { Base::operator=(std::move(b)); return *this; }

    SymmetricMatrix operator-() const { SymmetricMatrix result(*this); result *= -1.0; return result; }
};

////////////////////////////////////////////////////////////////////////////////
// Dynamic symmetric matrix--for dynamic cases which can be either 2x2 or 3x3
// Allocates storage for a 3x3, but dynamically chooses to use a subset. 
////////////////////////////////////////////////////////////////////////////////
template<typename Real>
class DynamicSymmetricMatrix : public SymmetricMatrixValue<Real, 3> {
    using Base = SymmetricMatrixValue<Real, 3>;
public:
    // Base constructor initializes the matrix to zero.
    DynamicSymmetricMatrix(size_t n = 3) { resize(n); }
    DynamicSymmetricMatrix(const DynamicSymmetricMatrix  &b) = default;
    DynamicSymmetricMatrix(DynamicSymmetricMatrix &&b) : Base(std::move(b)), m_dynamicSize(b.m_dynamicSize) { }

    // Copy from generic symmetric matrix
    template<typename _Real, size_t _N, typename _ST, typename _CST>
    DynamicSymmetricMatrix(const ConstSymmetricMatrixBase<_Real, _N, _ST, _CST> &sm) {
        resize(_N);
        for (size_t i = 0; i < m_dynamicSize; ++i)
            for (size_t j = i; j < m_dynamicSize; ++j)
                (*this)(i, j) = sm(i, j);
    }

    Real operator()(size_t i, size_t j) const {
        if ((i >= m_dynamicSize) || (j >= m_dynamicSize)) throw std::runtime_error("DynamicSymmetricMatrix index out of bounds.");
        return (*this)[flattenIndices(m_dynamicSize, i, j)];
    }

    Real &operator()(size_t i, size_t j) {
        if ((i >= m_dynamicSize) || (j >= m_dynamicSize)) throw std::runtime_error("DynamicSymmetricMatrix index out of bounds.");
        return (*this)[flattenIndices(m_dynamicSize, i, j)];
    }

    // Note: matrix after resize is zero (doesn't simply
    // clip/pad--that would require a bit more code)
    void resize(size_t n) {
        Base::clear();
        reinterpret_resize(n);
    }

    // Set the dynamic size of the matrix without changing the underlying data.
    // This is useful for manually propogating dimension information to
    // results, e.g. for interpolation.
    void reinterpret_resize(size_t n) {
        if ((n < 2) || (n > 3)) throw std::runtime_error("Only 2x2 and 3x3 matrices supported.");
        m_dynamicSize = n;
    }

    using DVector = Eigen::Matrix<Real, Eigen::Dynamic, 1, 0, 3, 1>;
    DVector eigenvalues() const {
        if (m_dynamicSize == 3) {
            Eigen::Matrix<Real, 3, 3> mat;
            for (size_t j = 0; j < 3; ++j)
                for (size_t i = 0; i <= j; ++i)
                    mat(i, j) = operator()(i, j);
            return mat.template selfadjointView<Eigen::Upper>().eigenvalues();
        }
        else {
            Eigen::Matrix<Real, 2, 2> mat;
            for (size_t j = 0; j < 2; ++j)
                for (size_t i = 0; i <= j; ++i)
                    mat(i, j) = operator()(i, j);
            return mat.template selfadjointView<Eigen::Upper>().eigenvalues();
        }
    }

    // Base::frobeniusNormSq() gives the wrong result for 2x2 :(
    Real frobeniusNormSq() const {
        Real val = 0;
        for (size_t i = 0; i < size(); ++i) {
            for (size_t j = 0; j < size(); ++j) {
                Real e = (*this)(i, j);
                val += e * e;
            }
        }
        return val;
    }

    size_t     size() const { return m_dynamicSize; }
    size_t flatSize() const { return flatLen(m_dynamicSize); }
    ////////////////////////////////////////////////////////////////////////////
    // Assignment/compount assignment operator overloads.
    // These call the base operator after copying/checking dynamic size, but
    // need a static cast to avoid calling Base's opeartor with
    // DynamicSymmetricMatrix RHS (it would falsely throw a size mismatch).
    ////////////////////////////////////////////////////////////////////////////
    DynamicSymmetricMatrix &operator=(const DynamicSymmetricMatrix  &b) { m_dynamicSize = b.m_dynamicSize; Base::operator=(    static_cast<const Base &>(b) ); return *this; }
    DynamicSymmetricMatrix &operator=(      DynamicSymmetricMatrix &&b) { m_dynamicSize = b.m_dynamicSize; Base::operator=(std::move(static_cast<Base &>(b))); return *this; }
    // Base::operator*= scalar is safe, but operator[+/-]= must enforce matching
    // dynamic size
    DynamicSymmetricMatrix &operator+=(const DynamicSymmetricMatrix &b) {
        if (b.m_dynamicSize != m_dynamicSize) throw std::runtime_error("DynamicSymmetricMatrix size mismatch in operator+=");
        // Static cast to avoid calling Base's DynamicSymmetricMatrix RHS
        // version of operator= (it will complain about size mismatch).
        Base::operator+=(static_cast<const Base &>(b));
        return *this;
    }

    DynamicSymmetricMatrix &operator-=(const DynamicSymmetricMatrix &b) {
        if (b.m_dynamicSize != m_dynamicSize) throw std::runtime_error("DynamicSymmetricMatrix size mismatch in operator-=");
        // Static cast to avoid calling Base's DynamicSymmetricMatrix RHS
        // version of operator= (it will complain about size mismatch).
        Base::operator-=(static_cast<const Base &>(b));
        return *this;
    }

private:
    size_t m_dynamicSize;
};


////////////////////////////////////////////////////////////////////////////////
// Arithmetic operators--always have a storage-backed result.
////////////////////////////////////////////////////////////////////////////////
template<typename _Real, size_t t_N,
         typename _Storage_t, typename _ConstStorageRef_t>
SymmetricMatrixValue<_Real, t_N> operator*(_Real s, const ConstSymmetricMatrixBase<_Real, t_N, _Storage_t, _ConstStorageRef_t> &mat)
{
    SymmetricMatrixValue<_Real, t_N> result(mat);
    result *= s;
    return result;
}

template<typename _Real, size_t t_N,
         typename _Storage_t, typename _ConstStorageRef_t>
SymmetricMatrixValue<_Real, t_N> operator*(const ConstSymmetricMatrixBase<_Real, t_N, _Storage_t, _ConstStorageRef_t> &mat, _Real s)
{
    SymmetricMatrixValue<_Real, t_N> result(mat);
    result *= s;
    return result;
}

template<typename _Real>
DynamicSymmetricMatrix<_Real> operator*(_Real s, const DynamicSymmetricMatrix<_Real> &mat)
{
    DynamicSymmetricMatrix<_Real> result(mat);
    result *= s;
    return result;
}

template<typename _Real>
DynamicSymmetricMatrix<_Real> operator*(const DynamicSymmetricMatrix<_Real> &mat, _Real s)
{
    DynamicSymmetricMatrix<_Real> result(mat);
    result *= s;
    return result;
}

#endif /* end of include guard: SYMMETRICMATRIX_HH */
