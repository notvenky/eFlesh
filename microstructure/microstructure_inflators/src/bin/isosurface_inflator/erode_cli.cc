#include <isosurface_inflator/WireMesh.hh>
#include <isosurface_inflator/PatternSignedDistance.hh>
#include "IsosurfaceInflatorConfig.hh"

#include <CLI/CLI.hpp>
#include <json.hpp>

#include <openvdb/openvdb.h>
#include <openvdb/tools/LevelSetFilter.h>
#include <openvdb/tools/MeshToVolume.h>
#include <openvdb/tools/VolumeToMesh.h>
#include <openvdb/tools/MultiResGrid.h>
#include <openvdb/tools/Mask.h>
#include <openvdb/tools/LevelSetMeasure.h>
#include <igl/readOBJ.h>
#include <igl/writeOBJ.h>
#include <igl/bounding_box_diagonal.h>

#include <iostream>
#include <cstdio>
#include <functional>

using namespace std;
using namespace openvdb;

void read_mesh(const std::string &input, std::vector<Vec3s> &V, std::vector<Vec3I> &F)
{
    Eigen::MatrixXd v;
    Eigen::MatrixXi f;
    igl::readOBJ(input, v, f);

    V.reserve(v.rows());
    F.reserve(f.rows());

    for (int i = 0; i < v.rows(); i++)
        V.emplace_back(v(i, 0), v(i, 1), v(i, 2));
    
    for (int j = 0; j < f.rows(); j++)
        F.emplace_back(f(j, 0), f(j, 1), f(j, 2));
}

void clean_quads(std::vector<Vec3I> &Tri, std::vector<Vec4I> &Quad)
{
    for (const auto &f : Quad)
    {
        Tri.emplace_back(f(0), f(1), f(2));
        Tri.emplace_back(f(0), f(2), f(3));
    }

    Quad.clear();
}

void write_mesh(const std::string &output, const std::vector<Vec3s> &V, const std::vector<Vec3I> &Tri, const std::vector<Vec4I> &Quad)
{
    FILE * obj_file = fopen(output.c_str(),"w");
    
    for(int i = 0;i<(int)V.size();i++)
    {
        fprintf(obj_file,"v");
        for(int j = 0;j<3;++j)
        {
        fprintf(obj_file," %0.17g", V[i](j));
        }
        fprintf(obj_file,"\n");
    }

    for(int i = 0;i<(int)Tri.size();++i)
    {
        fprintf(obj_file,"f");
        for(int j = 0; j<3;++j)
        {
        // OBJ is 1-indexed
        fprintf(obj_file," %u",Tri[i](j)+1);
        }
        fprintf(obj_file,"\n");
    }

    for(int i = 0;i<(int)Quad.size();++i)
    {
        fprintf(obj_file,"f");
        for(int j = 0; j<4;++j)
        {
        // OBJ is 1-indexed
        fprintf(obj_file," %u",Quad[i](j)+1);
        }
        fprintf(obj_file,"\n");
    }

    fclose(obj_file);
}

double SignedVolumeOfTriangle(const Vec3s &p1, const Vec3s &p2, const Vec3s &p3) {
    double v321 = p3(0)*p2(1)*p1(2);
    double v231 = p2(0)*p3(1)*p1(2);
    double v312 = p3(0)*p1(1)*p2(2);
    double v132 = p1(0)*p3(1)*p2(2);
    double v213 = p2(0)*p1(1)*p3(2);
    double v123 = p1(0)*p2(1)*p3(2);
    return (1 / 6.0)*(-v321 + v231 + v312 - v132 - v213 + v123);
}

double compute_surface_mesh_volume(const std::vector<Vec3s> &V, const std::vector<Vec3I> &Tri)
{
    Vec3s center(0, 0, 0);
    for (const auto &v : V)
        center += v;
    center /= V.size();
    
    double vol = 0;
    for (const auto &f : Tri)
    {
        vol += SignedVolumeOfTriangle(V[f(0)] - center, V[f(1)] - center, V[f(2)] - center);
    }

    return vol;
}

double volume_after_offset(const FloatGrid::Ptr &grid, const double radius, std::vector<Vec3s> &Ve, std::vector<Vec3I> &Tri)
{
    // erode
    auto tmp_grid = grid->deepCopyGrid();
    std::unique_ptr<tools::LevelSetFilter<FloatGrid>> filter = std::make_unique<tools::LevelSetFilter<FloatGrid>>(*gridPtrCast<FloatGrid>(tmp_grid));
    filter->offset(radius);

    // ls2mesh
    Ve.clear();
    Tri.clear();
    std::vector<Vec4I> Quad;
    tools::volumeToMesh(*gridPtrCast<FloatGrid>(tmp_grid), Ve, Tri, Quad, 0, 0, true);
    clean_quads(Tri, Quad);

    double vol = abs(compute_surface_mesh_volume(Ve, Tri));
    return vol;
}

