#include <polygons.h>
#include <MeshFEM/MeshIO.hh>
#include <MeshFEM/Triangulate.h>
#include <CLI/CLI.hpp>
#include <iostream>
#include <string>
#include <json.hpp>

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
		double density = 0.5;
		std::string densities_path = "";
		std::string interior_point = "";
	} args;

	CLI::App app{"CheckStarPolygon"};
	app.add_option("input_mesh", args.input_mesh, "Input mesh");
	app.add_option("output_mesh", args.output_mesh, "Output mesh");
	app.add_option("--density", args.density, "constant density to apply to all polygons");
	app.add_option("--densities_json", args.densities_path, "density file");
	app.add_option("--interior_point", args.interior_point, "interior point");
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
        std::vector<Point2D> poly;
        auto elem = in_elements[e];
        for (int i=0; i<in_elements[e].size();i++) {
            Point2D v = in_vertices[elem[i]];
            poly.push_back(v);
        }
        std::list<Point2D> outer_poly(poly.begin(), poly.end());

        // 1 - Create half plane equations for polygon
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

        // 2 - Run intersect half planes to identify kernel
        Eigen::MatrixXd vertices;
        intersect_half_planes(planes, vertices);

        // 3 - if kernel is empty, answer that polygon is not star shaped
        if (vertices.size() == 0) {
            std::cout << "Kernel is empty. Polygon is not star shaped!";
            continue;
        }

        // 4 - Otherwise, compute centroid of kernel
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

        // 4a - Compute edges assuming order is correct
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

        /*std::vector<std::pair<size_t, size_t>> kernel_edges;
        for (size_t i = 0; i < kernel_vertices.size(); ++i)
            kernel_edges.push_back({i, (i + 1) % kernel_vertices.size()});

        // 4b - Compute triangulation of polygon
        std::vector<Point2D> convex_holes_points;
        std::vector<MeshIO::IOVertex> convex_vertices;
        std::vector<MeshIO::IOElement> convex_triangles;
        triangulatePSLC(kernel_vertices, kernel_edges, convex_holes_points, convex_vertices, convex_triangles, 1.0, "");

        // 4c - Compute centroid
        Point2D centroid;
        centroid << 0.0, 0.0;
        for (auto tri : convex_triangles) {
            Point2D tri_centroid;
            tri_centroid << 0.0, 0.0;
            for (auto vi : tri) {
                Point2D v = convex_vertices[vi];
                tri_centroid += v;
            }
            tri_centroid /= 3.0;

            double tri_area = convex_vertices[0] * p_ip1[1] - p_ip1[0] * p_i[1];

            centroid += tri_area * tri_centroid;
        }
        centroid /= convex_triangles.size();
        std::cout << "Star shaped polygon has kernel centroid at " << centroid << std::endl; */

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

        // 5 - Apply Homothepy transformation to each polygon point
        std::list<Point2D> hole_poly;
        double density = densities[e];
        int i = 0;
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

        polygons_list.push_back(outer_poly);

    }

    std::vector<MeshIO::IOVertex> out_vertices;
    std::vector<MeshIO::IOElement> out_triangles;

    triangulatePSLC(polygons_list, holes_points, out_vertices, out_triangles, 0.01, "");

    save(args.output_mesh, out_vertices, out_triangles);

    return 0;
}

/*
 * void triangulatePSLC(const std::list<std::list<Point>> &polygons,
        const std::vector<HolePoint> &holes,
        std::vector<MeshIO::IOVertex> &outVertices,
        std::vector<MeshIO::IOElement> &outTriangles,
        double area = 0.01,
        const std::string additionalFlags = "")
 */
