////////////////////////////////////////////////////////////////////////////////
// Flattening.hh
////////////////////////////////////////////////////////////////////////////////
/*! @file
//		Collects flattening arithmetic and conventions into one place to ensure
//		consistency across the entire codebase. For derivations, see
//		    doc/meshless_fem/TensorFlattening.pdf
*/ 
//  Author:  Julian Panetta (jpanetta), julian.panetta@gmail.com
//  Company:  New York University
//  Created:  06/06/2014 15:02:17
////////////////////////////////////////////////////////////////////////////////
#ifndef FLATTENING_HH
#define FLATTENING_HH
#include <cstddef>

// Length of a flattened rank 2 tensor in "dim" dimensions.
// This is also the row and column size of the flattened rank 4 tensor.
constexpr size_t flatLen(size_t dim) { return (dim == 3) ? 6 : 3; }

// Implements flattening of symmetric 2D indices into 1D indices
constexpr size_t flattenIndices(size_t dim, size_t i, size_t j) {
    return (i == j) ? i :
           ((i < j) ? (dim * (dim + 1) - j * (j - 1)) / 2 - (i + 1)
                    : (dim * (dim + 1) - i * (i - 1)) / 2 - (j + 1));
}

// Optimized version...
template<size_t _Dim>
inline constexpr size_t flattenIndices(size_t i, size_t j);

// 02
//  1
template<>
inline constexpr size_t flattenIndices<2>(size_t i, size_t j) {
    return (i == j) ? i : 2;
}

// 054
// 513
// 432
template<>
inline constexpr size_t flattenIndices<3>(size_t i, size_t j) {
    return (i == j) ? i :
            ((i < j) ? ((j == 2) ? 4 - i : 5)
                     : ((i == 2) ? 4 - j : 5));
}

#endif /* end of include guard: FLATTENING_HH */
