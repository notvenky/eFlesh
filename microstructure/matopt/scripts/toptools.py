import math
import os
import itertools
import sys
import subprocess
import re

import numpy as np
from scipy.interpolate import RegularGridInterpolator

Tolerance = 1e-10

script_directory = os.getcwd()


def jacobian_from_alpha(alpha, radian=False):
    if radian is False:
        alpha = math.radians(alpha)
    c, s = math.cos(alpha), math.sin(alpha)
    return np.array([[1, c], [0, s]])


class NDInterpolator:
    def __init__(self, function_value, parameter_values, parameter_ranges):
        self.f_values = function_value
        self.p_values = parameter_values
        self.p_ranges = parameter_ranges

        # construct grid data
        self.data = self.generate_grid_data(self.f_values, self.p_values, self.p_ranges)

        # construct map to reduced data
        self.reduced_data, self.reduced_ranges, self.index_to_reduced_index_map = self.generate_reduced_data(self.data, self.p_ranges)

        # construct interpolator structure
        self.nd_interpolator = RegularGridInterpolator(tuple(self.reduced_ranges), self.reduced_data)

    def generate_reduced_data(self, data, p_ranges):
        data_shape = data.shape
        reduced_ranges = []
        index_to_reduced_index_map = [-1] * len(data_shape)
        reduced_index = 0
        for p in range(0, len(data_shape)):
            if data_shape[p] > 1:
                reduced_ranges += [p_ranges[p]]
                index_to_reduced_index_map[p] = reduced_index
                reduced_index += 1

        reduced_data = np.squeeze(data)

        return reduced_data, reduced_ranges, index_to_reduced_index_map

    def generate_grid_data(self, function_value, p, p_ranges):
        data_shape = []
        for i in range(0, len(p_ranges)):
            data_shape += [len(p_ranges[i])]
        data_shape = tuple(data_shape)

        data = np.ndarray(shape=data_shape, dtype=float)
        data.fill(float('nan'))

        for i in range(0, len(function_value)):
            p_index = [-1] * len(p_ranges)
            for j in range(0, len(p_ranges)):
                p_index[j] = p_ranges[j].index(p[i, j])

            data[tuple(p_index)] = function_value[i]

        return data

    def is_computed(self, point):
        for j in range(0, len(self.p_ranges)):
            try:
                index = self.p_ranges[j].index(point[j])
            except:
                return False

        return True

    def point_to_reduced_point(self, point):
        reduced_point = []

        for i in range(0, len(point)):
            index = self.index_to_reduced_index_map[i]

            if index >= 0:
                reduced_point += [point[index]]

        return reduced_point

    def interpolate(self, point):
        # First, verify if point was already computed
        if self.is_computed(point):
            p_index = [-1] * len(self.p_ranges)
            for j in range(0, len(self.p_ranges)):
                try:
                    p_index[j] = self.p_ranges[j].index(point[j])
                except:
                    return False

            result = self.data[tuple(p_index)]

        else:
            reduced_point = self.point_to_reduced_point(point)
            result = self.nd_interpolator(reduced_point)[0]

        return result

    def print_experiment_info(self, params):
        print("Experiment: " + str(params))

    def check_data(self):
        index_vectors = []
        for i in range(0, len(self.p_ranges)):
            index_vectors += [range(0, len(self.p_ranges[i]))]

        experiments_indices = []
        for experiment_indices in itertools.product(*index_vectors):
            experiments_indices.append(experiment_indices)

        for e in experiments_indices:
            if math.isnan(self.data[tuple(e)]):
                not_computed = []
                for i in range(0, len(self.p_ranges)):
                    not_computed += [self.p_ranges[i][e[i]]]

                self.print_experiment_info(not_computed)


