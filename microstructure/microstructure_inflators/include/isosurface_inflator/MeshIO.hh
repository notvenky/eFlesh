////////////////////////////////////////////////////////////////////////////////
// MeshIO.hh
////////////////////////////////////////////////////////////////////////////////
/*! @file
//      Implements I/O for meshes in multiple formats
//
//      Read/write a plain polygon/polyhedron element soup using the functions:
//          load(path, nodes, elements[, format])
//          save(path, nodes, elements[, format])
*/
//  Author:  Julian Panetta (jpanetta), julian.panetta@gmail.com
//  Company:  New York University
//  Created:  05/07/2012 11:55:27
////////////////////////////////////////////////////////////////////////////////
#ifndef MESH_IO_HH
#define MESH_IO_HH

#include "Types.hh"
#include "TemplateHacks.hh"
#include "Concepts.hh"

#include <string>
#include <fstream>
#include <stdexcept>
#include <iomanip>
#include <iostream>
#include <vector>

namespace MeshIO {
    /** Supported file formats */
    typedef enum { FMT_OFF = 0, FMT_OBJ = 1, FMT_MSH = 2, FMT_MSH_ASCII = 3, FMT_POLY = 4, FMT_NODE_ELE = 5, FMT_MEDIT = 6, FMT_STL = 7,
                   FMT_GUESS = -1, FMT_INVALID = -1 } Format;

    typedef enum { MESH_TRI = 0, MESH_TET = 1, MESH_QUAD = 2, MESH_TRI_QUAD = 3, MESH_HEX = 4,
                   MESH_TRI_DEG2 = 5, MESH_TET_DEG2 = 6, MESH_LINE = 7, MESH_LINE_DEG2 = 8,
                   MESH_GUESS = -1, MESH_INVALID = -1 } MeshType;

    ////////////////////////////////////////////////////////////////////////////
    /*! @class IOVertex
    //  Minimal vertex class for unattributed mesh I/O
    *///////////////////////////////////////////////////////////////////////////
    class IOVertex {
        typedef typename Point3D::Scalar _Real;
    public:

        Point3D point;

        IOVertex()                       : point(0, 0, 0) { }
        IOVertex(Real x, Real y, Real z) : point(x, y, z) { }
        IOVertex(const Real *p)          : point(p) { }
        IOVertex(const Point3D &p)       : point(p) { }
        // Padding constructors
        IOVertex(Real x, Real y)         : point(x, y, 0) { }
        IOVertex(const Point2D &p)       : point(p[0], p[1], 0) { }

        void set(_Real x, _Real y, _Real z) {
            point[0] = x; point[1] = y; point[2] = z;
        }

        _Real  operator[](size_t i) const { assert(i < 3); return point[i]; }
        _Real &operator[](size_t i)       { assert(i < 3); return point[i]; }

        operator const Point3D &() const { return point; }
        operator       Point3D &()       { return point; }
        operator Point2D() const { return truncateFrom3D<Point2D>(point); }

        // Lexicographic comparison for simple sorting
        bool operator<(const IOVertex &b) const {
            return std::lexicographical_compare(  point.data(),   point.data() + 3,
                                                b.point.data(), b.point.data() + 3);
        }

        int attribute = 0;
    };

    template<class EmbeddingSpace> EmbeddingSpace truncateFromND(const MeshIO::IOVertex &p) { return ::truncateFromND<EmbeddingSpace, Point3D>(p.point); }