double sdf_volume(const FloatGrid::Ptr &grid)
{
    if (grid->empty())
        return 0.;
    
    tools::LevelSetMeasure<FloatGrid> measure(*gridPtrCast<FloatGrid>(grid));
    return abs(measure.volume());
}

double volume_after_offset(const FloatGrid::Ptr &grid, const double radius)
{
    // erode
    auto tmp_grid = grid->deepCopyGrid();
    if (radius != 0)
    {
        std::unique_ptr<tools::LevelSetFilter<FloatGrid>> filter = std::make_unique<tools::LevelSetFilter<FloatGrid>>(*gridPtrCast<FloatGrid>(tmp_grid));
        filter->offset(radius);
    }

    return sdf_volume(gridPtrCast<FloatGrid>(tmp_grid));
}

void execute2(const string &input, const string &output, const double offset_size)
{
    std::vector<Vec3s> V;
    std::vector<Vec3I> F;
    read_mesh(input, V, F);

    double half_diag;
    {
        Eigen::MatrixXd V_mat(V.size(), 3);
        for (int i = 0; i < V.size(); i++)
            for (int d = 0; d < 3; d++)
                V_mat(i, d) = V[i](d);
        half_diag = igl::bounding_box_diagonal(V_mat) / 2;
    }

    double voxel = 3e-2 * half_diag;
    int halfWidth = std::max(5, (int)(offset_size / voxel / 2));
    math::Transform::Ptr xform(nullptr);
    xform = math::Transform::createLinearTransform(voxel);
    FloatGrid::Ptr grid = tools::meshToLevelSet<FloatGrid>(*xform, V, F, halfWidth);

    volume_after_offset(grid, offset_size, V, F);
    write_mesh(output, V, F, std::vector<Vec4I>());
}

void execute1(const string &input, const string &output, const double volume_ratio, bool keep_original, double min_width, double tol) 
{
    std::vector<Vec3s> V;
    std::vector<Vec3I> F;
    read_mesh(input, V, F);

    double half_diag;
    {
        Eigen::MatrixXd V_mat(V.size(), 3);
        for (int i = 0; i < V.size(); i++)
            for (int d = 0; d < 3; d++)
                V_mat(i, d) = V[i](d);
        half_diag = igl::bounding_box_diagonal(V_mat) / 2;
    }

    const double initial_vol = abs(compute_surface_mesh_volume(V, F));
    const double hole_vol = 1 - volume_ratio;

    std::cout << "target volume ratio: " << hole_vol << "\n";
    std::cout << "half diag: " << half_diag << "\n";

    // eroded mesh
    std::vector<Vec3s> Ve;
    std::vector<Vec3I> Tri;
    double voxel = 3e-2 * half_diag;
    int halfWidth = 5;// std::max(5, (int)(min_width / voxel));
    double current_vol, current;
    while (voxel > 1e-3 * half_diag)
    {
        math::Transform::Ptr xform(nullptr);
        xform = math::Transform::createLinearTransform(voxel);
        FloatGrid::Ptr grid = tools::meshToLevelSet<FloatGrid>(*xform, V, F, halfWidth);
        const double initial_vol = sdf_volume(grid);

        double upper = 0.2 * half_diag;
        double upper_vol = volume_after_offset(grid, upper, Ve, Tri) / initial_vol;
        double lower = 0;
        double lower_vol = volume_after_offset(grid, lower, Ve, Tri) / initial_vol;

        while (upper_vol > hole_vol)
        {
            upper *= 1.5;
            upper_vol = volume_after_offset(grid, upper, Ve, Tri) / initial_vol;

            cout << "current radius interval: " << upper << "\n";
            cout << "current volume interval: " << upper_vol << "\n";
        }

        // if (lower_vol > 1 || upper_vol < 0)
        // {
        //     voxel /= 2;
        //     halfWidth *= 2;
        //     std::cout << "Bounds doesn't satisfy volume requirement, refine voxel to " << voxel / half_diag << "\n";
        //     continue;
        // }

        assert(upper_vol <= hole_vol && lower_vol >= hole_vol);

        current = (lower + upper) / 2;
        current_vol = volume_after_offset(grid, current, Ve, Tri) / initial_vol;

        while (abs(current_vol - hole_vol) > tol && abs(upper - lower) > 1e-6 * half_diag)
        {
            if (current_vol > hole_vol)
            {
                lower = current;
                lower_vol = current_vol;
            }
            else
            {
                upper = current;
                upper_vol = current_vol;
            }

            current = (lower + upper) / 2;
            current_vol = volume_after_offset(grid, current, Ve, Tri) / initial_vol;

            cout << "current radius interval: " << lower << " " << current << " " << upper << "\n";
            cout << "current volume interval: " << lower_vol << " " << current_vol << " " << upper_vol << "\n";

            if (current_vol > lower_vol || current_vol < upper_vol)
                break;
        }

        if (abs(current_vol - hole_vol) <= tol)
        {
            // current_vol = volume_after_offset(grid, current, Ve, Tri) / initial_vol;
            break;
        }
        else
        {
            voxel /= 2;
            halfWidth *= 1.5;
            std::cout << current_vol << " doesn't satisfy volume requirement, refine voxel to " << voxel / half_diag << "\n";
        }
    }

    if (abs(current_vol - hole_vol) > tol)
        throw std::runtime_error("Doesn't satisfy volume requirement");

    // merge with original mesh
    // if (compute_surface_mesh_volume(V, F) * compute_surface_mesh_volume(Ve, Tri) >= 0)
    // {
    //     std::cout << "Wrong orientation of eroded mesh!\n";
    // }
    
    if (keep_original)
    {
        for (auto &f : F)
            for (int i = 0; i < 3; i++)
                f(i) += Ve.size();
        Ve.insert(Ve.end(), V.begin(), V.end());
        Tri.insert(Tri.end(), F.begin(), F.end());
    }

    // save polygon mesh
    write_mesh(output, Ve, Tri, std::vector<Vec4I>());
    
    // igl::writeOBJ(output, V, F);
}

