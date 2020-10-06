
import sys
import math

# insert interface folder of the tool
sys.path.insert(1, '../../interface/python')
from Omega2dSimulator import Omega2dSimulator


# system dynamics
sampling_period = 0.3
def model_dynamics(x,u):
    xx_0 = u[0]*math.cos(math.atan((math.tan(u[1])/2.0))+x[2])/math.cos(math.atan((math.tan(u[1])/2.0)))
    xx_1 = u[0]*math.sin(math.atan((math.tan(u[1])/2.0))+x[2])/math.cos(math.atan((math.tan(u[1])/2.0)))
    xx_2 = u[0]*math.tan(u[1])
    return [xx_0, xx_1, xx_2]

def main():
    Omega2dSimulator(
        600,                    # screen width
        600,                    # screen hight
        "Vehicle Example",      # screen title
        model_dynamics,         # dynamics function of the model
        [0.5,1.5,1.5],          # initial state for the simulation
        sampling_period,        # sampling period to be used in computing the next states
        "vehicle.cfg",          # the config file oof the problem
        "vehicle.mdf",          # the controller file of the problem
        "vehicle.png",          # an image file to represent the model
        0.035                   # scale factor of the model image
    ).start()

if __name__ == "__main__":
    main()