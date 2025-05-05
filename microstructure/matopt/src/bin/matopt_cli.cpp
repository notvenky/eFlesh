#include <material_optimization_ipopt.h>
#include <material_optimization_gd.h>
#include <material_optimization_lbfgs.h>
#include <material_optimization.h>
#include <logger.h>
#include <convert.h>
#include <MeshFEM/MeshIO.hh>
#include <MeshFEM/Fields.hh>
#include <MeshFEM/MSHFieldParser.hh>
#include <MeshFEM/MSHFieldWriter.hh>
#include <MeshFEM/MaterialOptimization.hh>

#include <CLI/CLI.hpp>
#include <iostream>
#include <string>

typedef ScalarField<Real> SField;

std::vector<double> readRegularizationMultipliers(const std::string &cpath) {
    std::vector<double> result;
    std::ifstream inFile(cpath);
    if (!inFile.is_open()) throw std::runtime_error("Couldn't open regularization file");

    nlohmann::json jsonValues;
    inFile >> jsonValues;

    for (auto value : jsonValues) {
        result.push_back(value);
    }

    return result;
}

std::vector<int> readCellIndices(const std::string &cpath) {
    std::vector<int> result;
    std::ifstream inFile(cpath);
    if (!inFile.is_open()) throw std::runtime_error("Couldn't open cell index file");

    nlohmann::json jsonValues;
    inFile >> jsonValues;

    for (auto value : jsonValues) {
        result.push_back(value);
    }

    return result;
}

std::vector<double> readInitialGuess(const std::string &cpath) {
    std::vector<double> result;
    std::ifstream inFile(cpath);
    if (!inFile.is_open()) throw std::runtime_error("Couldn't open initial guess file");

    nlohmann::json jsonValues;
    inFile >> jsonValues;

    for (auto value : jsonValues["x"]) {
        result.push_back(value);
    }

    return result;
}