    ////////////////////////////////////////////////////////////////////////////
    /*! @class IOElement
    //  Minimal polygon/polyhedron class for unattributed mesh i/o
    //  Note: inheriting from STL is dangerous because STL containers do not
    //  have virtual destructors. However, since we don't intend to use this
    //  class polymorphically, we should be fine using this hack...
    *///////////////////////////////////////////////////////////////////////////
    class IOElement : public std::vector<size_t> {
        typedef std::vector<size_t> Base;
    public:
        IOElement(size_t n = 0) : Base(n) { }
        // Line (2), Triangle (3), Tet/Quad (4), and Hex (8)
        // Quadratic Triangle (6), Quadratic Tet (10)
        static constexpr bool is_valid_element_size(size_t size) {
            return (size == 2) || (size == 3) || (size ==  4)
                || (size == 8) || (size == 6) || (size == 10);
        }
        template<typename... Args>
        IOElement(size_t v1, size_t v2, Args... args) : Base{v1, v2, static_cast<size_t>(args)...} {
            static_assert(all_integer_parameters<Args...>(), "Vertex indices must all be integers");
            static_assert(is_valid_element_size(2 + sizeof...(Args)), "Index constructor only supports Lines, Triangles, Quads, Tet, and Hex-sized elements");
        }
        
        IOElement(const std::pair<size_t, size_t> &e) : Base{e.first, e.second} { }

        template<typename PType>
        IOElement &operator=(const PType &rhs) {
            Base::resize(rhs.size());
            for (size_t i = 0; i < rhs.size(); ++i)
                (*this)[i] = rhs[i];
            return *this;
        }

        // Lexicographic comparison for simple sorting
        bool operator<(const IOElement &b) const {
            if (b.size() != size()) throw std::runtime_error("Attempted to compare elements of different sizes.");
            return std::lexicographical_compare(  begin(),   end(),
                                                b.begin(), b.end());
        }

        int attribute = 0;
    };

    ////////////////////////////////////////////////////////////////////////////
    /*! IOVertex ASCII input  (for implementing OFF I/O)
    //  @param[in]  is      input stream
    //  @param[out] p       vertex to read
    //  @return     input stream for stream operator chaining
    *///////////////////////////////////////////////////////////////////////////
    std::istream & operator>>(std::istream &is, IOVertex &v);

    ////////////////////////////////////////////////////////////////////////////
    /*! IOVertex ASCII output  (for implementing OFF I/O)
    //  @param[in]  os      output stream
    //  @param[in]  p       vertex to output
    //  @return     output stream for stream operator chaining
    *///////////////////////////////////////////////////////////////////////////
    std::ostream & operator<<(std::ostream &os, const IOVertex &v);

    ////////////////////////////////////////////////////////////////////////////
    /*! IOElement ASCII input  (for implementing OFF I/O)
    //  Format: Nv v0 v1 ... v[Nv - 1]
    //  @param[in]  is  input stream
    //  @param[out] e   element to read
    //  @return     input stream for stream operator chaining
    *///////////////////////////////////////////////////////////////////////////
    std::istream & operator>>(std::istream &is, IOElement &e);

    ////////////////////////////////////////////////////////////////////////////
    /*! IOElement ASCII output  (for implementing OFF I/O)
    //  Format: Nv v0 v1 ... v[Nv - 1]
    //  @param[in]  os  output stream
    //  @param[in]  e   element to output
    //  @return     output stream for stream operator chaining
    *///////////////////////////////////////////////////////////////////////////
    std::ostream & operator<<(std::ostream &os, const IOElement &e);

    ////////////////////////////////////////////////////////////////////////////
    /*! Abstract base functor for supporting various mesh format i/o
    *///////////////////////////////////////////////////////////////////////////
    class MeshIO {
        public:
            typedef IOVertex  Vertex;
            typedef IOElement Element;

            virtual void save(std::ostream &os,
                              const std::vector<Vertex> &nodes,
                              const std::vector<Element> &elements, MeshType t) = 0;
            virtual MeshType load(std::istream &is, std::vector<Vertex> &nodes,
                                  std::vector<Element> &elements, MeshType t) = 0;
            virtual ~MeshIO() { }
    };

    class MeshIO_OFF : public MeshIO {
        public:
            typedef IOVertex  Vertex;
            typedef IOElement Element;

            void save(std::ostream &os, const std::vector<Vertex> &v, const std::vector<Element> &e, MeshType t);
            MeshType load(std::istream &is, std::vector<Vertex> &v, std::vector<Element> &e, MeshType t);
    };

