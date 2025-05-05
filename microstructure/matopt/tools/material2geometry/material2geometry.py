#!/usr/bin/env python3
import argparse

import numpy as np

import zpline_2D

import copy


def generate_splines_with_coefficients(coeffs):
    kx = coeffs.shape[0] - 4
    ky = coeffs.shape[1] - 4

    start = [-0.1, 0.0]
    stop = [0.4, 0.5]
    alpha = 0.5

    resolution = [(stop[0] - start[0]) / (kx + 1), (stop[1] - start[1]) / (ky + 1)]
    width = [kx + 1, ky + 1]

    print("Res: " + str(resolution))
    print("Width: " + str(width))

    cbs = zpline_2D.CubicBiSpline(start=start, resolution=resolution, width=width, alpha=alpha, coef=coeffs)

    return cbs


class Material2Geometry:

    def __init__(self, in_path = None, base_nu = 0.3, base_E = 1.0):
        self.base_nu = base_nu
        self.base_E = base_E

        self.method = "splines"

        self.start_with_file(in_path)


    def start_with_file(self, input_path):
        input = open(input_path, 'r')

        for idx, line in enumerate(input):
            if idx == 0:
                self.method = line.strip()

            else:
                if self.method == "splines":
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
                    elif parameter == 3:
                        self.p3_map = generate_splines_with_coefficients(coeffs_matrix)
                    elif parameter == 4:
                        self.p4_map = generate_splines_with_coefficients(coeffs_matrix)
                    elif parameter == 5:
                        self.p5_map = generate_splines_with_coefficients(coeffs_matrix)
                    elif parameter == 6:
                        self.p6_map = generate_splines_with_coefficients(coeffs_matrix)
                    elif parameter == 7:
                        self.p7_map = generate_splines_with_coefficients(coeffs_matrix)
                    elif parameter == 8:
                        self.p8_map = generate_splines_with_coefficients(coeffs_matrix)
                    elif parameter == 9:
                        self.p9_map = generate_splines_with_coefficients(coeffs_matrix)


    #def to_default_base_material(self, nu, E):
    #    defaultNu = nu - (E/self.base_E) * self.base_Nu
    #    defaultE  = E/self.base_E

    #    return defaultNu, defaultE


    def evaluate(self, nu, E):
        # change of variables
        #nu, E = self.to_default_base_material(nu, E)

        p1 = float(self.p1_map(nu, E))
        p2 = float(self.p2_map(nu, E))
        p3 = float(self.p3_map(nu, E))
        p4 = float(self.p4_map(nu, E))
        p5 = float(self.p5_map(nu, E))
        p6 = float(self.p6_map(nu, E))
        p7 = float(self.p7_map(nu, E))
        p8 = float(self.p8_map(nu, E))
        p9 = float(self.p9_map(nu, E))

        if p1 < 0.0:
            p1 = 0.01
        if p2 < 0.0:
            p2 = 0.01
        if p3 < 0.0:
            p3 = 0.01
        if p4 < 0.0:
            p4 = 0.01
        if p5 < 0.0:
            p5 = 0.01
        if p6 < 0.0:
            p6 = 0.01
        if p7 < 0.0:
            p7 = 0.01
        if p8 < 0.0:
            p8 = 0.01
        if p9 < 0.0:
            p9 = 0.01

        p10 = 0.01
        p11 = 0.01
        p12 = 0.01
        p13 = 0.01

        return [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13]

