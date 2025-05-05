#include <MeshFEM/MeshIO.hh>
#include <convert.h>
#include <json.hpp>
#include <CLI/CLI.hpp>
#include <igl/boundary_facets.h>
#include <igl/barycenter.h>
#include <iostream>
#include <string>

using namespace nlohmann;

int main(int argc, char * argv[]) {
    struct {
		std::string input_mesh;
		std::string input_bcs;
		std::string output_bcs;
	} args;

	CLI::App app{"Box2PathBCs"};
	app.add_option("input_mesh", args.input_mesh, "Input mesh");
	app.add_option("input_bcs", args.input_bcs, "Input BCs");
	app.add_option("output_bcs", args.output_bcs, "Output BCs");
	CLI11_PARSE(app, argc, argv);

	std::vector<MeshIO::IOVertex > in_vertices;
    std::vector<MeshIO::IOElement> in_elements;
    std::string in_path = args.input_mesh;

    MeshIO::MeshType type;
    type = load(in_path, in_vertices, in_elements);

    Eigen::MatrixXd V;
    Eigen::MatrixXi F;
    from_meshfem(in_vertices, in_elements, V, F);

    std::ifstream boundary_conditions_file(args.input_bcs);
    nlohmann::json input_bcs;
    boundary_conditions_file >> input_bcs;
    nlohmann::json output_bcs;
    for (auto region : input_bcs["regions"]) {
        std::string type = region["type"];

        if (type.rfind("dirichlet", 0) == std::string::npos && type.rfind("target", 0) == std::string::npos)
            continue;

        if (!(region.count("box") > 0) && !(region.count("box%") > 0))
            continue;

        Eigen::RowVector3d bbox_min = Eigen::RowVector3d::Zero();
	    Eigen::RowVector3d bbox_max = Eigen::RowVector3d::Zero();
        if (region.count("box") > 0) {
            auto box_entry = region["box"];
            std::vector<double> min_corner = box_entry["minCorner"];
            std::vector<double> max_corner = box_entry["maxCorner"];

            bbox_min << min_corner[0], min_corner[1], 0.0;
	        bbox_max << max_corner[0], max_corner[1], 0.0;
        }

        if (region.count("box%") > 0) {
            auto box_entry = region["box%"];
            std::vector<double> min_corner = box_entry["minCorner"];
            std::vector<double> max_corner = box_entry["maxCorner"];

            bbox_min << min_corner[0], min_corner[1], 0.0;
	        bbox_max << max_corner[0], max_corner[1], 0.0;

	        const Eigen::RowVector3d minV = V.colwise().minCoeff();
	        const Eigen::RowVector3d maxV = V.colwise().maxCoeff();
	        Eigen::RowVector3d origin = minV;
	        Eigen::RowVector3d extent = maxV - minV;
	        origin(2) = 0;
	        extent(2) = 1;
	        bbox_min = bbox_min.array() * extent.array() + origin.array();
	        bbox_max = bbox_max.array() * extent.array() + origin.array();
        }

        Eigen::AlignedBox2d box(bbox_min.head<2>().transpose(), bbox_max.head<2>().transpose());

        // For each boundary edge in the current mesh, if it falls within a box,
        // and is within 'tol' of a boundary edge of the parent mesh, then keep.
        Eigen::MatrixXd BV;
        Eigen::MatrixXi BE;
        igl::boundary_facets(F, BE);
        igl::barycenter(V, BE, BV);
        std::vector<bool> keep(BE.rows(), false);
        std::vector<int> kept_edges;
        for (int i = 0; i < BE.rows(); ++i) {
            Eigen::Vector2d centroid = BV.row(i).head<2>().transpose();
            if (box.contains(centroid)) {
                keep[i] = true;
                kept_edges.push_back(i);
            }
        }

        // For each kept edge, add it as a new region
        /*for (int i = 0; i < BE.rows(); ++i) {
            if (keep[i]) {
                json path_region = {
                    {"type", region["type"]},
                    {"value", region["value"]}
                };
                auto V1 = V.row(BE(i,0));
                auto V2 = V.row(BE(i,1));
                path_region["path"] = {{V1(0), V1(1)}, {V2(0), V2(1)}};

                output_bcs["regions"].push_back(path_region);
            }
        }*/


        // for each vertex index, add the edges are incidents to it
        std::vector<std::vector<int>> edges_per_vertex(V.rows(), std::vector<int>());
        for (int e=0; e<kept_edges.size(); e++) {
            int ke = kept_edges[e];
            int v0 = BE(ke,0);
            int v1 = BE(ke,1);
            edges_per_vertex[v0].push_back(e);
            edges_per_vertex[v1].push_back(e);
        }

        // find polygons from edges
        std::vector<bool> used(kept_edges.size(), false);
        while (true) {
            json path_region = {
                {"type", region["type"]},
                {"value", region["value"]}
            };

            // find unused edge that has starting point
            int e=0;
            bool found_unused = false;
            int starting_v = -1;
            int current_e = -1;
            for (; e<kept_edges.size(); e++) {
                int ke = kept_edges[e];
                if (!used[e]) {
                    int v0 = BE(ke,0);
                    int v1 = BE(ke,1);

                    if (edges_per_vertex[v0].size() == 1) {
                        starting_v = v0;
                        current_e = e;
                        found_unused = true;
                        break;
                    }

                    if (edges_per_vertex[v1].size() == 1) {
                        starting_v = v1;
                        current_e = e;
                        found_unused = true;
                        break;
                    }
                }
            }

            // all used
            if (!found_unused)
                break;

            // start with unused edge a
            int ke = kept_edges[current_e];
            int v0 = BE(ke,0);
            int v1 = BE(ke,1);
            int current_v = -1;
            if (starting_v == v0) {
                path_region["path"] = {{V(v0,0), V(v0,1)}, {V(v1,0), V(v1,1)}};
                current_v = v1;
            }
            else {
                path_region["path"] = {{V(v1,0), V(v1,1)}, {V(v0,0), V(v0,1)}};
                current_v = v0;
            }
            used[current_e] = true;

            while (true) {
                auto edges_with_v = edges_per_vertex[current_v];

                // Check if current vertex is end of polygon
                if (edges_with_v.size() == 1) {
                    break;
                }

                // find other edges using current v
                int e0 = edges_with_v[0];
                int e1 = edges_with_v[1];

                // ignore current edge
                current_e = (current_e == e0) ? e1 : e0;
                ke = kept_edges[current_e];
                int v0 = BE(ke,0);
                int v1 = BE(ke,1);

                // add next vertex
                if (current_v == v0) {
                    path_region["path"].push_back({V(v1,0), V(v1,1)});
                    current_v = v1;
                }
                else {
                    path_region["path"].push_back({V(v0,0), V(v0,1)});
                    current_v = v0;
                }

                used[current_e] = true;
            }

            output_bcs["regions"].push_back(path_region);
        }

    }

    std::ofstream writeToFile(args.output_bcs);
    writeToFile << std::setw(4) << output_bcs << std::endl;

	return 0;
}