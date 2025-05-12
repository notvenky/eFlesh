from collections import deque
import logging
import time
from pathlib import Path

import torch
import gradio as gr
import numpy as np
from scipy.signal import savgol_filter

from zmq_utils import create_request_socket, ZMQKeypointPublisher
from reskin_server import ReskinSensorSubscriber

logger = logging.getLogger(__name__)
STRETCH_GRIPPER_MAX = 40

LOCALHOST = "127.0.0.1"
ANYCAST = "0.0.0.0"


def get_home_param(
    h=0.7,
    y=0.02,
    x=0.0,
    yaw=0.0,
    pitch=0.0,
    roll=0.0,
    gripper=1.0,
    closing_threshold=0.5,
    reopening_threshold=0.5,
    stretch_gripper_max=None,
    stretch_gripper_min=None,
    stretch_gripper_tight=None,
    sticky_gripper=None,
    # Below the first value, it will close, above the second value it will open
    gripper_threshold_post_grasp_list=None,
):
    """
    Returns a list of home parameters
    """
    return [
        h,
        y,
        x,
        yaw,
        pitch,
        roll,
        gripper,
        stretch_gripper_max,
        stretch_gripper_min,
        stretch_gripper_tight,
        sticky_gripper,
        closing_threshold,
        reopening_threshold,
        gripper_threshold_post_grasp_list,
    ]


