#!/usr/bin/env python

import numpy as np
import math
import scipy
import scipy.sparse
import scipy.sparse.linalg


# [x^0, x^1, x^2, x^3]
poly_coef = [[
    np.array([0, 0, 0, 1]) / 6,
    np.array([4, -12, 12, -3]) / 6,
    np.array([-44, 60, -24, 3]) / 6,
    np.array([64, -48, 12, -1]) / 6
], [
    np.array([0, 0, 0, 1]) / 6,
    np.array([9, -27, 27, -7]) / 12,
    np.array([-135, 189, -81, 11]) / 12
], [
    np.array([0, 0, 0, 1]) / 4,
    np.array([8, -24, 24, -7]) / 4
], [
    np.array([0, 0, 0, 1])
]] + [[  # reversed part
    np.array([1, -3, 3, -1]),
], [
    np.array([0, 12, -18, 7]) / 4,
    np.array([8, -12, 6, -1]) / 4
], [
    np.array([0, 0, 18, -11]) / 12,
    np.array([-18, 54, -36, 7]) / 12,
    np.array([27, -27, 9, -1]) / 6
]]


def poly_coef_derivative(coef):
    """
    Take derivative of poly coefficients
    degree(coef) is len(j)-1
    """
    return [[np.arange(1, len(j)) * j[1:] for j in i] for i in coef]


poly_coef_d1 = poly_coef_derivative(poly_coef)
poly_coef_d2 = poly_coef_derivative(poly_coef_d1)


def flatten(l): return np.asarray([j for i in l for j in i])


poly_coefs = (list(map(flatten, [poly_coef, poly_coef_d1, poly_coef_d2])))


def table_1d(length=10):
    """
    This is a specific table construction. Each row stores the polynomial segments to use
    Left: 0 point for x
    Multi: knot multi type for the basis
    Segment: from left to right, which of the segment is used here.
    b_id: id of the basis, corresponding to coefficent access
    in the end, an additional last row is added for the purpose of evaluating on the last ending knot.
    """

    def left_multi_to_c(left, multi):
        return multi + 3 if left == 0 else left + 3

    def multi_segment_to_coefid(multi, segment):
        return multi_segment_to_coefid.cs[multi] + segment

    multi_segment_to_coefid.cs = np.cumsum([0, 4, 3, 2, 1, 1, 2])
    indices = []
    for i in range(length):
        bases = []
        # (i-1,i,i+1,i+2)
        for j in range(-1, 3):
            segment, multi, left = 2 - j, 0, i + j - 2
            if i + j < 2:
                multi = (i + j) - 2
                segment = i
                left = 0
            elif i + j >= length - 2:
                multi = (i + j) - length + 2
            b_id = left_multi_to_c(left, multi)
            c_id = multi_segment_to_coefid(multi, segment)
            bases.append((left, c_id, b_id))
        indices.append(bases)
    indices = indices + indices[-1:]  # add the last row again, to avoid issue on the last knot
    return indices


def power_list(x, d=3):
    return np.array([1] + [x ** i for i in range(1, d)])


def _bspev_and_c(x, table, coef=poly_coefs[0]):
    if x > len(table) or x < 0: return 0, 0
    tg = table[int(x)]
    degree = coef.shape[1]
    b = [np.dot(coef[cc],
                power_list(x - left, d=degree)) for left, cc in tg[:, :2]]
    i = tg[:, 2].reshape(-1)
    return b, i


def _bspev_and_cgrid_3D(x, tables, coef_u=poly_coefs[0], coef_v=poly_coefs[0], coef_w=poly_coefs[0]):
    bu, iu = _bspev_and_c(x[0], tables[0], coef_u)
    bv, iv = _bspev_and_c(x[1], tables[1], coef_v)
    bw, iw = _bspev_and_c(x[2], tables[2], coef_w)

    basis_values = np.prod(np.ix_(bu, bv, bw))
    indices = (iu.reshape(-1, 1, 1), iv.reshape(1, -1, 1), iw.reshape(1, 1, -1))

    return basis_values, indices


