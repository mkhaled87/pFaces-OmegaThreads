import sys
import math

# insert interface folder of the tool
sys.path.insert(1, '../../interface/python')
from Omega2dSimulator import Omega2dSimulator


def system_post(x,u):
    action = int(u[0])
    next_x = x
    if action == 0:
        next_x[0] = x[0] - 1.0
    elif action == 1:
        next_x[1] = x[1] + 1.0
    elif action == 2:
        next_x[0] = x[0] + 1.0
    elif action == 3:
        next_x[1] = x[1] - 1.0
    else:
        pass
    return next_x

def main():
    Omega2dSimulator(
        600,                    # screen width
        600,                    # screen hight
        "2D Robot Example",     # screen title
        system_post,         	# dynamics function of the model
        [0.5,0.5],              # initial state for the simulation
        0.5,        	        # sampling period to be used in computing the next states
        "robot.cfg",          	# the config file oof the problem
        "robot.mdf",          	# the controller file of the problem
        "robot.png",          	# an image file to represent the model
        0.04,                   # scale factor of the model image
        False,                  # simulate dimension 3 ?
        False,                  # is model_dynamics ODE ?
    ).start()

if __name__ == "__main__":
    main()
