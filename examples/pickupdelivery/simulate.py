
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
    input = u[0]
    
    is_charging = (pos_x >= 6.5 and pos_x <= 8.5) and (pos_y >= 3.5 and pos_y <= 4.5)
    if is_charging:
        if input == 0:      # stay
            pos_x = pos_x
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
        
        battery += 1.0
    else:
        battery -= 1.0
    
    return [pos_x, pos_y, battery]

def main():
    simulator = Omega2dSimulator(
        600,                    				# screen width
        400,                    				# screen hight
        "Pickup-Delivery Drone Example",      	# screen title
        model_dynamics,         				# dynamics function of the model
        1.0,        				            # sampling period to be used in computing the next states
        "pickupdelivery.cfg",          			# the config file oof the problem
        "pickupdelivery.mdf",          			# the controller file of the problem
        "vehicle.png",          				# an image file to represent the model
        0.035,                   				# scale factor of the model image
        False,                                  # is model_dynamics ODE ?
        ["lowbattery", "fullbattery"]           # APs to skip from drawing
    )

    simulator.start()

if __name__ == "__main__":
    main()
