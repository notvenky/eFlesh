from xarm.wrapper import XArmAPI
import numpy as np
import time

from robot.utils import create_transform, transform_to_vec
from robot.xarm.gripper import Gripper

HOME_POS = [284.008911, -18.132006, 484.083954, -27.36647, -87.102247, -155.131474]
END_EFFECTOR_TO_IPHONE = [125,0,95,0,-15,0]
GRIPPER_OPEN = 3100
GRIPPER_CLOSE = 1200

# xArm coordinate system:
""" 
        x      y
        |    /
        |  /
z ______|/
"""

class xArm:
    def __init__(self, xarm_ip):
        self.arm = XArmAPI(xarm_ip)
        self.arm.connect()
        self.arm.motion_enable(enable=True)
        self.arm.set_mode(0)
        self.arm.set_state(0)
        
        print('xArm initialized')
        
        self.wrist_to_iphone = create_transform(END_EFFECTOR_TO_IPHONE)
        self.base_to_home = create_transform(HOME_POS)
        self.gripper = Gripper()
        self.open_gripper()
        
    def open_gripper(self):
        self.gripper.move_to_pos(GRIPPER_OPEN)
    
    def home(self):
        self.open_gripper()
        
        self.arm.set_position(*HOME_POS, speed=100, mvacc=1000, wait=True)
        
        # TODO: clean up gripper movement code
        self.gripper_has_moved = False
    
    # moves from home position to run starting position given relative motion from server
    def move_relative(self, relative_action):
        print(relative_action)
        relative_action = np.array(relative_action)[:-1] # convert to numpy array get rid of gripper value
        relative_action[:3] = 1000 * np.array([[1,0,0],[0,0,-1],[0,1,0]]) @ relative_action[:3] # swap y and -z, and convert to mm

        new_pos = transform_to_vec(self.base_to_home @ create_transform(relative_action))
            
        self.arm.set_position(*new_pos, speed=100, mvacc=1000, wait=True)
        self.open_gripper()
    
    def move_to_pose(self, relative_action, gripper):
        print("Time at executing action", time.time())
        code, current_pos = self.arm.get_position(is_radian=False)
        if code == 0:
            relative_action[:3] *= 1000 # convert translation from m to mm
            relative_action[0] *= -1
            relative_action[2] *= -1
            
            relative_action[3:] = np.rad2deg(relative_action[3:]) # convert from radians to degrees
            relative_action[3] *= -1
            relative_action[5] *= -1
            print(relative_action)
            
            base_to_end_effector = create_transform(current_pos)
            iphone_to_action = create_transform(relative_action)
            
            full_transform = base_to_end_effector @ self.wrist_to_iphone @ iphone_to_action @ np.linalg.inv(self.wrist_to_iphone)
            new_pos = transform_to_vec(full_transform)
            
            self.arm.set_position(*new_pos, speed=100, mvacc=1000, wait=True)
            
            if gripper < 0.5 and not self.gripper_has_moved:
                self.gripper.move_to_pos(800)
                self.gripper_has_moved = True  
            elif gripper > 0.9 and self.gripper_has_moved:
                self.gripper.move_to_pos(GRIPPER_OPEN)