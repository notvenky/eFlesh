#include <polygons.h>
#include <convert.h>
#include <MeshFEM/MeshIO.hh>
#include <MeshFEM/Triangulate.h>
#include <CLI/CLI.hpp>
#include <iostream>
#include <string>
#include <json.hpp>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/create_offset_polygons_2.h>
#include <boost/shared_ptr.hpp>
#include <vector>
#include <cassert>
#include <igl/boundary_loop.h>

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

    print_polygon(poly);

    SsPtr ss = CGAL::create_interior_straight_skeleton_2(poly);
    PolygonPtrVector offset_polygons = CGAL::create_offset_polygons_2<Polygon_2>(offset, *ss);
    print_polygons(offset_polygons);

    typedef std::vector< boost::shared_ptr<Polygon_2> > PolygonVector ;
    for(typename PolygonVector::const_iterator pi = offset_polygons.begin(); pi != offset_polygons.end(); ++ pi) {
        std::vector<Point2D> new_polygon;
        Point2D op;
        for(typename Polygon_2::Vertex_const_iterator vi = (*pi)->vertices_begin(); vi != (*pi)->vertices_end(); ++ vi) {
            op = from_cgal_point_to_meshfem(*vi);
            new_polygon.push_back(op);
        }

        result.push_back(new_polygon);
    }

    return result;
}

double compute_polygon_area(std::vector<Point2D> meshfem_poly) {
    // Transform initial polygon into cgal polygon
    Polygon_2 poly;
    for (size_t i=0; i<meshfem_poly.size(); i++) {
        poly.push_back(from_meshfem_point_to_cgal(meshfem_poly[i]));
    }

    return poly.area();
}

void string_to_double(std::string dataline, std::vector<double> &numbers) {
    std::istringstream stm(dataline);

    double n;
    while(stm >> n) {
        numbers.push_back(n);
    }
}

struct PointComparator2D {

    PointComparator2D(Point2D centroid) : m_centroid(centroid) {}

    bool operator() (Point2D p1, Point2D p2) {
        auto a = p1 - m_centroid;
        auto b = p2 - m_centroid;

        return atan2(a[1], a[0]) < atan2(b[1], b[0]);
    }

    Point2D m_centroid;
};