class CubicTriSpline:
    def __init__(self, start, resolution, width, coef=None, alpha=1.0):
        self.start = np.asarray(start)
        self.scale = 1 / np.asarray(resolution)
        self.table = [np.asarray(table_1d(w)) for w in width]
        self.coef = coef
        self.alpha = alpha

        self.transform = lambda x: (x - self.start) * self.scale

    def __call__(self, x):
        x = self.transform(x)
        outer, ixgrid = _bspev_and_cgrid_3D(x, self.table)
        c = self.coef[ixgrid].ravel()
        return np.dot(outer.ravel(), c)

    def __call__(self, x, y, z):
        x = self.transform([x, y, z])
        outer, ixgrid = _bspev_and_cgrid_3D(x, self.table)
        c = self.coef[ixgrid].ravel()
        return np.dot(outer.ravel(), c)

    def interpolate(self, X, f, regularization_points = None):
        X = self.transform(X)
        self.basis_values = np.array(
            [(i, c, d) for i, x in enumerate(X) for c, d in self._global_basis_row(x, self.table)])

        if regularization_points is None:
            regularization_points = X
        else:
            regularization_points = self.transform(regularization_points)

        laps_list = []
        num_laps_rows = 0
        for i, x in enumerate(regularization_points):
            for laps_rows in self._global_basis_d2_row(x, self.table):
                for c, d in laps_rows:
                    laps_list.append((X.shape[0] + num_laps_rows, c, self.alpha * d))

                num_laps_rows += 1

        self.laps_values = np.array(laps_list)

        self.RCD = np.vstack([self.basis_values, self.laps_values])
        self.A = scipy.sparse.csr_matrix((self.RCD[:, 2], (self.RCD[:, 0].astype(np.int32), self.RCD[:, 1].astype(np.int32))),
                                    shape=(X.shape[0] + num_laps_rows, (len(self.table[0]) + 2) * (len(self.table[1]) + 2) * (len(self.table[2]) + 2)))

        self.f2 = np.zeros((num_laps_rows, f.shape[1]))
        f = np.vstack([f, self.f2])

        coef = scipy.sparse.linalg.spsolve(self.A.transpose() @ self.A + 1e-8*scipy.sparse.eye(self.A.shape[1]), self.A.transpose() @ f)
        self.coef = coef.reshape(len(self.table[0]) + 2, len(self.table[1]) + 2, len(self.table[2]) + 2)

    def interpolate_same_base(self, f):
        f = np.vstack([f, self.f2])

        coef = scipy.sparse.linalg.spsolve(self.A.transpose() @ self.A + 1e-8*scipy.sparse.eye(self.A.shape[1]), self.A.transpose() @ f)
        self.coef = coef.reshape(len(self.table[0]) + 2, len(self.table[1]) + 2, len(self.table[2]) + 2)

    @classmethod
    def _global_basis_row(cls, x, tables):
        outer, ixgrid = _bspev_and_cgrid_3D(x, tables)
        index = np.ravel_multi_index(ixgrid, (len(tables[0]) + 2, len(tables[1]) + 2, len(tables[2]) + 2))

        # return cols and data
        return list(zip(index.ravel(), outer.ravel()))

    @classmethod
    def _global_basis_d2_row(cls, x, tables):
        dim = (len(tables[0]) + 2, len(tables[1]) + 2, len(tables[2]) + 2)

        outer, ixgrid = _bspev_and_cgrid_3D(x, tables, coef_u=poly_coefs[2], coef_v=poly_coefs[0], coef_w=poly_coefs[0])
        ind = np.ravel_multi_index(ixgrid, dim).ravel()
        out = outer.ravel()
        row1 = list(zip(ind, out))

        outer, ixgrid = _bspev_and_cgrid_3D(x, tables, coef_u=poly_coefs[1], coef_v=poly_coefs[1], coef_w=poly_coefs[0])
        ind = np.ravel_multi_index(ixgrid, dim).ravel()
        out = outer.ravel()
        row2 = list(zip(ind, math.sqrt(2)*out))

        outer, ixgrid = _bspev_and_cgrid_3D(x, tables, coef_u=poly_coefs[1], coef_v=poly_coefs[0], coef_w=poly_coefs[1])
        ind = np.ravel_multi_index(ixgrid, dim).ravel()
        out = outer.ravel()
        row3 = list(zip(ind, math.sqrt(2)*out))

        outer, ixgrid = _bspev_and_cgrid_3D(x, tables, coef_u=poly_coefs[0], coef_v=poly_coefs[2], coef_w=poly_coefs[0])
        ind = np.ravel_multi_index(ixgrid, dim).ravel()
        out = outer.ravel()
        row4 = list(zip(ind, out))

        outer, ixgrid = _bspev_and_cgrid_3D(x, tables, coef_u=poly_coefs[0], coef_v=poly_coefs[1], coef_w=poly_coefs[1])
        ind = np.ravel_multi_index(ixgrid, dim).ravel()
        out = outer.ravel()
        row5 = list(zip(ind, math.sqrt(2)*out))

        outer, ixgrid = _bspev_and_cgrid_3D(x, tables, coef_u=poly_coefs[0], coef_v=poly_coefs[0], coef_w=poly_coefs[2])
        ind = np.ravel_multi_index(ixgrid, dim).ravel()
        out = outer.ravel()
        row6 = list(zip(ind, out))


        return [row1, row2, row3, row4, row5, row6]


