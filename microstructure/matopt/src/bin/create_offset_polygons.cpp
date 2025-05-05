#include <MeshFEM/MeshIO.hh>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/create_offset_polygons_2.h>
#include <boost/shared_ptr.hpp>
#include <vector>
#include <cassert>

typedef CGAL::Exact_predicates_inexact_constructions_kernel K ;
typedef K::Point_2                   Point ;
typedef CGAL::Polygon_2<K>           Polygon_2 ;
typedef CGAL::Straight_skeleton_2<K> Ss ;
typedef boost::shared_ptr<Polygon_2> PolygonPtr ;
typedef boost::shared_ptr<Ss> SsPtr ;
typedef std::vector<PolygonPtr> PolygonPtrVector ;

Point2D from_cgal_point_to_meshfem(Point cgal_point) {
    Point2D result;
    double x = cgal_point.x();
    double y = cgal_point.y();
    result << x, y;

    return result;
}

Point from_meshfem_point_to_cgal(Point2D meshfem_point) {
    Point result(meshfem_point[0], meshfem_point[1]);

    return result;
}

template<class K>
void print_point ( CGAL::Point_2<K> const& p )
{
  Point p1 = from_meshfem_point_to_cgal(from_cgal_point_to_meshfem(p));

  std::cout << "(" << p.x() << "," << p.y() << ")" << " / " << "(" << p1.x() << "," << p1.y() << ")" ;
}

template<class K, class C>
void print_polygon ( CGAL::Polygon_2<K,C> const& poly )
{
  typedef CGAL::Polygon_2<K,C> Polygon ;

  std::cout << "Polygon with " << poly.size() << " vertices" << std::endl ;

  for( typename Polygon::Vertex_const_iterator vi = poly.vertices_begin() ; vi != poly.vertices_end() ; ++ vi )
  {
    print_point(*vi); std::cout << std::endl ;
  }
}

template<class K, class C>
void print_polygons ( std::vector< boost::shared_ptr< CGAL::Polygon_2<K,C> > > const& polies )
{
  typedef std::vector< boost::shared_ptr< CGAL::Polygon_2<K,C> > > PolygonVector ;

  std::cout << "Polygon list with " << polies.size() << " polygons" << std::endl ;

  for( typename PolygonVector::const_iterator pi = polies.begin() ; pi != polies.end() ; ++ pi )
    print_polygon(**pi);
}

std::vector<std::vector<Point2D>> create_offset_polygons(std::vector<Point2D> meshfem_poly, double offset = 0.0) {
    std::vector<std::vector<Point2D>> result;

    // Transform initial polygon into cgal polygon
    Polygon_2 poly;
    for (size_t i=0; i<meshfem_poly.size(); i++) {
        poly.push_back(from_meshfem_point_to_cgal(meshfem_poly[i]));
    }

    SsPtr ss = CGAL::create_interior_straight_skeleton_2(poly);
    PolygonPtrVector offset_polygons = CGAL::create_offset_polygons_2<Polygon_2>(offset, *ss);
    print_polygons(offset_polygons);

    typedef std::vector< boost::shared_ptr<Polygon_2> > PolygonVector ;
    for(typename PolygonVector::const_iterator pi = offset_polygons.begin(); pi != offset_polygons.end(); ++ pi) {
        std::vector<Point2D> new_polygon;
        Point2D op;
        for(typename Polygon_2::Vertex_const_iterator vi = poly.vertices_begin(); vi != poly.vertices_end(); ++ vi) {
            op = from_cgal_point_to_meshfem(*vi);
            new_polygon.push_back(op);
        }

        result.push_back(new_polygon);
    }

    return result;
}

int main()
{
    std::vector<Point2D> poly;
    Point2D p;
    p << 0.0, 0.0; poly.push_back(p);
    p << 2.0, 0.0; poly.push_back(p);
    p << 2.0, 2.0; poly.push_back(p);
    p << 1.0, 1.0; poly.push_back(p);
    p << 0.0, 2.0; poly.push_back(p);

    auto result = create_offset_polygons(poly, 0.5);

    return 0;
}