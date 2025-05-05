#!/usr/bin/env python3
import argparse
import sys
import os

import numpy as np
import scipy

import toptools

import zpline
import lls

import copy

# T: (a, b) -> (-1, 1)
def general_to_default_interval(x, a, b):
    alpha = 2.0 / (b - a)
    beta = 1.0 - 2 * b / (b - a)

    y = alpha * x + beta

    return y


def generate_linear_interpolation(nu, E, angles, p):
    data = np.array([nu, E, angles])

    return scipy.interpolate.LinearNDInterpolator(np.transpose(data), p, rescale=True)


def have_neighbors(target, points, dx=1.0, dy=1.0, dz=1.0):
    result = False

    for p in points:
        if abs(p[0] - target[0]) <= dx and abs(p[1] - target[1]) <= dy and abs(p[2] - target[2]) <= dz:
            result = True
            break

    return result


def generate_regularization_points():
    start = [-0.8, -0.01, 44.0]
    stop =  [ 0.9,  0.34, 136.0]

    regularization_points = []
    for reg_x in np.linspace(start[0], stop[0], 40):
        for reg_y in np.linspace(start[1], stop[1], 60):
            for reg_z in np.linspace(start[2], stop[2], 80):
                reg_point = [reg_x, reg_y, reg_z]

                regularization_points.append(reg_point)

    return regularization_points


def generate_lls(nu, E, angles, p, dim = None):

    if dim is None:
        lls_approx = lls.LLSInterpolation(nu, E, angles, p)
    else:
        lls_approx = lls.LLSInterpolation(nu, E, angles, p, d1=dim[0], d2=dim[1], d3=dim[2])

    return lls_approx


def generate_lls_with_coefficients(d1, d2, d3, coeffs):
    lls_approx = lls.LLSInterpolation(d1=d1, d2=d2, d3=d3, coeffs=coeffs)

    return lls_approx


def generate_splines(nu, E, angles, p, regularization_points, dim = None, alpha = 0.5, larger_than_90=False):

    if dim is None or len(dim) != 3:
        kx = 12
        ky = 12
        kz = 12
    else:
        kx = dim[0] - 4
        ky = dim[1] - 4
        kz = dim[2] - 4

    start = [-0.8, -0.01, 44.0]
    if larger_than_90:
        stop = [0.9, 0.34, 136.0]
    else:
        stop = [0.9, 0.34, 91.0]

    resolution = [(stop[0] - start[0]) / (kx + 1), (stop[1] - start[1]) / (ky + 1), (stop[2] - start[2]) / (kz + 1)]
    width = [kx + 1, ky + 1, kz + 1]

    print("Res: " + str(resolution))
    print("Width: " + str(width))


    #print(alpha)
    cbs = zpline.CubicTriSpline(start=start, resolution=resolution, width=width, alpha=alpha)

    x = []
    y = []
    for i in range(len(nu)):
        x1_ij = nu[i]
        x2_ij = E[i]
        x3_ij = angles[i]

        x.append([x1_ij, x2_ij, x3_ij])
        y.append([p[i]])


    print("Number of regularization points: " + str(len(regularization_points)))

    cbs.interpolate(x, np.array(y), regularization_points=regularization_points)

    return cbs


def generate_splines_from_other(original_cbs, p):
    cbs = copy.copy(original_cbs)

    y = []
    for i in range(len(p)):
        y.append([p[i]])

    cbs.interpolate_same_base(np.array(y))

    return cbs


def generate_splines_with_coefficients(coeffs, larger_than_90=False):
    kx = coeffs.shape[0] - 4
    ky = coeffs.shape[1] - 4
    kz = coeffs.shape[2] - 4

    start = [-0.8, -0.01, 44.0]
    if larger_than_90:
        stop = [0.9, 0.34, 136.0]
    else:
        stop = [0.9, 0.34, 91.0]
    alpha = 0.5

    resolution = [(stop[0] - start[0]) / (kx + 1), (stop[1] - start[1]) / (ky + 1), (stop[2] - start[2]) / (kz + 1)]
    width = [kx + 1, ky + 1, kz + 1]

    print("Res: " + str(resolution))
    print("Width: " + str(width))

    cbs = zpline.CubicTriSpline(start=start, resolution=resolution, width=width, alpha=alpha, coef=coeffs)

    return cbs


