
import sys
import math

# insert interface folder of the tool
sys.path.insert(1, '../../interface/python')
from Omega2dSimulator import Omega2dSimulator

# system dynamics
def model_dynamics(x,u):
    pos_x = x[0]
    pos_y = x[1]
    battery = x[2]
    input = int(u[0])
    
    if input == 0:      # stay
        pass
    elif input == 1:    # left
        pos_x = pos_x - 1.0
    elif input == 2:    # up
        pos_y = pos_y + 1.0
    elif input == 3:    # right
        pos_x = pos_x + 1.0
    elif input == 4:    # down
        pos_y = pos_y - 1.0
    else:
        pass
    
    is_charging = (x[0] >= 6.5 and x[0] <= 8.5) and (x[1] >= 3.5 and x[1] <= 4.5)
    if is_charging:        
        battery += 20.0
        if battery > 99.0:
            battery = 99.0    
    else:
        battery -= 1.0
    
    return [pos_x, pos_y, battery]

def main():
    Omega2dSimulator(
        1200,                    				# screen width
        600,                    				# screen hight
        "Pickup-Delivery Drone Example",      	# screen title
        model_dynamics,         				# dynamics function of the model
        [7.0,5.0,99.0],                         # initial state for the simulation
        0.1,        				            # sampling period to be used in computing the next states
        "pickupdelivery.cfg",          			# the config file oof the problem
        "pickupdelivery.mdf",          			# the controller file of the problem
        "drone.png",          				    # an image file to represent the model
        0.1,                       			# scale factor of the model image
        False,                                  # simulate dimension 3
        False,                                  # is model_dynamics ODE ?
        ["lowbattery", "fullbattery"]           # APs to skip from drawing
    ).start()

if __name__ == "__main__":
    main()
