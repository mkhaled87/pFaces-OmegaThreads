import sys
import math

# insert interface folder of the tool
sys.path.insert(1, '../../interface/python')
from Omega2dSimulator import Omega2dSimulator

def wrapToPi(rad):
    M_PI = math.pi
    M_2PI = 2*math.pi
    ret = rad
    ret -= M_2PI * math.floor((ret + M_PI) * (1.0/M_2PI))
    return ret

# system model (helper-)functions
def map_steering(angle_in):
    p1 = -0.116
    p2 = -0.02581
    p3 = 0.3895
    p4 = 0.02972
    x = angle_in
    return p1*x*x*x + p2*x*x + p3*x + p4

def map_speed(speed_in):
    if speed_in == 6: 
        return  0.70
    elif speed_in == 5: 
        return  0.65
    elif speed_in == 4: 
        return  0.60
    elif speed_in == 3:
        return  0.55
    elif speed_in == 2: 
        return  0.50
    elif speed_in == 1: 
        return  0.45
    elif speed_in == 0: 
        return  0.00
    elif speed_in == -1: 
        return -0.45
    elif speed_in == -2: 
        return -0.50
    elif speed_in == -3: 
        return -0.55
    elif speed_in == -4: 
        return -0.60
    elif speed_in == -5: 
        return -0.65
    elif speed_in == -6: 
        return -0.70
    else:
	    return 0.0

def get_v_params(u_speed):
    if u_speed == 0.0:
        K = 1.5
        T = 0.25
    elif u_speed == 0.45:
        K = 1.9353 
        T = 0.9092
    elif u_speed == 0.50:
        K = 2.2458 
        T = 0.8942
    elif u_speed == 0.55:
        K = 2.8922 
        T = 0.8508
    elif u_speed == 0.60:
        K = 3.0332 
        T = 0.8642
    elif u_speed == 0.65:
        K = 3.1274 
        T = 0.8419
    elif u_speed == 0.70:
        K = 3.7112 
        T = 0.8822
    elif u_speed == -0.45:
        K = 1.6392 
        T = 1.3008
    elif u_speed == -0.50:
        K = 2.5998 
        T = 0.9882
    elif u_speed == -0.55:
        K = 2.8032 
        T = 0.9640
    elif u_speed == -0.60:
        K = 3.1457 
        T = 0.9741
    elif u_speed == -0.65:
        K = 3.6170 
        T = 0.9481
    elif u_speed == -0.70:
        K = 3.6391 
        T = 0.9285
    else:
        print("get_v_params: Invalid input !")
        K = 0.0
        T = 0.0

    a = -1.0/T
    b = K/T
    return [a, b]

def model_ode(x,u):
    u_steer = map_steering(u[0])
    u_speed = map_speed(u[1])
    L = 0.165
    ab = get_v_params(u_speed)
    a = ab[0]
    b = ab[1]

    xx_0 = x[3]*math.cos(x[2])
    xx_1 = x[3]*math.sin(x[2])
    xx_2 = (x[3]/L)*math.tan(u_steer)
    xx_3 = a*x[3] + b*u_speed
    ret = [xx_0, xx_1, xx_2, xx_3]

    return ret

# this function gets called after drawing each frame in tthe simulation
# we use it to send the drawn image to the arena projectors
def after_draw(arcade):
    return 0

# this functin gets called after the simulator solves the ode and computes the post
# we use it to update the post. moost specifically, wrap the angle in x[2] to [-pi,pi]
def after_post(x):
    return [x[0], x[1], wrapToPi(x[2]), x[3]]

# start the simulation as main function
if __name__ == "__main__":
    if len(sys.argv) <= 1:
        print('Error: you must pass the config file as first argument.')
    else:
        cfg_file = sys.argv[1]
        print('Starting the simulator with the config file: ' + cfg_file)
        Omega2dSimulator(
            model_ode,
            cfg_file,
            after_draw,
            after_post
        ).start()