def generate_grid_data(function_value, p, p_ranges):
    data_shape = []
    for i in range(0, len(p_ranges)):
        data_shape += [len(p_ranges[i])]
    data_shape = tuple(data_shape)

    data = np.ndarray(shape=data_shape, dtype=float)
    data.fill(float('nan'))

    for i in range(0, len(function_value)):
        p_index = [-1] * len(p_ranges)
        for j in range(0, len(p_ranges)):
            p_index[j] = p_ranges[j].index(p[i][j])

        data[tuple(p_index)] = function_value[i]

    return data


def compute_step_sizes(p_ranges):
    step_list = [-1] * len(p_ranges)

    for i in range(0, len(p_ranges)):
        step_list[i] = p_ranges[i][1] - p_ranges[i][0]

    return step_list


def compute_data_ranges(p):
    p_ranges = []
    for i in range(0, p.shape[1]):
        new_range = list(set(p[:, i]))
        new_range.sort()

        p_ranges.append(new_range)

    return p_ranges

def read_data_tensor_old_format(tables):
    # parsing information in Lookup tables and adding them to the chart
    i = 0
    S = []
    pattern = []
    p = []
    for i in range(0, len(tables)):
        tablePath = tables[i]
        print(tablePath)
        tableFile = open(tablePath)
        for line in tableFile:
            fields = line.strip().split()

            pattern.append(fields[0])
            current_S = [float(fields[1]), float(fields[2]), float(fields[3]), float(fields[4]), float(fields[5]),
                         float(fields[6]), float(fields[7]), float(fields[8]), float(fields[9])]
            S.append(current_S)

            # parse parameters
            more_parameters = True
            param_index = 0
            new_parameters = []
            while more_parameters:
                m = re.search('p' + str(param_index + 1) + '-(.+?)_', fields[10])
                if m:
                    # p[param_index].append(float(m.group(1)))
                    new_parameters.append(float(m.group(1)))
                else:
                    m = re.search('p' + str(param_index + 1) + '-(.+?).msh', fields[10])
                    if m:
                        # p[param_index].append(float(m.group(1)))
                        new_parameters.append(float(m.group(1)))
                        p.append(np.array(new_parameters))
                        more_parameters = False
                    else:
                        print("Warning! parsing parameters ended unexpectedly: " + str(param_index))
                        more_parameters = False

                param_index += 1

    p = np.array(p)

    return S, p, pattern



def read_data_tensor(tables):
    # parsing information in Lookup tables and adding them to the chart
    i = 0
    S = []
    pattern = []
    p = []
    for i in range(0, len(tables)):
        tablePath = tables[i]
        #print(tablePath)
        tableFile = open(tablePath)
        for line in tableFile:
            fields = line.strip().split()

            pattern.append(fields[0])
            current_S = [float(fields[1]), float(fields[2]), float(fields[3]), float(fields[4]), float(fields[5]), float(fields[6]), float(fields[7]), float(fields[8]), float(fields[9])]
            S.append(current_S)

            # parse parameters
            new_parameters = []
            number_parameters = int(fields[10])
            for j in range(11, number_parameters + 11):
                new_parameters.append(float(fields[j]))

            p.append(np.array(new_parameters))

    p = np.array(p)

    return S, p, pattern



