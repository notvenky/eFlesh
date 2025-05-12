from .tensor_subscriber import TensorSubscriber
from .hello_robot import HelloRobot

import time
import zmq
from ..zmq_utils import *

class Listener(ProcessInstantiator):
    def __init__(self, host, hello_robot, hello_robot_config, gripper_safety_limits, translation_safety_limits, stream_during_motion, port_configs):
        super().__init__()
        self.hello_robot = hello_robot
        self.gripper_safety_limits = gripper_safety_limits
        self.translation_safety_limits = translation_safety_limits
        self.stream_during_motion = stream_during_motion
        self.host=host

        print("starting robot listner")
        if self.hello_robot is None:
            if hello_robot_config is not None:
                self.hello_robot = HelloRobot(**hello_robot_config)
            else:
                self.hello_robot = HelloRobot()

        self.hello_robot.home()
        self.tensor_subscriber = TensorSubscriber(port_configs)

    # continue looping until instruction is given, then handle instruction
    def _wait_and_execute_action(self):
        while True:
            robot_action = self.tensor_subscriber.robot_action_subscriber.recv_keypoints(flags=zmq.NOBLOCK)
            if robot_action is not None:
                print('received action')
                self._handle_action("robot_action", robot_action)
                return
            home = self.tensor_subscriber.home_subscriber.recv_keypoints(flags=zmq.NOBLOCK)
            if home is not None:
                print('received home')
                self._handle_action("home", home)
                return
            home_params = self.tensor_subscriber.home_params_subscriber.recv_keypoints(flags=zmq.NOBLOCK)
            if home_params is not None:
                print('received home params')
                self._handle_action("home_params", home_params)
                return

    def _handle_action(self, instruction, data):
        if instruction == "robot_action":
            self._execute_robot_action(data)
        elif instruction == "home":
            self.hello_robot.home()
        elif instruction == "home_params":
            self.hello_robot.set_home_position(*data)

    # execute the robot action given by policy
    def _execute_robot_action(self, action):
        print("Received action to execute at", time.time())

        if len(action) == 7:
            translation_tensor = action[:3]
            rotational_tensor = action[3:6]
            gripper_tensor = [action[-1]]
            print('received robot action')
            self.hello_robot.move_to_pose(
                translation_tensor, rotational_tensor, gripper_tensor
            )
        elif len(action) == 1:
            print('received beep action')
            beep_tensor = action[-1]
            if beep_tensor == 1:
                print('beeping')
                self.hello_robot.robot.pimu.trigger_beep()
                self.hello_robot.robot.push_command()



    # wait for flag to before waiting for action
    def _wait_for_flag(self):
        print('waiting for flag')
        self.tensor_subscriber.flag_socket.recv()
        print('flag received')

    # send flag back once action is executed
    def _send_flag(self):
        self.tensor_subscriber.flag_socket.send(b"")

    def stream(self):
        print("server started")
        while True:
            self._wait_for_flag()
            self._wait_and_execute_action()
            self._send_flag()
