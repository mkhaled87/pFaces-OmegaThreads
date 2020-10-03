
import sys
import math

# insert interface folder of the tool
sys.path.insert(1, '../../interface/python')
from OmegaInterface import RungeKuttaSolver

# system dynamics
sampling_period = 0.1
RC_charge = 0.5
RC_discharge = 5.0
def model_dynamics(x,u):
    xx_0 = u[0]*math.cos(u[1])
    xx_1 = u[0]*math.sin(u[1])

    if ((x[0] >= 6.5 and x[0] <= 8.5) and (x[1] >= 3.5 and x[1] <= 4.5)):
        xx_2 = (99.0 - x[2]) / RC_charge
    else:
        xx_2 = (-x[2]) / RC_discharge
        
    return [xx_0, xx_1, xx_2]

ode = RungeKuttaSolver(model_dynamics, 5)
if __name__ == "__main__":

    print("Discharging: ")
    state =  [0, 0, 99.0]
    for  k in range(100):
        old_b = state[2]
        state = ode.RK4(state, [0,0], sampling_period)
        print(str(k) + ": " + str(old_b) + " ("  + str(old_b - state[2]) + ")")
    
