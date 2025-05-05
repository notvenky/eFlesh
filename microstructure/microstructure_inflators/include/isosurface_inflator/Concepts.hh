////////////////////////////////////////////////////////////////////////////////
// Concepts.hh
////////////////////////////////////////////////////////////////////////////////
/*! @file
//      Define some tags for "concepts" so we can use enable_if to conditionally
//      enable implementations based on whether a template parameter models a
//      concept.
//
//      These tags are applied by making the model class derive from the tag
//      class.
*/ 
//  Author:  Julian Panetta (jpanetta), julian.panetta@gmail.com
//  Company:  New York University
//  Created:  02/09/2016 00:24:25
////////////////////////////////////////////////////////////////////////////////
#ifndef CONCEPTS_HH
#define CONCEPTS_HH

#include <type_traits>

template<class Concept, class Model>
using models_concept = std::is_base_of<Concept, Model>;

template<class Concept, class Model, class T>
using enable_if_models_concept_t = typename std::enable_if<models_concept<Concept, Model>::value, T>::type;

template<class Concept, class Model, class T>
using enable_if_not_models_concept_t = typename std::enable_if<!models_concept<Concept, Model>::value, T>::type;

namespace Concepts {

// Concept tags
struct Mesh { };
struct TetMesh { };
struct TriMesh { };
struct ElementMesh { };
struct EdgeSoup { };

}

#endif /* end of include guard: CONCEPTS_HH */