class Controller:
    def __init__(self, cfg):
        self.cfg = cfg
        self.task = cfg["task"]
        network_cfg = cfg["network"]

        publisher = ZMQKeypointPublisher(
            network_cfg.get("host", ANYCAST), network_cfg["action_port"]
        )
        subscriber = ReskinSensorSubscriber()

        self.flag_socket = create_request_socket(
            network_cfg.get("remote", LOCALHOST), port=network_cfg["flag_port"]
        )

        self.publisher = publisher
        self.subscriber = subscriber

        self.prediction_queue = deque(maxlen=cfg["prediction_buffer_size"])

        self.h = cfg["robot_params"]["h"]
        self.stretch_gripper_tight = cfg["robot_params"]["stretch_gripper_tight"]

        self.gripper = 1.0
        self.slip_detection_freq = int(100 / cfg["slip_detection_freq"])

        # pull detection linear classifier
        self.classifier = None

        # Data saving
        self.demo_num = 0
        Path("data_0326").mkdir(exist_ok=True)

        self.demo = self._init_demo()

    def get_gripper_val(self):
        return self.gripper

    def setup_model(self, model):
        self.classifier = model

    def _start_data_collect(self):
        data = deque(maxlen=self.cfg["data_collection_buffer_size"])
        while True:
            start_time = time.time()
            anyskin_state = self.subscriber.get_sensor_state()
            data.append(anyskin_state["sensor_values"])

            if len(data) == self.cfg["data_collection_buffer_size"]:
                seq = torch.as_tensor(np.array(data), dtype=torch.float32)
                torch.save(seq, f"data_0326/seq_{self.demo_num}.pt")
                self.demo_num += 1
                gr.Info(f"Demo {self.demo_num} saved")
                break

            elapsed_time = time.time() - start_time
            sleep_time = max(0, 0.01 - elapsed_time)  # run at 100Hz
            time.sleep(sleep_time)

    def _run(self):
        logger.info("Run robot handover")
        self._close_gripper()

        sensor_queue = deque(maxlen=self.cfg["tactile_buffer_size"])
        baseline = None
        window_len = 10
        xy_mask = [0, 1, 3, 4, 6, 7, 9, 10, 12, 13]

        policy_counter = self.slip_detection_freq
        while True:
            start_time = time.time()
            anyskin_state = self.subscriber.get_sensor_state()
            sensor_queue.append(anyskin_state["sensor_values"])
            policy_counter -= 1

            if len(sensor_queue) >= 50 and baseline is None:
                seq = np.array(sensor_queue)
                seq = savgol_filter(seq, 11, 3, axis=0)
                baseline = np.median(
                    np.array(sensor_queue)[:50, 15:], axis=0, keepdims=True
                )

            if policy_counter <= 0 and baseline is not None:
                policy_counter = self.slip_detection_freq  # reset counter

                # filter, get the last window and subtract baseline, then normalize
                seq = np.array(sensor_queue)
                seq = savgol_filter(seq, 11, 3, axis=0)
                window = seq[-window_len:, 15:] - baseline
                window = window[:, xy_mask] / 100

                xy_total_force = np.abs(window.sum())
                total_diff = window.max() - window.min()
                total_deviation = window.std(axis=0).sum()

                features = np.asarray(
                    [total_diff, xy_total_force, total_deviation]
                ).reshape(1, -1)

                Yhat = self.classifier.predict(features)

                if Yhat:
                    logger.info("Slip")
                    self.prediction_queue.append(1)
                else:
                    logger.info("No-slip")
                    self.prediction_queue.append(0)

                if (
                    sum(self.prediction_queue) / self.cfg["prediction_buffer_size"]
                    > self.cfg["slip_detection_threshold"]
                ):
                    logger.info("Opening gripper")
                    self._open_gripper()
                    # clear the prediction queue
                    self.prediction_queue.clear()
                    break

            elapsed_time = time.time() - start_time
            sleep_time = max(0, 0.01 - elapsed_time)  # run at 100Hz
            time.sleep(sleep_time)

    def _open_gripper(self):
        logger.info("Opening gripper")
        self.gripper = 1.0
        self.flag_socket.send(b"")
        self.publisher.pub_keypoints(
            [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, self.gripper], "robot_action"
        )
        self.flag_socket.recv()
        time.sleep(0.5)

    def _close_gripper(self):
        logger.info("Close gripper")
        self.gripper = 0.0
        self.flag_socket.send(b"")
        self.publisher.pub_keypoints(
            [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, self.gripper], "robot_action"
        )
        self.flag_socket.recv()
        time.sleep(2.0)
        self.flag_socket.send(b"")
        self.publisher.pub_keypoints([1.0], "robot_action")
        self.flag_socket.recv()

    def _run_home(self):
        logger.info("Publishing home")
        self.flag_socket.send(b"")
        self.publisher.pub_keypoints([1], "home")
        self.flag_socket.recv()

    def update_robot_params(
        self,
        height,
        gripper_tight,
        gripper=1.0,
    ):
        logger.info(
            f"Publishing params: height={height}, gripper_tight={gripper_tight}"
        )

        # Map starting position back to the height and base position, then publish that location to move
        x: int = 0
        self.h = height

        self.flag_socket.send(b"")
        self.publisher.pub_keypoints(
            # Below the first value, it will close, above the second value it will open
            get_home_param(
                h=height,
                x=x,
                stretch_gripper_max=STRETCH_GRIPPER_MAX * gripper,
                closing_threshold=0.5,
                reopening_threshold=0.5,
                stretch_gripper_tight=STRETCH_GRIPPER_MAX * gripper_tight,
            ),
            "params",
        )
        self.flag_socket.recv()

    def _init_demo(self):
        # Convert the above demo to blocks.
        with gr.Blocks(theme="soft", analytics_enabled=False) as demo:
            gr.Markdown(
                """
            # AnySkin Demo Admin Tool
            1. Click "Start Handover" to grasp an object.
            2. After grasping, wait for the beep in 2 seconds 
            3. You can now pull the object whenever you want.
            """
            )
            with gr.Row():
                with gr.Column(scale=1):
                    gripper_val = gr.Number(
                        label="Gripper value",
                        show_label=True,
                        interactive=False,
                        value=self.get_gripper_val,
                        every=gr.Timer(0.03),
                    )

                with gr.Column(scale=4):
                    height = gr.Slider(0, 1, value=0.8, step=0.01, label="Height")
                    gripper_tight = gr.Slider(
                        -2.0,
                        0.2,
                        value=-2.0,
                        step=0.01,
                        label="Gripper tightness",
                        info="This is how tight the gripper becomes when it closes. The lower the value, the tighter the gripper.",
                    )

                    with gr.Row():
                        update_params_button = gr.Button(
                            "Update params", variant="secondary"
                        )
                        update_params_button.click(
                            fn=self.update_robot_params,
                            inputs=[
                                height,
                                gripper_tight,
                            ],
                            outputs=None,
                        )
                        home_button = gr.Button("Home the robot")
                        home_button.click(fn=self._run_home)

                    with gr.Row():
                        open_button = gr.Button("Open Gripper")
                        open_button.click(fn=self._open_gripper)

                        close_button = gr.Button("Close Gripper")
                        close_button.click(fn=self._close_gripper)

                    data_collect = gr.Button("Start data collection", variant="primary")
                    data_collect.click(fn=self._start_data_collect)

                    run_button = gr.Button("Start Handover", variant="primary")
                    run_button.click(fn=self._run)

        return demo

    def run(self):
        self.flag_socket.send(b"")
        time.sleep(0.5)
        self.publisher.pub_keypoints(get_home_param(h=self.h), "params")
        self.flag_socket.recv()

        print("Launched the Gradio UI at http://ROBOT_IP:7860/")
        self.demo.launch(prevent_thread_lock=False, server_name="0.0.0.0", quiet=True)