    class MeshIO_OBJ : public MeshIO {
        public:
            typedef IOVertex  Vertex;
            typedef IOElement Element;

            void save(std::ostream &os, const std::vector<Vertex> &v, const std::vector<Element> &e, MeshType t);
            MeshType load(std::istream &is, std::vector<Vertex> &v, std::vector<Element> &e, MeshType t);
    };

    class MeshIO_STL : public MeshIO {
        public:
            typedef IOVertex  Vertex;
            typedef IOElement Element;

            void save(std::ostream &os, const std::vector<Vertex> &v, const std::vector<Element> &e, MeshType t);
            [[ noreturn ]] MeshType load(std::istream &is, std::vector<Vertex> &v, std::vector<Element> &e, MeshType t);
    };

    class MeshIO_POLY : public MeshIO {
        public:
            typedef IOVertex  Vertex;
            typedef IOElement Element;

            void save(std::ostream &os, const std::vector<Vertex> &v, const std::vector<Element> &e, MeshType t);
            [[ noreturn ]] MeshType load(std::istream &is, std::vector<Vertex> &v, std::vector<Element> &e, MeshType t);
    };

    class MeshIO_NodeEle  {
        public:
            typedef IOVertex  Vertex;
            typedef IOElement Element;

            MeshType load(const std::string &nodePath, const std::string &elePath,
                          std::vector<Vertex> &nodes, std::vector<Element>
                          &elements);
    };

    class MeshIO_MSH : public MeshIO {
        public:
            typedef IOVertex  Vertex;
            typedef IOElement Element;

            struct ElementInfo {
                // We can't have default member initializtion and use
                // aggregate initialization (until C++14...), so we provide a
                // constructor that can be used with an initializer list:
                ElementInfo(MeshType mt = MESH_INVALID, int et = -1, size_t npe = 0)
                    : meshType(mt), elementType(et), nodesPerElem(npe) { }
                MeshType meshType;
                int elementType;
                size_t nodesPerElem;
            };

            // Table initialized in MeshIO.cc
            static const std::vector<ElementInfo> elementInfoArray;

            MeshIO_MSH() : m_binary(true) { }

            static const ElementInfo &elementInfoForMeshType(MeshType meshType) {
                for (const auto &ei : elementInfoArray)
                    if (ei.meshType == meshType) return ei;
                throw std::runtime_error("Unsupported MeshType for MSH I/O");
            }

            static const ElementInfo &elementInfoForElementType(int elementType) {
                for (const auto &ei : elementInfoArray)
                    if (ei.elementType == elementType) return ei;
                throw std::runtime_error("Unsupported element type for MSH I/O: "
                                    + std::to_string(elementType));
            }

            static const ElementInfo &elementInfoForNodeCount(size_t nodesPerElem) {
                for (const auto &ei : elementInfoArray)
                    if (ei.nodesPerElem == nodesPerElem) return ei;
                throw std::runtime_error("Unsupported node count for MSH I/O");
            }

            void save(std::ostream &os, const std::vector<Vertex> &nodes,
                      const std::vector<Element> &elements, MeshType type);

            MeshType load(std::istream &is, std::vector<Vertex> &nodes,
                          std::vector<Element> &elements, MeshType type);

            bool binary() const { return m_binary; }
            void setBinary(bool binary) { m_binary = binary; }
        private:
            // Whether parsed input was binary/output will be binary.
            bool m_binary = false;
    };

    // The format used in CGAL
    class MeshIO_Medit : public MeshIO {
    public:
        typedef IOVertex  Vertex;
        typedef IOElement Element;

        void save(std::ostream &/* os */, const std::vector<Vertex> &/* nodes */,
                  const std::vector<Element> &/* elements */, MeshType /* type */);

        MeshType load(std::istream &is, std::vector<Vertex> &nodes,
                      std::vector<Element> &elements, MeshType type);
    };


