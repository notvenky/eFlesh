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

D435I_COLOR_SIZE = [640, 480]
D435I_DEPTH_SIZE = [640, 480]
D435I_FPS = 30


realsense_ctx = rs.context()
connected_devices = {}

for i in range(len(realsense_ctx.devices)):
    camera_name = realsense_ctx.devices[i].get_info(rs.camera_info.name)
    camera_serial = realsense_ctx.devices[i].get_info(rs.camera_info.serial_number)
    connected_devices[camera_name] = camera_serial

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

class D435ImagePublisher:
    def __init__(self, host, port, use_depth):
        self.host = host
        self.port = port
        self.use_depth = use_depth

        self.rgb_publisher = ZMQCameraPublisher(
            host = self.host, 
            port = self.port
        )
        self._seq = 0
        print(connected_devices)
        try:
            d435i_serial = connected_devices["Intel RealSense D435I"]
        except KeyError:
            raise SystemError("Unable to find Realsense D435I...")

        self.pipeline_d435i = setup_realsense_camera(
            serial_number=d435i_serial,
            color_size=D435I_COLOR_SIZE,
            depth_size=D435I_DEPTH_SIZE,
            fps=D435I_FPS,
        )
    
    def get_head_image_and_depth(self):
        frames_d435i = self.pipeline_d435i.wait_for_frames()
        color_frame_d435i = frames_d435i.get_color_frame()
        depth_frame_d435i = frames_d435i.get_depth_frame()
        image = np.asanyarray(color_frame_d435i.get_data())
        depth = np.asanyarray(depth_frame_d435i.get_data())

        image = np.rot90(image, k=-1)

        return image, depth

    def stream(self):
        count = 0
        while True:
            image, depth = self.get_head_image_and_depth()    

            if self.use_depth:
                depth = np.ascontiguousarray(depth).astype(np.uint16)
                resized_depth = cv2.resize(depth, D435I_DEPTH_SIZE,  interpolation = cv2.INTER_NEAREST) 
                depth_processed = (resized_depth * 0.0001).astype(np.float32)

                if "DISPLAY" in os.environ:
                    cv2.imshow("D435i Depth pre", resized_depth)
                    cv2.imshow("D435i", image)
                
                self.rgb_publisher.pub_image_and_depth(image, depth_processed, time.time())
            else:
                if "DISPLAY" in os.environ:
                    cv2.imshow("D435i", image)
                
                self.rgb_publisher.pub_rgb_image(image, time.time())

            self._seq += 1

            # Stopping the camera
            if cv2.waitKey(1) == 27:
                break
            time.sleep(1 / D435I_FPS)
            count += 1

        cv2.destroyAllWindows()


if __name__ == "__main__":
    print("connected")
    camera_publisher = D435ImagePublisher("localhost", 32922)
    # print('calling publisher')
    camera_publisher.stream()
    # print('publisher end')