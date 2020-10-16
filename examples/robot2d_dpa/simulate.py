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

if __name__ == "__main__":
    Omega2dSimulator(
        system_post,         	# dynamics function of the model
        "robot.cfg",          	# the config file oof the problem
    ).start()