    ////////////////////////////////////////////////////////////////////////////
    // Functions to query attributes of the different mesh types.
    ////////////////////////////////////////////////////////////////////////////
    // What dimension are the elements? (e.g. triangle meshes are 2D even if
    // embedded in 3D).
    inline constexpr size_t meshDimension(const MeshType &mtype) {
        return  ((mtype == MESH_TRI) || (mtype == MESH_TRI_DEG2) || (mtype == MESH_QUAD) || (mtype == MESH_TRI_QUAD)) ? 2
            : ( ((mtype == MESH_TET) || (mtype == MESH_TET_DEG2) || (mtype == MESH_HEX)) ? 3
                : ( ((mtype == MESH_LINE) || (mtype == MESH_LINE_DEG2)) ? 1 : 0 ) );
    }

    inline constexpr bool isUnknownMesh(const MeshType &mtype) { return mtype == MESH_INVALID; }
    inline constexpr bool isMixedMesh(const MeshType &mtype)   { return mtype == MESH_TRI_QUAD; }

    // Are the elements of the mesh simplices?
    inline constexpr bool isSimplicialComplex(const MeshType &mtype) {
        return ((mtype == MESH_TRI)  || (mtype == MESH_TRI_DEG2) ||
                (mtype == MESH_TET)  || (mtype == MESH_TET_DEG2) ||
                (mtype == MESH_LINE) || (mtype == MESH_LINE_DEG2));
    }

    // Implied order of basis functions on elements in the mesh.
    inline constexpr size_t meshDegree(const MeshType &mtype) {
        return  ((mtype == MESH_TRI     ) || (mtype == MESH_TET     ) || (mtype == MESH_LINE     )) ? 1
            : ( ((mtype == MESH_TRI_DEG2) || (mtype == MESH_TET_DEG2) || (mtype == MESH_LINE_DEG2)) ? 2 : 0 );
    }

    // Number of nodes on the elements of a particular mesh
    // Throws exception for mixed mesh types.
    inline size_t nodesPerElement(const MeshType &mtype) {
        const auto &ei = MeshIO_MSH::elementInfoForMeshType(mtype);
        return ei.nodesPerElem;
    }

    ////////////////////////////////////////////////////////////////////////////
    /*! Guesses the file format of a mesh from its file extension
    //  @param[in]  path    mesh path
    //  @return     file format, or INVALID if the extension wasn't recognized
    *///////////////////////////////////////////////////////////////////////////
    Format guessFormat(const std::string &path);

    ////////////////////////////////////////////////////////////////////////////
    /*! Gets a parser/writer that will work with a particular file format
    //  @param[in]  format  file format
    //  @return     format parser object
    *///////////////////////////////////////////////////////////////////////////
    MeshIO *getMeshIO(Format &format);

    ////////////////////////////////////////////////////////////////////////////
    /*! Writes an element soup to an output stream
    //  @param[in]  path      stream to which geometry is written
    //  @param[in]  nodes     nodes to write
    //  @param[in]  elements  elements to write
    //  @param[in]  format    file format (default: guess from extension)
    //  @param[in]  type      mesh element type (default: guess from first)
    *///////////////////////////////////////////////////////////////////////////
    void save(std::ostream &os, const std::vector<IOVertex> &nodes,
              const std::vector<IOElement> &elements, Format format,
              MeshType type = MESH_GUESS);

    ////////////////////////////////////////////////////////////////////////////
    /*! Writes an element soup to a mesh path
    //  @param[in]  path      the path to which geometry is written
    //  @param[in]  nodes     nodes to write
    //  @param[in]  elements  elements to write
    //  @param[in]  format    file format (default: guess from extension)
    //  @param[in]  type      mesh element type (default: guess from first)
    *///////////////////////////////////////////////////////////////////////////
    void save(const std::string &path, const std::vector<IOVertex> &nodes,
              const std::vector<IOElement> &elements, Format format = FMT_GUESS,
              MeshType type = MESH_GUESS);

