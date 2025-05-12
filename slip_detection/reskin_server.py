import hydra
from collections import deque
import time
from zmq_utils import ZMQKeypointPublisher, ZMQKeypointSubscriber

HOST = "localhost"
RESKIN_STREAM_PORT = 12005

from reskin_sensor import ReSkinProcess

def notify_component_start(component_name):
    print("***************************************************************")
    print("     Starting {} component".format(component_name))
    print("***************************************************************")


class FrequencyTimer(object):
    def __init__(self, frequency_rate):
        self.time_available = 1e9 / frequency_rate

    def start_loop(self):
        self.start_time = time.time_ns()

    def check_time(self, frequency_rate):
        # if prev_check_time variable doesn't exist, create it
        if not hasattr(self, "prev_check_time"):
            self.prev_check_time = self.start_time

        curr_time = time.time_ns()
        if (curr_time - self.prev_check_time) > 1e9 / frequency_rate:
            self.prev_check_time = curr_time
            return True
        return False

    def end_loop(self):
        wait_time = self.time_available + self.start_time

        while time.time_ns() < wait_time:
            continue

class ReskinSensorPublisher:
    def __init__(self, reskin_config):
        self.reskin_publisher = ZMQKeypointPublisher(HOST, RESKIN_STREAM_PORT)

        self.timer = FrequencyTimer(100)
        self.reskin_config = reskin_config
        if reskin_config.history is not None:
            self.history = deque(maxlen=reskin_config.history)
        else:
            self.history = deque(maxlen=1)
        self._start_reskin()

    def _start_reskin(self):
        self.sensor_proc = ReSkinProcess(
            num_mags=self.reskin_config["num_mags"],
            port=self.reskin_config["port"],
            baudrate=100000,
            burst_mode=True,
            device_id=0,
            temp_filtered=True,
            reskin_data_struct=True,
        )
        self.sensor_proc.start()
        time.sleep(0.5)

    def stream(self):
        notify_component_start("Reskin sensors")

        while True:
            try:
                self.timer.start_loop()
                reskin_state = self.sensor_proc.get_data(1)[0]
                data_dict = {}
                data_dict["timestamp"] = reskin_state.time
                data_dict["sensor_values"] = reskin_state.data
                self.history.append(reskin_state.data)
                data_dict["sensor_history"] = list(self.history)
                self.reskin_publisher.pub_keypoints(data_dict, topic_name="reskin")
                self.timer.end_loop()

            except KeyboardInterrupt:
                break


class ReskinSensorSubscriber:
    def __init__(self):
        self.reskin_subscriber = ZMQKeypointSubscriber(
            HOST, RESKIN_STREAM_PORT, topic="reskin"
        )

    def __repr__(self):
        return "reskin"

    def get_sensor_state(self):
        reskin_state = self.reskin_subscriber.recv_keypoints()
        return reskin_state


@hydra.main(version_base="1.2", config_path="configs", config_name="reskin")
def main(cfg):
    reskin_publisher = ReskinSensorPublisher(reskin_config=cfg.reskin_config)
    reskin_publisher.stream()


if __name__ == "__main__":
    main()