class Material2Geometry:

    def __init__(self, nu = [], E = [], angles = [], p = [], method="lls", in_path = None, dim = None, regularization_coefficient = 0.5, base_nu = 0.0, base_E = 1.0, larger_than_90 = False):
        self.base_nu = base_nu
        self.base_E = base_E

        if in_path is not None:
            self.start_with_file(in_path, larger_than_90)
            return

        self.method = method

        if method == "piecewise_linear":
            self.nu = nu
            self.E = np.array(E) * 10.0
            self.angles = np.array(angles) / 100.0
            self.parameter_values = p

            self.p1_map = generate_linear_interpolation(nu, self.E, self.angles, p[:, 0])
            self.p2_map = generate_linear_interpolation(nu, self.E, self.angles, p[:, 1])
            self.p3_map = generate_linear_interpolation(nu, self.E, self.angles, p[:, 2])
            self.p4_map = generate_linear_interpolation(nu, self.E, self.angles, p[:, 3])
            self.p5_map = generate_linear_interpolation(nu, self.E, self.angles, p[:, 4])
            self.p6_map = generate_linear_interpolation(nu, self.E, self.angles, p[:, 5])
            self.p7_map = generate_linear_interpolation(nu, self.E, self.angles, p[:, 6])
            self.p8_map = generate_linear_interpolation(nu, self.E, self.angles, p[:, 7])

        elif method == "lls":
            self.nu = nu
            self.E = np.array(E) * 10
            self.angles = np.array(angles) / 100.0
            self.parameter_values = p

            self.p1_map = generate_lls(self.nu, self.E, self.angles, p[:, 0], dim=dim)
            self.p2_map = generate_lls(self.nu, self.E, self.angles, p[:, 1], dim=dim)
            self.p3_map = generate_lls(self.nu, self.E, self.angles, p[:, 2], dim=dim)
            self.p4_map = generate_lls(self.nu, self.E, self.angles, p[:, 3], dim=dim)
            self.p5_map = generate_lls(self.nu, self.E, self.angles, p[:, 4], dim=dim)
            self.p6_map = generate_lls(self.nu, self.E, self.angles, p[:, 5], dim=dim)
            self.p7_map = generate_lls(self.nu, self.E, self.angles, p[:, 6], dim=dim)
            self.p8_map = generate_lls(self.nu, self.E, self.angles, p[:, 7], dim=dim)

        elif method == "splines":
            self.nu = nu
            self.E = E
            self.angles = angles
            self.parameter_values = p

            regularization_points = generate_regularization_points()

            print("p1")
            self.p1_map = generate_splines(self.nu, self.E, self.angles, p[:, 0], regularization_points, dim=dim, alpha=regularization_coefficient, larger_than_90=larger_than_90)
            print("p2")
            self.p2_map = generate_splines_from_other(self.p1_map, p[:, 1], larger_than_90=larger_than_90)
            print("p3")
            self.p3_map = generate_splines_from_other(self.p1_map, p[:, 2], larger_than_90=larger_than_90)
            print("p4")
            self.p4_map = generate_splines_from_other(self.p1_map, p[:, 3], larger_than_90=larger_than_90)
            print("p5")
            self.p5_map = generate_splines_from_other(self.p1_map, p[:, 4], larger_than_90=larger_than_90)
            print("p6")
            self.p6_map = generate_splines_from_other(self.p1_map, p[:, 5], larger_than_90=larger_than_90)
            print("p7")
            self.p7_map = generate_splines_from_other(self.p1_map, p[:, 6], larger_than_90=larger_than_90)
            print("p8")
            self.p8_map = generate_splines_from_other(self.p1_map, p[:, 7], larger_than_90=larger_than_90)

        else:
            raise Exception


    def start_with_file(self, input_path, larger_than_90=False):
        input = open(input_path, 'r')

        for idx, line in enumerate(input):
            if idx == 0:
                self.method = line.strip()

            else:
                if self.method == "lls":
                    fields = line.strip().split()

                    parameter = int(fields[0])
                    shape = (int(fields[1]), int(fields[2]), int(fields[3]))

                    coeffs = []
                    for i in range(4, 4 + shape[0] * shape[1] * shape[2]):
                        coeffs.append(float(fields[i]))

                    if parameter == 1:
                        self.p1_map = generate_lls_with_coefficients(shape[0], shape[1], shape[2], coeffs)
                    elif parameter == 2:
                        self.p2_map = generate_lls_with_coefficients(shape[0], shape[1], shape[2], coeffs)
                    elif parameter == 3:
                        self.p3_map = generate_lls_with_coefficients(shape[0], shape[1], shape[2], coeffs)
                    elif parameter == 4:
                        self.p4_map = generate_lls_with_coefficients(shape[0], shape[1], shape[2], coeffs)
                    elif parameter == 5:
                        self.p5_map = generate_lls_with_coefficients(shape[0], shape[1], shape[2], coeffs)
                    elif parameter == 6:
                        self.p6_map = generate_lls_with_coefficients(shape[0], shape[1], shape[2], coeffs)
                    elif parameter == 7:
                        self.p7_map = generate_lls_with_coefficients(shape[0], shape[1], shape[2], coeffs)
                    elif parameter == 8:
                        self.p8_map = generate_lls_with_coefficients(shape[0], shape[1], shape[2], coeffs)

                elif self.method == "splines":
                    fields = line.strip().split()

                    parameter = int(fields[0])
                    shape = (int(fields[1]), int(fields[2]), int(fields[3]))

                    coeffs = []
                    for i in range(4, 4 + shape[0] * shape[1] * shape[2]):
                        coeffs.append(float(fields[i]))

                    coeffs_matrix = np.array(coeffs).reshape(shape)

                    if parameter == 1:
                        self.p1_map = generate_splines_with_coefficients(coeffs_matrix, larger_than_90)
                    elif parameter == 2:
                        self.p2_map = generate_splines_with_coefficients(coeffs_matrix, larger_than_90)
                    elif parameter == 3:
                        self.p3_map = generate_splines_with_coefficients(coeffs_matrix, larger_than_90)
                    elif parameter == 4:
                        self.p4_map = generate_splines_with_coefficients(coeffs_matrix, larger_than_90)
                    elif parameter == 5:
                        self.p5_map = generate_splines_with_coefficients(coeffs_matrix, larger_than_90)
                    elif parameter == 6:
                        self.p6_map = generate_splines_with_coefficients(coeffs_matrix, larger_than_90)
                    elif parameter == 7:
                        self.p7_map = generate_splines_with_coefficients(coeffs_matrix, larger_than_90)
                    elif parameter == 8:
                        self.p8_map = generate_splines_with_coefficients(coeffs_matrix, larger_than_90)


    def to_default_base_material(self, nu, E):
        default_nu = nu - (E/self.base_E) * self.base_nu
        default_E  = E/self.base_E

        return default_nu, default_E

    def evaluate(self, nu, E, angle):

        # change of variables
        print(nu, E)
        nu, E = self.to_default_base_material(nu, E)
        print(nu, E)
        print("\n")

        if self.method == "splines":
            pass
        else:
            nu = 1.0 * nu
            E = 10.0 * E
            angle = angle / 100.0

        p1 = float(self.p1_map(nu, E, angle))
        p2 = float(self.p2_map(nu, E, angle))
        p3 = float(self.p3_map(nu, E, angle))
        p4 = float(self.p4_map(nu, E, angle))
        p5 = float(self.p5_map(nu, E, angle))
        p6 = float(self.p6_map(nu, E, angle))
        p7 = float(self.p7_map(nu, E, angle))
        p8 = float(self.p8_map(nu, E, angle))

        if p5 <= 0.001:
            p5 = 0.001
        if p6 <= 0.001:
            p6 = 0.001
        if p7 <= 0.001:
            p7 = 0.001
        if p8 <= 0.001:
            p8 = 0.001

        p9  = 0.01
        p10 = 0.01
        p11 = 0.01
        p12 = 0.01

        return [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12]


    def save(self, output_path):

        out = open(output_path, 'w')

        out.write(self.method + "\n")

        for p in range(1, 9):
            out.write("{}\t".format(p))

            if self.method == "lls":
                if p == 1:
                    coeffs = self.p1_map.coeffs
                if p == 2:
                    coeffs = self.p2_map.coeffs
                if p == 3:
                    coeffs = self.p3_map.coeffs
                if p == 4:
                    coeffs = self.p4_map.coeffs
                if p == 5:
                    coeffs = self.p5_map.coeffs
                if p == 6:
                    coeffs = self.p6_map.coeffs
                if p == 7:
                    coeffs = self.p7_map.coeffs
                if p == 8:
                    coeffs = self.p8_map.coeffs

                out.write("{}\t{}\t{}\t".format(self.p1_map.d1, self.p1_map.d2, self.p1_map.d3))

                coeffs_list = list(np.array(coeffs).reshape(-1, ))
                coeffs_string = ""
                for i in range(0, len(coeffs_list)):
                    coeffs_string += "\t" + '{:.16f}'.format(coeffs_list[i])

                out.write(coeffs_string)

            elif self.method == "splines":
                if p == 1:
                    coeffs = self.p1_map.coef
                if p == 2:
                    coeffs = self.p2_map.coef
                if p == 3:
                    coeffs = self.p3_map.coef
                if p == 4:
                    coeffs = self.p4_map.coef
                if p == 5:
                    coeffs = self.p5_map.coef
                if p == 6:
                    coeffs = self.p6_map.coef
                if p == 7:
                    coeffs = self.p7_map.coef
                if p == 8:
                    coeffs = self.p8_map.coef

                shape = coeffs.shape
                out.write("{}\t{}\t{}\t".format(shape[0], shape[1], shape[2]))

                coeffs_list = list(np.array(coeffs).reshape(-1,))
                coeffs_string = ""
                for i in range(0, len(coeffs_list)):
                    coeffs_string += "\t" + '{:.16f}'.format(coeffs_list[i])

                out.write(coeffs_string)

            out.write("\n")

        out.close()


