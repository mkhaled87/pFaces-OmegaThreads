import sys
import math

# insert interface folder of the tool
sys.path.insert(1, '../../interface/python')
from Omega2dSimulator import Omega2dSimulator
from OmegaInterface import RungeKuttaSolver

# ODE (continuos time) of the unicycle dynamics
sampling_period = 0.1
def unicycle_ode(x,u):
    xx_0 = 15.0*u[0]*math.cos(u[1])
    xx_1 = 15.0*u[0]*math.sin(u[1])
    return [xx_0, xx_1]

# Difference equation (discrete time) of the battery
def battery_level_de(x):
    battery_level = x[2]
    is_charging = (x[0] >= 6.0 and x[0] <= 9.0) and (x[1] >= 0.0 and x[1] <= 3.0)
    if is_charging:        
        battery_level += 20.0
        if battery_level > 99.0:
            battery_level = 99.0    
    else:
        battery_level -= 1.0
        if battery_level < 0.0:
            battery_level = 0.0

    return battery_level
            
# an ODE solver object
ode = RungeKuttaSolver(unicycle_ode, 5)

# the complete hybrid model
def system_post(x,u):

    # compute posts
    unicycle_post = ode.RK4([x[0], x[1]], u, sampling_period)
    battery_level = battery_level_de(x)

    return [unicycle_post[0], unicycle_post[1], battery_level]


if __name__ == "__main__":
    Omega2dSimulator(
        system_post,         				    # dynamics function of the model
        "pickupdelivery.cfg",          			# the config file oof the problem
    ).start()