// void usage(int status, const po::options_description &visible_opts) {
//     cerr << "Usage: ./erode_cli input_mesh output_mesh volume_ratio" << endl;
//     cerr << "eg: ./erode_cli mesh.obj out.obj 0.4" << endl;
//     cout << visible_opts << endl;
//     exit(status);
// }

// po::variables_map parseCmdLine(int argc, char *argv[]) {
//     po::options_description hidden_opts("Hidden Arguments");
//     hidden_opts.add_options()
//         ("input" ,  po::value<string>(), "input mesh file")
//         ("output",  po::value<string>(), "output mesh file")
//         ("ratio" ,  po::value<double>(), "solid volume")
//         ;
//     po::positional_options_description p;
//     p.add("input", 1);
//     p.add("output", 1);
//     p.add("ratio", 1);

//     po::options_description visible_opts;
//     visible_opts.add_options()
//         ("enforce-offset", "enforce specified offset size instead of volume ratio")
//         ("keep-input-surface", "include the input surface in the output mesh")
//         ("min-width", po::value<double>()->default_value(0), "Minimum width of the structure with holes")
//         ("volume-tolerance", po::value<double>()->default_value(1e-2), "")
//         ("help,h", "Produce this help message")
//         ;

//     po::options_description cli_opts;
//     cli_opts.add(visible_opts).add(hidden_opts);

//     po::variables_map vm;
//     try {
//         po::store(po::command_line_parser(argc, argv).
//                   options(cli_opts).positional(p).run(), vm);
//         po::notify(vm);
//     }
//     catch (std::exception &e) {
//         cerr << "Error: " << e.what() << endl << endl;
//         usage(1, visible_opts);
//     }

//     bool fail = false;
//     {
//         auto ratio = vm["ratio"].as<double>();
//         if (ratio <= 0 || ratio > 1) {
//             cerr << "Error: invalid ratio " << ratio << endl;
//             fail = true;
//         }
//     }

//     if (fail)
//         usage(1, visible_opts);

//     return vm;
// }

int main(int argc, char * argv[]) {
    // Default arguments
    struct {
        std::string input;
        std::string output;
        double ratio;
        bool enforce_offset = false;
        bool keep_input_surface = false;
        double volume_tolerance = 1e-2;
        double min_width = 0;
    } args;

    // Parse arguments
    CLI::App app{"erode_cli"};
    app.add_option("input", args.input, "")->required();
    app.add_option("output", args.output, "")->required();
    app.add_option("ratio", args.ratio, "")->required();
    app.add_flag("--enforce-offset", args.enforce_offset, "");
    app.add_flag("--keep-input-surface", args.keep_input_surface, "");
    app.add_option("--min-width", args.min_width, "");
    app.add_option("--volume-tolerance", args.volume_tolerance, "");

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError &e) {
        return app.exit(e);
    }

    if (args.enforce_offset)
        execute2(args.input, args.output, args.ratio);
    else
        execute1(args.input, args.output, args.ratio, args.keep_input_surface, args.min_width, args.volume_tolerance);
}