    ////////////////////////////////////////////////////////////////////////////
    /*! Writes a mesh with per-vertex positions in vertex field "p"
    //  @param[in]  path      the path to which geometry is written
    //  @param[in]  mesh      mesh to write
    //  @param[in]  format    file format (default: guess from extension)
    //  @param[in]  type      mesh element type (default: guess from first)
    *///////////////////////////////////////////////////////////////////////////
    // Template vodoo to distinguish from EdgeSoup case. Ideally we would add a
    // "MeshConcept" class for better granularity.
    template<class _Mesh>
    enable_if_not_models_concept_t<Concepts::EdgeSoup, _Mesh, void>
    save(const std::string &path, const _Mesh &mesh, Format format = FMT_GUESS, MeshType type = MESH_GUESS) {
        std::vector<IOVertex>  outVertices;
        std::vector<IOElement> outElements;
        outElements.resize(mesh.numElements());
        for (size_t ei = 0; ei < mesh.numElements(); ++ei) {
            auto e = mesh.element(ei);
            for (size_t c = 0; c < e.numVertices(); ++c)
                outElements[ei].push_back(e.vertex(c).index());
        }
        outVertices.reserve(mesh.numVertices());
        // Note: requires vertex-node index to coincide with vertex index!
        // This is the case for our FEMMesh.
        for (size_t vi = 0; vi < mesh.numVertices(); ++vi)
            outVertices.push_back(mesh.node(vi)->p);

        save(path, outVertices, outElements, format, type);
    }

    ////////////////////////////////////////////////////////////////////////////
    /*! Writes an edge soup to an output stream.
    //  @param[in]  path      stream to which geometry is written
    //  @param[in]  edgeSoup  edge soup (point and edge ranges)
    //  @param[in]  format    file format (default: guess from extension)
    //  @param[in]  type      mesh element type (default: guess from first)
    *///////////////////////////////////////////////////////////////////////////
    template<class _EdgeSoup>
    enable_if_models_concept_t<Concepts::EdgeSoup, _EdgeSoup, void>
    save(const std::string &path, const _EdgeSoup &edgeSoup, Format format = FMT_GUESS, MeshType type = MESH_GUESS) {
        if (edgeSoup.points().size() == 0) {
            std::cerr << "WARNING: tried to save empty mesh; skipped." << std::endl;
            return;
        }
        std::vector<IOVertex>  vertices;
        std::vector<IOElement> elements;
        for (const auto &p : edgeSoup.points()) vertices.emplace_back(p);
        for (const auto &e : edgeSoup. edges()) elements.emplace_back(e);
        save(path, vertices, elements, format, type);
    }


    // TODO: higher order mesh output.

    ////////////////////////////////////////////////////////////////////////////
    /*! Reads an element soup from an input stream
    //  @param[in]  is        stream from which to read geometry
    //  @param[out] nodes     nodes to read
    //  @param[out] elements  elements to read
    //  @param[in]  format    file format
    //  @param[in]  type      mesh element type (default: guess from first)
    *///////////////////////////////////////////////////////////////////////////
    MeshType load(std::istream &is, std::vector<IOVertex> &nodes,
              std::vector<IOElement> &elements, Format format,
              MeshType type = MESH_GUESS);

    ////////////////////////////////////////////////////////////////////////////
    /*! Reads an element soup from a mesh path
    //  @param[in]  path      path from which to read geometry
    //  @param[out] nodes     nodes to read
    //  @param[out] elements  elements to read
    //  @param[in]  format    file format (default: guess from extension)
    //  @param[in]  type      mesh element type (default: guess from first)
    //  @return     actual loaded MeshType
    *///////////////////////////////////////////////////////////////////////////
    MeshType load(const std::string &path, std::vector<IOVertex> &nodes,
                  std::vector<IOElement> &elements, Format format = FMT_GUESS,
                  MeshType type = MESH_GUESS);
}

#endif // MESH_IO_HH
