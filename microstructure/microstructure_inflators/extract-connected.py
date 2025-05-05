import igl, argparse
import numpy as np

parser = argparse.ArgumentParser()
parser.add_argument("input", type=str)
parser.add_argument("output", type=str)
args = parser.parse_args()

v, _, _, f, _, _ = igl.read_obj(args.input)

C = igl.connected_components(igl.adjacency_matrix(f))

corner = np.argmin(v, axis=0)[0]
cube_flag = C[1][corner]

if C[0] != 2:
    print("Wrong number of connected components!")
    exit(0)

v_mask = (C[1] != cube_flag)
f_mask = v_mask[f[:,0]]

sv, sf, _, _ = igl.remove_unreferenced(v, f[f_mask,:])
sf[:,[0,1]] = sf[:,[1,0]]
igl.write_obj(args.output, sv, sf)