def test_3d():
    c = np.ones((13, 13, 13))

    cbs = CubicTriSpline(start=[0, 0, 0], resolution=[0.1, 0.1, 0.1], width=[10, 20, 30], coef=c)
    y = cbs([0.6, 0.3, 0.4])
    print("Supposed to be 1.0: " + str(y))


    # prepare data to fit
    x1 = np.arange(-6, 6.1, 0.5)
    x2 = np.arange(-6, 6.1, 0.5)
    x3 = np.arange(-6, 6.1, 0.5)

    X1, X2, X3 = np.meshgrid(x1, x2, x3)
    Y = np.sqrt(X1 ** 2 + X2 ** 2 + X3 ** 2)

    x = []
    y = []
    for i in range(len(x1)):
        for j in range(len(x2)):
            for k in range(len(x3)):
                x1_ij = X1[i, j, k]
                x2_ij = X2[i, j, k]
                x3_ij = X3[i, j, k]

                x.append([x1_ij, x2_ij, x3_ij])
                y.append([Y[i, j, k]])

    # generate new coefficients
    cbs = CubicTriSpline(start=[-6.0, -6.0, -6.0], resolution=[1.0, 1.0, 1.0], width=[12, 12, 12], coef=c)

    cbs.interpolate(x, np.array(y))

    x1_sampling = np.arange(-6, 6.1, 0.5)
    x2_sampling = np.arange(-6, 6.1, 0.5)
    x3_sampling = np.arange(-6, 6.1, 0.5)
    Zbs = np.zeros((len(x1_sampling), len(x2_sampling), len(x3_sampling)))
    X1_sampling, X2_sampling, X3_sampling = np.meshgrid(x1_sampling, x2_sampling, x3_sampling)

    for i in range(len(x1_sampling)):
        for j in range(len(x2_sampling)):
            for k in range(len(x3_sampling)):
                x1_ij = X1_sampling[i, j, k]
                x2_ij = X2_sampling[i, j, k]
                x3_ij = X3_sampling[i, j, k]
                #Zbs[i, j, k] = cbs.ev([x1_ij, x2_ij, x3_ij])[0]
                Zbs[i, j, k] = cbs([x1_ij, x2_ij, x3_ij])

    # Error:
    total_error = 0.0
    for i in range(len(x1)):
        for j in range(len(x2)):
            for k in range(len(x3)):
                error = abs(Zbs[i, j, k] - Y[i, j, k])
                #print("Error at {},{},{}: {}".format(i, j, k, error))
                total_error += error * error
    print("Total error: " + str(math.sqrt(total_error)))


if __name__ == '__main__':
    test_3d()
    pass
