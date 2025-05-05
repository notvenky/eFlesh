import matplotlib.pyplot as plt
from shapely.geometry.polygon import LinearRing
from shapely.geometry.polygon import Polygon

def plot_line(ax, ob, color):
    x, y = ob.xy
    ax.plot(x, y, color=color, alpha=0.7, linewidth=3,
            solid_capstyle='round', zorder=2)

polygon = [[-29.675, -30.675],
           [-28.4094, -29.4094],
           [-28.325, -29.325],
           [-28.325, -29.764],
           [-28.325, -29.7933],
           [-28.4587, -29.8274],
           [-28.4676, -29.8297],
           [-28.5956, -29.8814],
           [-28.6041, -29.8848],
           [-28.724, -29.953],
           [-28.732, -29.9576],
           [-28.8417, -30.0413],
           [-28.849, -30.0469],
           [-28.9466, -30.1445],
           [-28.9531, -30.151],
           [-29.0368, -30.2607],
           [-29.0424, -30.268],
           [-29.1106, -30.3879],
           [-29.1152, -30.3959],
           [-29.1669, -30.5239],
           [-29.1703, -30.5324],
           [-29.2044, -30.6661],
           [-29.2067, -30.675],
           [-29.6457, -30.675],
           [-29.675, -30.675]]

polygon = LinearRing([[-0.1087855498584256, 0.3807501316112708], [-0.0941074272945553, 0.3954282541751412], [-0.1135924617569318, 0.3970800556159928], [-0.1256288333862892, 0.3975934151391343], [-0.1087855498584256, 0.3807501316112708]])
poly_line = LinearRing(polygon)
poly_line_offset = poly_line.parallel_offset(0.002, side="left")

#for p in poly_line_offset:
#    poly = Polygon(p)

#    print(poly.area)

#print(poly_line_offset[0])
print(poly_line_offset)

fig = plt.figure()
ax = fig.add_subplot(111)
plot_line(ax, poly_line, "blue")
plot_line(ax, poly_line_offset, "green")
plt.show()