def read_data_orthotropic(tables, isotropic_only = False, isotropic_tolerance = 0.01, orthotropic_tolerance = 1.0):
    # parsing information in Lookup tables and adding them to the chart
    i = 0
    nuyx = []
    Ex = []
    Ey = []
    mu = []
    pattern = []
    p = []
    for i in range(0, len(tables)):
        tablePath = tables[i]
        tableFile = open(tablePath)
        for line in tableFile:
            fields = line.strip().split()

            anisotropy = float(fields[5])
            if isotropic_only and abs(anisotropy - 1.0) > isotropic_tolerance:
                continue

            nu21 = float(fields[3])
            nu12 = nu21 * float(fields[1]) / float(fields[2])

            if nu21 * nu12 > 1.0:
                print(new_parameters)
                print("Warning! Result of Poisson's ratio is not correct: " + str(nu21 * nu12))
                print("nu21: " + str(nu21))
                print("nu12: " + str(nu12))
                continue

            if abs(float(fields[1]) - float(fields[2])) / max(abs(float(fields[1])), abs(float(fields[2]))) > orthotropic_tolerance:
                print("Warning! Two different young module: " + str(float(fields[1])) + ", " + str(float(fields[2])))
                continue

            if (float(fields[12]) == 0.0):
                continue

            pattern.append(fields[0])
            Ex.append(float(fields[1]))
            Ey.append(float(fields[2]))
            nuyx.append(float(fields[3]))
            mu.append(float(fields[4]))

            # parse parameters
            new_parameters = []
            number_parameters = int(fields[6])
            for j in range(7, number_parameters + 7):
                new_parameters.append(float(fields[j]))

            p.append(np.array(new_parameters))

    p = np.array(p)
    return nuyx, Ex, Ey, mu, p, pattern


def read_data(tables, isotropic_only = False, isotropic_tolerance = 0.01, angle_range = [39.0, 91.0]):
    # parsing information in Lookup tables and adding them to the chart
    i = 0
    nu = []
    E = []
    mu = []
    pattern = []
    p = []
    angles = []
    anisotropies = []
    filtered_out = 0

    for i in range(0, len(tables)):
        tablePath = tables[i]
        tableFile = open(tablePath)
        for line in tableFile:
            fields = line.strip().split()

            if float(fields[2]) > 1.0 or float(fields[2]) < -1.0:
                print("Warning! Result of Poisson's ratio is not correct: " + str(float(fields[2])))
                continue

            # parse parameters
            new_parameters = []
            number_parameters = int(fields[5])
            all_zeros = True
            for j in range(6, number_parameters+6):
                new_parameters.append(float(fields[j]))
                if float(fields[j]) != 0.0:
                    all_zeros = False

            if len(fields) == number_parameters + 7:
                angle = float(fields[number_parameters + 6])
                if angle < angle_range[0] or angle > angle_range[1]:
                    continue

            anisotropy = float(fields[4])
            if isotropic_only and abs(anisotropy - 1.0) > isotropic_tolerance:
                # print("Warning! Filtering results that are not isotropic")
                filtered_out += 1
                continue

            if not all_zeros:
                p.append(np.array(new_parameters))
                nu.append(float(fields[2]))
                E.append(float(fields[1]))
                mu.append(float(fields[3]))
                pattern.append(fields[0])
                anisotropies.append(float(fields[4]))
                if len(fields) == number_parameters + 7:
                    angles.append(float(fields[number_parameters+6]))
                else:
                    angles.append(90.0)

    p = np.array(p)

    if filtered_out > 0:
        print("Total of {} data points were filtered out due to isotropy".format(filtered_out))

    return nu, E, mu, p, pattern, angles, anisotropies


def rotate(theta_degrees, vertices):
    theta = math.radians(theta_degrees)
    R = np.matrix([[math.cos(theta), -math.sin(theta)], [math.sin(theta), math.cos(theta)]])

    reflected_vertices = R * np.array(vertices).transpose()

    return np.asarray(reflected_vertices.transpose())


def reflect(theta_degrees, vertices):
    theta = math.radians(theta_degrees)
    Rl = np.matrix([[math.cos(2 * theta), math.sin(2 * theta)], [math.sin(2 * theta), -math.cos(2 * theta)]])

    np_vertices = np.array(vertices)
    reflected_vertices = Rl * np_vertices.transpose()

    return np.asarray(reflected_vertices.transpose())


def add_new_vertices_and_edges(reflected_vertices, reflected_edges, vertices, edges):
    new_edges_description = []
    for edge in reflected_edges:
        v1 = edge[0]
        v2 = edge[1]

        reflected_vertex1 = reflected_vertices[v1]
        reflected_vertex2 = reflected_vertices[v2]

        new_edges_description.append([reflected_vertex1, reflected_vertex2])

    add_new_edges(new_edges_description, vertices, edges)


