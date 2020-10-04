
import sys
import math

# insert interface folder of the tool
sys.path.insert(1, '../../interface/python')
from OmegaInterface import RungeKuttaSolver


# ODE (continuos time) of the unicycle dynamics
sampling_period = 0.1
def unicycle_ode(x,u):
    xx_0 = u[0]*math.cos(u[1])
    xx_1 = u[0]*math.sin(u[1])
    return [xx_0, xx_1]

# Difference equation (discrete time) of the battery
def battery_level_de(x):
    battery_level = x[2]
    is_charging = (x[0] >= 6.5 and x[0] <= 8.5) and (x[1] >= 3.5 and x[1] <= 4.5)
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
def system_post(x,u_packed):

    # unpacking u
    v = 1.0/sampling_period
    if u_packed == 0:      # stay
        u = [0.0 , 0.0]
    elif u_packed == 1:    # left
        u = [v , math.pi]
    elif u_packed == 2:    # up
        u = [v , math.pi/2]
    elif u_packed == 3:    # right
        u = [v , 0.0]
    elif u_packed == 4:    # down
        u = [v , 3*math.pi/2]
    else:
        u = [0.0, 0.0]

    # compute posts
    unicycle_post = ode.RK4([x[0], x[1]], u, sampling_period)
    battery_level = battery_level_de(x)

    # emulate c-like float
    ret_0 = float(format(unicycle_post[0], '.7g'))
    ret_1 = float(format(unicycle_post[1], '.7g'))
    ret_2 = float(format(battery_level, '.7g'))

    return [ret_0, ret_1, ret_2]


if __name__ == "__main__":

    print("Discharging: ")
    x =  [0.0, 0.0, 99.0]
    u = 0
    for  k in range(101):
        old_b = x[2]
        x = system_post(x,u)
        print(str(k) + ": " + str(x) + " ("  + str(x[2] - old_b) + ")")

    print("stay and charge: ")
    x =  [7.0, 4.0, 0.0]
    u = 0
    for  k in range(7):
        old_b = x[2]
        x = system_post(x,u)
        print(str(k) + ": " + str(x) + " ("  + str(x[2] - old_b) + ")")
    

    print("left while discharging:")
    x =  [2.0, 2.0, 99.0]
    u = 1
    for  k in range(3):
        x = system_post(x,u)
        print(str(k) + ": " + str(x))

    print("up while discharging:")
    x =  [2.0, 2.0, 99.0]
    u = 2
    for  k in range(3):
        x = system_post(x,u)
        print(str(k) + ": " + str(x))

    print("right while discharging:")
    x =  [2.0, 2.0, 99.0]
    u = 3
    for  k in range(3):
        x = system_post(x,u)
        print(str(k) + ": " + str(x))

    print("down while discharging:")
    x =  [2.0, 2.0, 99.0]
    u = 4
    for  k in range(3):
        x = system_post(x,u)
        print(str(k) + ": " + str(x))