int main(int argc, char * argv[]) {
    struct {
		std::string input_mesh;
		std::string boundary_conditions;
		std::string material_bounds;
		std::string log_filename = "log.txt";
		std::string intermediate_results = "";
		std::string output_msh;
		bool is_quiet = false;
		int log_level = 0; // trace
		double regularization_weight = 0.0;
		double anisotropy_weight = 0.0;
		std::string material_type = "isotropic";
		std::string solver = "ceres";
		std::string shear_coeffs_path = "";
		double orthotropic_angle = 0.0;
		std::string regularization_multipliers = "";
		int iterations = 5;
		int mult_mode = MaterialOptimization::avg;
		std::string cell_index = "";
		std::string initial_guess = "";
		double elasticity_weight = 1.0;
		double bounds_weight = 1e4;
		double smoothing_weight = 0.0;
		std::string solution_info = "";
	} args;

	CLI::App app{"MatOpt"};

	app.add_option("input_mesh", args.input_mesh, "Input triangle mesh");
	app.add_option("boundary_conditions", args.boundary_conditions, "Input boundary conditions");
	app.add_option("material_bounds", args.material_bounds, "Input material bounds");
	app.add_option("output_msh", args.output_msh, "Output triangle mesh");
	app.add_option("--log", args.log_filename, "Log info to given file");
	app.add_option("--intermediate_results", args.intermediate_results, "Log results at each optimization iteration");
	app.add_flag("-q,--is-quiet", args.is_quiet, "Mute console output. (optional)");
	app.add_option("--level", args.log_level, "Log level (0 = most verbose, 6 = off)");
	app.add_option("--iterations", args.iterations, "number of iterations");
	app.add_option("--regularization_weight", args.regularization_weight, "Regularization weight");
	app.add_option("--anisotropy_weight", args.anisotropy_weight, "Anisotropy weight");
	app.add_option("--material_type", args.material_type, "Material type");
	app.add_option("--solver", args.solver, "Solver");
	app.add_option("--ENu2Shear", args.shear_coeffs_path, "File containing map from Youngs modulus and Poissons ratio to shear modulus");
	app.add_option("--orthotropic_angle", args.orthotropic_angle, "Angle from which regular grid is tilted");
	app.add_option("--regularization_multipliers", args.regularization_multipliers, "Input reg multipliers");
	app.add_option("--regularization_mult_mode", args.mult_mode, "regularization mode: enum/MultiMode in {min=0, avg=1, max=2}");
	app.add_option("--cell_index", args.cell_index, "Input cell indices");
	app.add_option("--initial_guess", args.initial_guess, "Gives an initial solution as guess");
	app.add_option("--elasticity_weight", args.elasticity_weight, "Elasticity weight");
	app.add_option("--bounds_weight", args.bounds_weight, "Bounds weight");
	app.add_option("--smoothing_weight", args.smoothing_weight, "Smoothing weight");
	app.add_option("--solution_info", args.solution_info, "Solution info");

	CLI11_PARSE(app, argc, argv);

	Logger::init(!args.is_quiet, args.log_filename);
	args.log_level = std::max(0, std::min(6, args.log_level));
	spdlog::set_level(static_cast<spdlog::level::level_enum>(args.log_level));
	spdlog::flush_every(std::chrono::seconds(3));

    std::vector<MeshIO::IOVertex > in_vertices;
    std::vector<MeshIO::IOElement> in_elements;
    std::string in_path = args.input_mesh;

    MeshIO::MeshType type;
    type = load(in_path, in_vertices, in_elements);

    //MSHFieldParser<3> fields(args.input_msh);
    //DomainType dtype;
    //SField sf = fields.scalarField("boundary", DomainType::ANY, dtype);

    Eigen::MatrixXd V;
    Eigen::MatrixXi F;
    from_meshfem(in_vertices, in_elements, V, F);

    Eigen::VectorXi cell_index;
    cell_index.resize(in_elements.size());
    if (args.cell_index != "") {
        auto cell_index_vector = readCellIndices(args.cell_index);
        for (int i = 0; i < in_elements.size(); i++) {
            cell_index[i] = cell_index_vector[i];
        }
    }
    else {
        for (int i = 0; i < in_elements.size(); i++) {
            cell_index[i] = i;
        }
    }

    std::vector<double> initial_guess;
    if (args.initial_guess != "") {
        initial_guess = readInitialGuess(args.initial_guess);
    }

    std::ifstream boundary_conditions_file(args.boundary_conditions);
    nlohmann::json boundary_conditions;
    boundary_conditions_file >> boundary_conditions;

    std::ifstream material_bounds_file(args.material_bounds);
    nlohmann::json material_bounds;
    material_bounds_file >> material_bounds;

    std::ifstream shear_file(args.shear_coeffs_path);
    std::string word;
    int i = 0;
    int shear_degree = 0;
    std::vector<double> shear_coeffs;
    while (shear_file >> word)
    {
        if (i == 0) {
            std::cout << "Using method " + word + "for E,nu to shear modulus" << std::endl;
        }
        else if (i == 1) {
            shear_degree = stoi(word);
            std::cout << "Using polynomial of degree " << shear_degree << std::endl;
        }
        else {
            shear_coeffs.push_back(std::stod(word));
        }

        i++;
    }
    if (shear_coeffs.size() > 0) {
        std::cout << "Coefficients are: ";
        for (auto c : shear_coeffs) {
            std::cout << c << "\t";
        }
        std::cout << "\n" << std::endl;
    }

    MaterialOptimization::RegularizationMultMode mult_mode = static_cast<MaterialOptimization::RegularizationMultMode>(args.mult_mode);
    std::cout << "Regularization Mult Mode: " << mult_mode << std::endl;

    std::vector<double> regularization_multipliers(in_elements.size(), 1.0);
    if (args.regularization_multipliers != "") {
        regularization_multipliers = readRegularizationMultipliers(args.regularization_multipliers);
    }

    Eigen::MatrixXd result;
    std::cout << "Starting optimization" << std::endl;
    if (args.solver == "ceres") {
        material_optimization(V, F, cell_index, boundary_conditions, material_bounds, result, regularization_multipliers,
                              1, args.iterations, 1, args.regularization_weight, args.anisotropy_weight, false, args.intermediate_results,
                              args.material_type, shear_degree, shear_coeffs, args.orthotropic_angle, mult_mode);
    }
    else if (args.solver == "ipopt")  {
        material_optimization_ipopt(V, F, cell_index, boundary_conditions, material_bounds, result, regularization_multipliers,
                                    1, args.iterations, 1, args.regularization_weight, args.anisotropy_weight, false, {},
                                    args.intermediate_results, args.material_type, shear_degree, shear_coeffs,
                                    args.orthotropic_angle, mult_mode);
    }
    else if (args.solver == "lbfgs")  {
        material_optimization_lbfgs(V, F, cell_index, boundary_conditions, material_bounds, result, regularization_multipliers,
                                    2, args.iterations, 1, args.regularization_weight, args.anisotropy_weight, false,
                                    args.intermediate_results, args.material_type, shear_degree, shear_coeffs,
                                    args.orthotropic_angle, mult_mode, initial_guess, args.elasticity_weight,
                                    args.bounds_weight, args.smoothing_weight, args.solution_info);
    }
    else {
        material_optimization_gd(V, F, cell_index, boundary_conditions, material_bounds, result, regularization_multipliers,
                                    2, args.iterations, 1, args.regularization_weight, args.anisotropy_weight, false,
                                    args.intermediate_results, args.material_type, shear_degree, shear_coeffs,
                                    args.orthotropic_angle, mult_mode, initial_guess, args.elasticity_weight,
                                    args.bounds_weight, args.smoothing_weight);
    }

    std::cout << "Ending optimization" << std::endl;

    if (args.material_type == "isotropic" || args.material_type == "constrained_cubic") {
        SField young(result.col(0));
        SField poisson(result.col(1));

        MSHFieldWriter writer(args.output_msh, in_vertices, in_elements);
        writer.addField("E", young);
        writer.addField("nu", poisson);
    }
    else if (args.material_type == "orthotropic"){
        SField Ex(result.col(0));
        SField Ey(result.col(1));
        SField nuyx(result.col(2));
        SField mu(result.col(3));

        MSHFieldWriter writer(args.output_msh, in_vertices, in_elements);
        writer.addField("E_x", Ex);
        writer.addField("E_y", Ey);
        writer.addField("nu_yx", nuyx);
        writer.addField("mu", mu);
    }
    else if (args.material_type == "cubic"){
        SField young(result.col(0));
        SField poisson(result.col(1));
        SField shear(result.col(2));

        MSHFieldWriter writer(args.output_msh, in_vertices, in_elements);
        writer.addField("E", young);
        writer.addField("nu", poisson);
        writer.addField("mu", shear);
    }

    return 0;
}