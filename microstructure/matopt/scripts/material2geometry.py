#!/usr/bin/env python3
import argparse

import numpy as np
import scipy

import zpline_2D
import lls_2D

import copy

# T: (a, b) -> (-1, 1)
def general_to_default_interval(x, a, b):
    alpha = 2.0 / (b - a)
    beta = 1.0 - 2 * b / (b - a)

    y = alpha * x + beta

    return y


def generate_regularization_points():
    start = [-0.1,  0.0]
    stop =  [ 0.4,  1.0]

    regularization_points = []
    for reg_x in np.linspace(start[0], stop[0], 40):
        for reg_y in np.linspace(start[1], stop[1], 60):
            reg_point = [reg_x, reg_y]

            regularization_points.append(reg_point)

    return regularization_points


def generate_lls(nu, E, p, dim = None):

    if dim is None:
        lls_approx = lls_2D.LLSInterpolation(nu, E, p)
    else:
        lls_approx = lls_2D.LLSInterpolation(nu, E, p, dd=dim[0])

    return lls_approx


def generate_lls_with_coefficients(d, coeffs):
    lls_approx = lls_2D.LLSInterpolation(d=d, coeffs=coeffs)

    return lls_approx


def generate_splines(nu, E, p, regularization_points, dim = None, alpha = 0.5):

    if dim is None or len(dim) != 3:
        kx = 12
        ky = 12
    else:
        kx = dim[0] - 4
        ky = dim[1] - 4

    start = [-0.1, 0.0]
    stop  = [ 0.4, 1.0]

    resolution = [(stop[0] - start[0]) / (kx + 1), (stop[1] - start[1]) / (ky + 1)]
    width = [kx + 1, ky + 1]

    print("Res: " + str(resolution))
    print("Width: " + str(width))

    #print(alpha)
    cbs = zpline_2D.CubicBiSpline(start=start, resolution=resolution, width=width, alpha=alpha)

    x = []
    y = []
    for i in range(len(nu)):
        x1_ij = nu[i]
        x2_ij = E[i]

        x.append([x1_ij, x2_ij])
        y.append([p[i]])


    #print("Number of regularization points: " + str(len(regularization_points)))

    cbs.interpolate(x, np.array(y), regularization_points=regularization_points)

    return cbs


def generate_splines_from_other(original_cbs, p):
    cbs = copy.copy(original_cbs)

    y = []
    for i in range(len(p)):
        y.append([p[i]])

    cbs.interpolate_same_base(np.array(y))

    return cbs


def generate_splines_with_coefficients(coeffs):
    kx = coeffs.shape[0] - 4
    ky = coeffs.shape[1] - 4

    start = [-0.1, 0.0]
    stop = [0.4, 1.0]
    alpha = 0.5

    resolution = [(stop[0] - start[0]) / (kx + 1), (stop[1] - start[1]) / (ky + 1)]
    width = [kx + 1, ky + 1]

    print("Res: " + str(resolution))
    print("Width: " + str(width))

    cbs = zpline_2D.CubicBiSpline(start=start, resolution=resolution, width=width, alpha=alpha, coef=coeffs)

    return cbs


class Material2Geometry:

    def __init__(self, nu = [], E = [], p = [], method="lls", in_path = None, dim = None, regularization_coefficient = 0.5, base_nu = 0.3, base_E = 1.0):
        self.base_nu = base_nu
        self.base_E = base_E

        if in_path is not None:
            self.start_with_file(in_path)
            return

        self.method = method

        if method == "lls":
            self.nu = nu
            self.E = np.array(E)
            self.parameter_values = p

            self.p1_map = generate_lls(self.nu, self.E, p[:, 0], dim=dim)
            self.p2_map = generate_lls(self.nu, self.E, p[:, 1], dim=dim)

        elif method == "splines":
            self.nu = nu
            self.E = E
            self.parameter_values = p

            regularization_points = generate_regularization_points()

            print("p1")
            self.p1_map = generate_splines(self.nu, self.E, p[:, 0], regularization_points, dim=dim, alpha=regularization_coefficient)
            print("p2")
            self.p2_map = generate_splines_from_other(self.p1_map, p[:, 1])

        else:
            raise Exception


    def start_with_file(self, input_path):
        input = open(input_path, 'r')

        for idx, line in enumerate(input):
            if idx == 0:
                self.method = line.strip()

            else:
                if self.method == "lls":
                    fields = line.strip().split()

                    parameter = int(fields[0])
                    shape = (int(fields[1]), int(fields[2]))

                    coeffs = []
                    for i in range(4, 4 + shape[0] * shape[1]):
                        coeffs.append(float(fields[i]))

                    if parameter == 1:
                        self.p1_map = generate_lls_with_coefficients(shape[0], coeffs)
                    elif parameter == 2:
                        self.p2_map = generate_lls_with_coefficients(shape[0], coeffs)

                elif self.method == "splines":
                    fields = line.strip().split()

                    parameter = int(fields[0])
                    shape = (int(fields[1]), int(fields[2]))

                    coeffs = []
                    for i in range(3, 3 + shape[0] * shape[1]):
                        coeffs.append(float(fields[i]))

                    coeffs_matrix = np.array(coeffs).reshape(shape)

                    if parameter == 1:
                        self.p1_map = generate_splines_with_coefficients(coeffs_matrix)
                    elif parameter == 2:
                        self.p2_map = generate_splines_with_coefficients(coeffs_matrix)


    #def to_default_base_material(self, nu, E):
    #    defaultNu = nu - (E/self.base_E) * self.base_Nu
    #    defaultE  = E/self.base_E

    #    return defaultNu, defaultE


    def evaluate(self, nu, E):
        # change of variables
        #nu, E = self.to_default_base_material(nu, E)

        p1 = float(self.p1_map(nu, E))
        p2 = float(self.p2_map(nu, E))

        if p1 < 0.0:
            p1 = 0.01
        if p2 < 0.0:
            p2 = 0.01

        p3 = 0.01
        p4 = 0.01

        return [p1, p2, p3, p4]


    def save(self, output_path):

        out = open(output_path, 'w')

        out.write(self.method + "\n")

        for p in range(1, 3):
            out.write("{}\t".format(p))

            if self.method == "lls":
                if p == 1:
                    coeffs = self.p1_map.coeffs
                if p == 2:
                    coeffs = self.p2_map.coeffs

                out.write("{}\t{}\t".format(self.p1_map.d1, self.p1_map.d2))

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

                shape = coeffs.shape
                out.write("{}\t{}\t".format(shape[0], shape[1]))

                coeffs_list = list(np.array(coeffs).reshape(-1,))
                coeffs_string = ""
                for i in range(0, len(coeffs_list)):
                    coeffs_string += "\t" + '{:.16f}'.format(coeffs_list[i])

                out.write(coeffs_string)

            out.write("\n")

        out.close()