def triangle_incenter(triangle):
    p_a = np.array(triangle[0])
    p_b = np.array(triangle[1])
    p_c = np.array(triangle[2])

    a = np.linalg.norm(p_b - p_c)
    b = np.linalg.norm(p_a - p_c)
    c = np.linalg.norm(p_a - p_b)

    incenter = (a * p_a + b * p_b + c * p_c) / (a + b + c)

    return incenter


def polygon_to_edges_descriptions(polygon):
    edges_descriptions = []

    for i in range(0, len(polygon)):
        p1 = polygon[i]
        p2 = polygon[(i + 1) % len(polygon)]

        edges_descriptions.append([p1, p2])
    return edges_descriptions


def theoretical_rectangle(E_base, nu_base, volume_fraction):
    vertices = []
    k_base = E_base / (2 * (1 - nu_base))
    mu_base = E_base / (2 * (1 + nu_base))

    rho = volume_fraction
    k_lower = 0.0
    mu_lower = 0.0

    k_upper = k_base + (1 - rho) / (rho / (k_base + mu_base) - 1 / k_base)
    mu_upper = mu_base + (1 - rho) / (rho * (k_base + 2 * mu_base) / (2 * mu_base * (k_base + mu_base)) - 1 / mu_base)

    vertices.append([k_lower, mu_lower])
    vertices.append([k_lower, mu_upper])
    vertices.append([k_upper, mu_upper])
    vertices.append([k_upper, mu_lower])

    return vertices


def theoretical_triangle(E_base, nu_base, volume_fraction):
    rectangle_vertices = theoretical_rectangle(E_base, nu_base, volume_fraction)

    upper_vertex = rectangle_vertices[2]
    k = upper_vertex[0]
    mu = upper_vertex[1]

    # top vertex
    nu_top = (k - mu) / (k + mu)
    E_top = 2 * mu * (1 + nu_top)
    top_vertex = [nu_top, E_top]

    # left vertex
    left_vertex = [-1.0, 0.0]

    # right vertex
    right_vertex = [1.0, 0.0]

    return [left_vertex, top_vertex, right_vertex]


def top_theoretical_triangle(E_base, nu_base, volume_fraction):
    rectangle_vertices = theoretical_rectangle(E_base, nu_base, volume_fraction)

    upper_vertex = rectangle_vertices[2]
    k = upper_vertex[0]
    mu = upper_vertex[1]

    # top vertex
    nu_top = (k - mu) / (k + mu)
    E_top = 2 * mu * (1 + nu_top)
    top_vertex = [nu_top, E_top]

    return top_vertex


def det_2D(a, b):
    return a[0] * b[1] - a[1] * b[0]


def edge_intersection(edge_1, edge_2):
    x_delta = (edge_1[0][0] - edge_1[1][0], edge_2[0][0] - edge_2[1][0])
    y_delta = (edge_1[0][1] - edge_1[1][1], edge_2[0][1] - edge_2[1][1])

    div = det_2D(x_delta, y_delta)
    if div == 0:
        raise Exception('lines do not intersect')

    d = (det_2D(*edge_1), det_2D(*edge_2))
    x = det_2D(d, x_delta) / div
    y = det_2D(d, y_delta) / div
    return [x, y]


def parameters_string(params):
    params_string = ""
    for i in range(0, len(params) - 1):
        params_string += str(params[i]) + ", "

    params_string += str(params[len(params) - 1])

    return params_string


