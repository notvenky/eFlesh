import numpy as np
import scipy
import scipy.optimize

class LLSInterpolation:
    #def __init__(self, nu = [], E = [], shear = [], d1 = 3, d2 = 3, coeffs = None):
    def __init__(self, nu=[], E=[], shear=[], d=4, coeffs=None):
        self.d = d

        if coeffs is not None:
            self.coeffs = coeffs
            return

        A = []
        B = np.array(shear)

        # populating A
        for ip in range(0, len(shear)):
            new_row = []

            for i in range(0, self.d+1):
                for j in range(0, self.d+1):
                    if (i + j) > self.d:
                        continue

                    new_term = pow(nu[ip], i) * pow(E[ip], j)
                    new_row.append(new_term)

            A.append(new_row)

        print("Number of terms: " + str(len(A[0])))

        A = np.array(A)
        x = scipy.optimize.lsq_linear(A, B)

        self.coeffs = x.x
        print(x.x)


    def __call__(self, nu, E):
        result = 0.0

        coeff_idx = 0
        for i in range(0, self.d+1):
            for j in range(0, self.d+1):
                if (i + j) > self.d:
                    continue

                result += self.coeffs[coeff_idx] * pow(nu, i) * pow(E, j)

                coeff_idx += 1

        print(result)
        return result

    def save(self, output_path):

        out = open(output_path, 'w')

        out.write("lls\t")

        coeffs = self.coeffs
        out.write("{}".format(self.d))

        coeffs_list = list(np.array(coeffs).reshape(-1, ))
        coeffs_string = ""
        for i in range(0, len(coeffs_list)):
            coeffs_string += "\t" + '{:.16f}'.format(coeffs_list[i])

        coeffs_string += "\n"

        out.write(coeffs_string)
