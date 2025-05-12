from camera.demo import R3DApp
import cv2
import time
from robot.zmq_utils import *
import os
    
class R3DCameraPublisher(ProcessInstantiator):
    def __init__(self, host, port, use_depth):
        super().__init__()
        self.host = host
        self.port = port
        self.use_depth = use_depth
        self.rgb_publisher = ZMQCameraPublisher(
            host = self.host, 
            port = self.port
        )
        
        self._seq = 0
        self.timer = FrequencyTimer(50)

        self._start_camera()

    # start the Record3D streaming
    def _start_camera(self):
        self.app = R3DApp()
        while self.app.stream_stopped:
            try:
                self.app.connect_to_device(dev_idx=0)
            except RuntimeError as e:
                print(e)
                print(
                    "Retrying to connect to device with id {idx}, make sure the device is connected and id is correct...".format(
                        idx=0
                    )
                )
                time.sleep(2)

    # get the RGB and depth images from the Record3D
    def get_rgb_depth_images(self):
        image = None
        while image is None:
            image, depth, pose = self.app.start_process_image()
            image = np.moveaxis(image, [0], [1])[..., ::-1, ::-1]
            image = cv2.resize(image, dsize=(256, 256), interpolation=cv2.INTER_CUBIC)
        if self.use_depth: 
            depth = np.ascontiguousarray(np.rot90(depth, -1)).astype(np.float64)  
            return image, depth, pose
        else:
            return image, pose
    
    # get RGB images at 50Hz and publish them to the ZMQ port
    def stream(self):
        while True:
            if self.app.stream_stopped:
                try:
                    self.app.connect_to_device(dev_idx=0)
                except RuntimeError as e:
                    print(e)
                    print(
                        "Retrying to connect to device with id {idx}, make sure the device is connected and id is correct...".format(
                            idx=0
                        )
                    )
                    time.sleep(2)
            else:
                self.timer.start_loop()
                if self.use_depth:
                    image, depth, pose = self.get_rgb_depth_images()
                    self.rgb_publisher.pub_image_and_depth(image, depth, time.time())
                else:
                    image, pose = self.get_rgb_depth_images()
                    self.rgb_publisher.pub_rgb_image(image, time.time())
                self.timer.end_loop()

                if "DISPLAY" in os.environ:
                    cv2.imshow("iPhone", image)
            
                if cv2.waitKey(1) == 27:
                    break
        
        cv2.destroyAllWindows()