def inflate_pattern(pattern, params, mesh_name, symmetry = '2d_diagonal'):
    pathname = os.path.dirname(sys.argv[0])
    inflator_executable_path = pathname + '/../../cmake-build-release/isosurface_inflator/isosurface_cli'

    params_string = parameters_string(params)
    pattern_path = pathname + "/../../data/patterns/2D/topologies/" + pattern + ".obj"

    cmd = [inflator_executable_path, symmetry, pattern_path, '--params', params_string, '-m', pathname + '/2d_meshing_opts.json',
       '--cheapPostprocessing', '--inflation_graph_radius', str(10), mesh_name]
    try:
        subprocess.call(cmd)
        return True
    except:
        print("Could not build mesh.!")
        print(cmd)
        return False


def simulate_mesh(mesh_name, output_log, material):
    homogenization_executable_path = script_directory + '/../../../MeshFEM/cmake-build-release/MeshFEM/PeriodicHomogenization_cli'

    if material == 'B9Creator':
        material = script_directory + '/../../data/materials/B9Creator.material'

    cmd = [homogenization_executable_path, mesh_name, '-m', material]

    with open(output_log, 'w') as out_log:
        try:
            result = subprocess.call(cmd, stdout=out_log)
            return bool(result == 0)
        except KeyboardInterrupt:
            raise
        except:
            print("Could not run simulation. Go to the following!")
            return False

    return True


def parse_orthotropic_logfile(logfile):
    out_log = open(logfile, 'r')

    float_pattern = re.compile(r'\-?\d+\.?\d*e?-?\d*')  # Compile a pattern to capture float values

    wire_content = out_log.readlines()
    for l, line in enumerate(wire_content):

        if line.startswith("Approximate Young moduli:"):
            floats = [float(i) for i in float_pattern.findall(line)]  # Convert strings to float
            Ex = floats[0]
            Ey = floats[1]

        elif line.startswith("Approximate shear modulus:"):
            floats = [float(i) for i in float_pattern.findall(line)]  # Convert strings to float
            mu = floats[0]

        elif line.startswith('v_yx, v_xy:'):
            floats = [float(i) for i in float_pattern.findall(line)]  # Convert strings to float
            nuyx = floats[0]

        elif line.startswith('Anisotropy:'):
            floats = [float(i) for i in float_pattern.findall(line)]  # Convert strings to float
            anisotropy = floats[0]

    return nuyx, Ex, Ey, mu, anisotropy


import enum
class VoxelCostMode(enum.Enum):
    CONSTANT = 0
    LOG = 1


def coverage_area_cost(geom_params, mat_dict, mat_dim, mat_limits, voxel_cost_mode=VoxelCostMode.CONSTANT):
    result = 0.0

    # Build grid in (m-Dimensional) material space
    m = len(mat_dict)
    mat_ranges = []
    for i in m:
        new_range = np.linspace(mat_limits[i,0], mat_limits[i,1], mat_dim[i])
        mat_ranges += [new_range]

    # For each tuple of geometric parameters, check if it falls in any hypercube in material space
    mat_shape = tuple(mat_dim)
    mat_counter = np.full(mat_shape, 0)
    for p in geom_params:
        result = mat_dict[p]

        # Find what should be index of this result in material space
        idx_list = []
        for d in m:
            d_min = mat_ranges[d][0]
            d_max = mat_ranges[d][-1]
            d_bins = mat_dim[d]

            # If falls out of range, just ignore
            if result[d] < d_min or result[d] > d_max:
                pass

            # Else, find correct index
            idx = (result[d] - d_min) / (d_max - d_min) * (d_bins - 1)

            idx_list += [idx]

        idx_tuple = tuple(idx_list)
        mat_counter[idx_tuple] += 1

    # Then, compute final area cost by using the chosen voxel cost mode
    for x in np.nditer(mat_counter):

        if voxel_cost_mode == VoxelCostMode.CONSTANT:
            if x >= 1:
                result += 1
        elif voxel_cost_mode == VoxelCostMode.LOG:
            result = math.log((math.exp(1)-1)*x + 1)
        else:
            raise RuntimeError('Voxel Cost Mode not implemented')

    return result, mat_counter