int main(int argc, char * argv[]) {
    struct {
		std::string input_mesh;
		std::string output_mesh;
		std::string holes_mesh;
		double density = 0.5;
		std::string densities_path = "";
		std::string interior_point = "";
		double offset = 0.0;
	} args;

	CLI::App app{"CheckStarPolygon"};
	app.add_option("input_mesh", args.input_mesh, "Input mesh");
	app.add_option("output_mesh", args.output_mesh, "Output mesh");
	app.add_option("--holes_mesh", args.holes_mesh, "file with holes points");
	app.add_option("--density", args.density, "constant density to apply to all polygons");
	app.add_option("--densities_json", args.densities_path, "density file");
	app.add_option("--interior_point", args.interior_point, "interior point");
	app.add_option("--offset", args.offset, "offset");
	CLI11_PARSE(app, argc, argv);

    std::vector<MeshIO::IOVertex > in_vertices;
    std::vector<MeshIO::IOElement> in_elements;
    std::string in_path = args.input_mesh;

    MeshIO::MeshType type;
    std::cout << "path: " << in_path << std::endl;
    type = load(in_path, in_vertices, in_elements);

    std::vector<double> densities(in_elements.size(), args.density);
    if (args.densities_path.size() > 0) {
        std::ifstream densities_file(args.densities_path);
        nlohmann::json densities_json;
        densities_file >> densities_json;

        int e=0;
        for (auto value : densities_json) {
            densities[e] = value;
            e++;
        }
    }

    std::list<std::list<Point2D>> polygons_list;
    std::vector<Point2D> holes_points;

    /*std::vector<MeshIO::IOVertex > in_hole_points;
    std::vector<MeshIO::IOElement> in_empty_elements;
    std::string holes_path = args.holes_mesh;

    std::cout << "holes path: " << holes_path << std::endl;
    type = load(holes_path, in_hole_points, in_empty_elements);

    for (auto nh : in_hole_points) {
        holes_points.push_back(nh);
    }*/

    if (args.interior_point.size() > 0) {
        std::cout << "Parsing: " << args.interior_point << std::endl;
        std::vector<double> coordinates;
        string_to_double(args.interior_point, coordinates);
        std::cout << "Into " << coordinates.size() << " coordinates..." << std::endl;
        Point2D interior_hole;
        interior_hole <<  coordinates[0], coordinates[1];
        holes_points.push_back(interior_hole);
    }

    for (int e=0; e<in_elements.size(); e++) {
        // 0 - create polygon (list of vertices)
        std::vector<Point2D> orig_poly;
        auto elem = in_elements[e];
        for (int i=0; i<in_elements[e].size();i++) {
            Point2D v = in_vertices[elem[i]];
            orig_poly.push_back(v);
        }
        double original_area = compute_polygon_area(orig_poly);
        std::list<Point2D> outer_poly(orig_poly.begin(), orig_poly.end());

        // 1 - Compute offset polygons
        auto off_polygons = create_offset_polygons(orig_poly, args.offset);

        double remaining_area = 0.0;
        for (auto poly : off_polygons) {
            remaining_area += compute_polygon_area(poly);
        }

        double offset_density = (original_area - remaining_area) / original_area;
        std::cout << "offset: " << args.offset << std::endl;
        std::cout << "original_area: " << original_area << std::endl;
        std::cout << "remaining_area: " << remaining_area << std::endl;
        std::cout << "offset_percentage: " << offset_density << std::endl;

        // Loop through all off polygons
        for (auto poly : off_polygons) {
            // 2 - Create half plane equations for polygon
            Eigen::MatrixXd planes(poly.size(), 3);
            for (int i = 0; i < planes.rows(); ++i) {
                Point2D v1 = poly[(i+1) % poly.size()];
                Point2D v0 = poly[i];

                Point2D v = v1 - v0;
                Point2D normal;
                normal << v(1), -v(0);
                double d = - normal.dot(v0);

                planes(i, 0) = normal(0);
                planes(i, 1) = normal(1);
                planes(i, 2) = d;

                std::cout << "plane " << i << ": " << planes.row(i) << std::endl;
            }

            // 3 - Run intersect half planes to identify kernel
            Eigen::MatrixXd vertices;
            intersect_half_planes(planes, vertices);

            // 4 - if kernel is empty, answer that polygon is not star shaped
            if (vertices.size() == 0) {
                std::cout << "Kernel is empty. Polygon is not star shaped!";
                continue;
            }

            // 5 - Otherwise, compute centroid of kernel
            Point2D fake_centroid;
            fake_centroid << 0.0, 0.0;
            std::vector<Point2D> kernel_vertices;
            for (int i=0; i<vertices.rows(); i++) {
                fake_centroid += vertices.row(i);
                kernel_vertices.push_back(vertices.row(i));
                std::cout << "vertex " << i << ": " << vertices.row(i) << std::endl;
            }
            fake_centroid /= vertices.rows();
            std::cout << "Star shaped polygon has fake centroid at " << fake_centroid << std::endl;

            // 5a - Compute edges assuming order is correct
            PointComparator2D pointComparator2D(fake_centroid);
            std::sort(kernel_vertices.begin(), kernel_vertices.end(), pointComparator2D);
            Point2D last;
            last = fake_centroid;
            std::vector<Point2D> cleaned_vertices;
            for (int i=0; i<kernel_vertices.size(); i++) {
                auto diff = last - kernel_vertices[i];
                if (diff.norm() < 1e-10) {
                    continue;
                }

                std::cout << "sorted vertex " << i << ": " << kernel_vertices[i] << std::endl;

                cleaned_vertices.push_back(kernel_vertices[i]);
                last = kernel_vertices[i];
            }
            kernel_vertices = cleaned_vertices;

            Point2D centroid;
            centroid << 0.0, 0.0;
            double total_area = 0.0;
            size_t n = kernel_vertices.size();
            for (size_t i = 0; i < kernel_vertices.size(); ++i) {
                auto a = kernel_vertices[i] - fake_centroid;
                auto b = kernel_vertices[(i+1) % n] - fake_centroid;

                Point2D tri_centroid = fake_centroid;
                tri_centroid += kernel_vertices[i];
                tri_centroid += kernel_vertices[(i+1) % n];
                tri_centroid /= 3.0;

                double tri_area = 0.5 * (a[0] * b[1] - b[0] * a[1]);
                centroid += tri_area * tri_centroid;
                total_area += tri_area;
            }
            centroid /= total_area;
            std::cout << "Star shaped polygon has kernel centroid at " << centroid << std::endl;

            // 6 - Apply Homothepy transformation to each polygon point
            std::list<Point2D> hole_poly;
            double general_density = densities[e];
            double density = (general_density * original_area - offset_density * original_area)/remaining_area;
            int i = 0;
            std::cout << "original density: " << general_density << std::endl;
            std::cout << "offset density: " << offset_density << std::endl;
            std::cout << "inside density: " << density << std::endl;
            if (density < 0.0) {
                std::cout << "[warning] Density is negative: " << density << std::endl;
                density = 0.0;
            }
            if (abs(density - 1.0) > 1e-4) {
                for (auto p : poly) {
                    double alpha = sqrt(1.0 - density);
                    std::cout << "density (1 - alpha squared): " << density << std::endl;
                    std::cout << "alpha: " << alpha << std::endl;
                    std::cout << "alpha squared: " << alpha * alpha << std::endl;
                    auto hole_point = alpha * p + (1 - alpha) * centroid;
                    hole_poly.push_back(hole_point);
                    std::cout << "hole vertex " << i << ": " << hole_point << std::endl;
                    i++;
                }

                polygons_list.push_back(hole_poly);
                holes_points.push_back(centroid);
            }
        }

        polygons_list.push_back(outer_poly);
    }

    std::vector<MeshIO::IOVertex> out_vertices;
    std::vector<MeshIO::IOElement> out_triangles;

    triangulatePSLC(polygons_list, holes_points, out_vertices, out_triangles, 0.01, "");

    save(args.output_mesh, out_vertices, out_triangles);

    return 0;
}