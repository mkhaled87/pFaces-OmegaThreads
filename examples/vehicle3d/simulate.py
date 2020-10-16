
import sys
import math

# insert interface folder of the tool
sys.path.insert(1, '../../interface/python')
from Omega2dSimulator import Omega2dSimulator

# system ode
def model_ode(x,u):
    xx_0 = u[0]*math.cos(math.atan((math.tan(u[1])/2.0))+x[2])/math.cos(math.atan((math.tan(u[1])/2.0)))
    xx_1 = u[0]*math.sin(math.atan((math.tan(u[1])/2.0))+x[2])/math.cos(math.atan((math.tan(u[1])/2.0)))
    xx_2 = u[0]*math.tan(u[1])
    return [xx_0, xx_1, xx_2]

# start the simulation as main function
if __name__ == "__main__":
    Omega2dSimulator(
        model_ode,              # dynamics function of the model
        "vehicle.cfg",          # the config file oof the problem
    ).start()
