from .tensor_subscriber import TensorSubscriber
from .xarm import xArm

import time
import zmq
from ..zmq_utils import *
    
class Listener(ProcessInstantiator):
    def __init__(self, xarm, port_configs):
        super().__init__()
        self.xarm = xarm
        
        print("starting robot listner")
        print(port_configs)
        if self.xarm is None:
            self.xarm = xArm(xarm_ip=port_configs["xarm_ip"])

        self.xarm.home()
        self.tensor_subscriber = TensorSubscriber(port_configs)

    # continue looping until instruction is given, then handle instruction
    def _wait_and_execute_action(self):
        while True:
            # TODO check this sleep
            time.sleep(0.005)
            robot_action = self.tensor_subscriber.robot_action_subscriber.recv_keypoints(flags=zmq.NOBLOCK) 
            if robot_action is not None:
                print('received action')
                print("Time at receiving action", time.time())
                self._handle_action("robot_action", data=robot_action)
                return
            home = self.tensor_subscriber.home_subscriber.recv_keypoints(flags=zmq.NOBLOCK)
            if home is not None:
                print('received home')
                self._handle_action("home")
                return
            home_params = self.tensor_subscriber.home_params_subscriber.recv_keypoints(flags=zmq.NOBLOCK)
            if home_params is not None:
                self._handle_action("home_params", data=home_params)
                return
            quit = self.tensor_subscriber.quit_subscriber.recv_keypoints(flags=zmq.NOBLOCK)
            if quit:
                self._handle_action("quit")
                return

    def _handle_action(self, instruction, data=None):
        if instruction == "robot_action":
            self._execute_robot_action(data)
        elif instruction == "home":
            self.xarm.home()
        elif instruction == "home_params":
            self.xarm.move_relative(data)
        elif instruction == "quit":
            self.xarm.open_gripper()
            self.xarm.gripper.disable()
            quit()
    
    # execute the robot action given by policy
    def _execute_robot_action(self, action):
        print("Received action to execute at", time.time())

        relative_action = action[:-1]
        gripper = action[-1]
        
        self.xarm.move_to_pose(
            relative_action, gripper
        )
    
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
