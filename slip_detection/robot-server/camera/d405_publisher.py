import cv2

import numpy as np
import time
import pyrealsense2 as rs
import os

from std_msgs.msg import Float32MultiArray, MultiArrayDimension, Int32
from robot.zmq_utils import ZMQCameraPublisher, ProcessInstantiator
import matplotlib.pyplot as plt

# from numpy_ros import converts_to_message, to_message

NODE_NAME = "gopro_node"
IMAGE_PUBLISHER_NAME = "/gopro_image"
DEPTH_PUBLISHER_NAME = "/gopro_depth"
SEQ_PUBLISHER_NAME = "/gopro_seq"

D405_COLOR_SIZE = [640, 480]
D405_DEPTH_SIZE = [640, 480]
RESIZED_IMAGE = (256, 256)
RESIZED_DEPTH = (256, 192)
D405_FPS = 15


realsense_ctx = rs.context()
connected_devices = {}

for i in range(len(realsense_ctx.devices)):
    camera_name = realsense_ctx.devices[i].get_info(rs.camera_info.name)
    camera_serial = realsense_ctx.devices[i].get_info(rs.camera_info.serial_number)
    connected_devices[camera_name] = camera_serial


# @converts_to_message(Float32MultiArray))
def convert_numpy_array_to_float32_multi_array(matrix):
    # Create a Float64MultiArray object
    data_to_send = Float32MultiArray()

    # Set the layout parameters
    data_to_send.layout.dim.append(MultiArrayDimension())
    data_to_send.layout.dim[0].label = "rows"
    data_to_send.layout.dim[0].size = len(matrix)
    data_to_send.layout.dim[0].stride = len(matrix) * len(matrix[0])

    data_to_send.layout.dim.append(MultiArrayDimension())
    data_to_send.layout.dim[1].label = "columns"
    data_to_send.layout.dim[1].size = len(matrix[0])
    data_to_send.layout.dim[1].stride = len(matrix[0])

    # Flatten the matrix into a list
    data_to_send.data = matrix.flatten().tolist()

    return data_to_send


def setup_realsense_camera(serial_number, color_size, depth_size, fps):
    """
    Returns a Realsense camera pipeline used for accessing D435i & D405's video streams
    """
    pipeline = rs.pipeline()
    config = rs.config()

    if serial_number:
        config.enable_device(serial_number)

    config.enable_stream(
        rs.stream.color, color_size[0], color_size[1], rs.format.bgr8, fps
    )
    config.enable_stream(
        rs.stream.depth, depth_size[0], depth_size[1], rs.format.z16, fps
    )

    profile = pipeline.start(config)
    return pipeline

# def transform_d405_to_iphone(d405_cv2_image, iphone_size = (960, 720)):
#    HOMOGRAPHY = np.array([
#       [ 1.96789705e+00,  1.13820640e-01, -1.54030078e+02],
#       [ 1.03472813e-02,  2.07551671e+00, -1.85031015e+02],
#       [-1.77798511e-05,  2.82942765e-04,  1.00000000e+00]
#    ])
#    warped_image = cv2.warpPerspective(d405_cv2_image, HOMOGRAPHY, iphone_size)
#    return warped_image

def transform_d405_to_iphone(d405_cv2_image, iphone_size = (960, 720)):
   AFFINE_TRANSFORM = np.array([
       [ 1.85133851e+00, -1.32808772e-02, -1.92553519e+02],
       [ 1.32808772e-02,  1.85133851e+00, -1.01745743e+02]
    ])
   warped_image = cv2.warpAffine(d405_cv2_image, AFFINE_TRANSFORM, iphone_size)
   return warped_image

# class D405ImagePublisher(ProcessInstantiator):
class D405ImagePublisher:
    def __init__(self, host, port, use_depth):
        self.host = host
        self.port = port
        self.use_depth = use_depth

        self.rgb_publisher = ZMQCameraPublisher(
            host = self.host, 
            port = self.port
        )
        self._seq = 0

        try:
            d405_serial = connected_devices["Intel RealSense D405"]
        except KeyError:
            raise SystemError("Unable to find Realsense D405...")

        self.pipeline_d405 = setup_realsense_camera(
            serial_number=d405_serial,
            color_size=D405_COLOR_SIZE,
            depth_size=D405_DEPTH_SIZE,
            fps=D405_FPS,
        )

    def stream(self):
        count = 0
        while True:

            frames_d405 = self.pipeline_d405.wait_for_frames()
            color_frame_d405 = frames_d405.get_color_frame()
            depth_frame_d405 = frames_d405.get_depth_frame()
            image = np.asanyarray(color_frame_d405.get_data())
            depth = np.asanyarray(depth_frame_d405.get_data())

            image = transform_d405_to_iphone(image)
            image = cv2.resize(image, dsize=RESIZED_IMAGE, interpolation=cv2.INTER_CUBIC)
            # print(depth.min(), depth.max(), depth.shape)

            if self.use_depth:
                depth = np.ascontiguousarray(depth).astype(np.uint16)
                resized_depth = cv2.resize(depth, RESIZED_DEPTH,  interpolation = cv2.INTER_NEAREST) 
                depth_processed = (resized_depth * 0.0001).astype(np.float32)
                # if "DISPLAY" in os.environ:
                #     cv2.imshow("D405 Depth pre", resized_depth)
                #     cv2.imshow("D405", image)
                
                self.rgb_publisher.pub_image_and_depth(image, depth_processed, time.time())
            else:
                # if "DISPLAY" in os.environ:
                #     cv2.imshow("D405", image)
                
                self.rgb_publisher.pub_rgb_image(image, time.time())

            self._seq += 1

            # Stopping the camera
            # if cv2.waitKey(1) == 27:
            #     break
            time.sleep(1 / D405_FPS)
            count += 1

        cv2.destroyAllWindows()


if __name__ == "__main__":
    print("connected")
    camera_publisher = D405ImagePublisher("localhost", 32922)
    # print('calling publisher')
    camera_publisher.stream()
    # print('publisher end')
