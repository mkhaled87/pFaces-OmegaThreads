import arcade
import math
import sys

# insert at interface folder
sys.path.insert(1, '../../interface/python')
from OmegaInterface import Controller
from OmegaInterface import Quantizer
from OmegaInterface import RungeKuttaSolver


# system information as in the config file
x_lb = [0.5,0.5,-3.2]
x_ub = [9.5,9.5,3.2]
x_eta = [0.2,0.2,0.2]
x_0 = [0.5,1.5,1.5]
u_lb = [-2.0,-1.0]
u_ub = [2.0,1.0]
u_eta = [0.4,0.2]
sampling_period = 0.3
def sys_dynamics(x,u):
    xx_0 = u[0]*math.cos(math.atan((math.tan(u[1])/2.0))+x[2])/math.cos(math.atan((math.tan(u[1])/2.0)))
    xx_1 = u[0]*math.sin(math.atan((math.tan(u[1])/2.0))+x[2])/math.cos(math.atan((math.tan(u[1])/2.0)))
    xx_2 = u[0]*math.tan(u[1])
    return [xx_0, xx_1, xx_2]


# create a quantizer and an ode solver
qnt_x = Quantizer(x_lb, x_eta, x_ub)
qnt_u = Quantizer(u_lb, u_eta, u_ub)
ode = RungeKuttaSolver(sys_dynamics, 5)

# window settings
SCREEN_WIDTH = 600
SCREEN_HEIGHT = 600

# info for the arena
ZERO_BASE_X = 50
ZERO_BASE_Y = 50
PADDING = 50
X_GRID = (SCREEN_WIDTH-2*PADDING)/qnt_x.get_widths()[0]
Y_GRID = (SCREEN_HEIGHT-2*PADDING)/qnt_x.get_widths()[1]
ARENA_WIDTH = (qnt_x.get_widths()[0])*X_GRID
ARENA_HIGHT = (qnt_x.get_widths()[1])*X_GRID


X_SCALE_FACTOR = 100.0 # from ZERO_BASE_X/x_lb
Y_SCALE_FACTOR = 100.0 # from ZERO_BASE_Y/y_lb
T_SCALE_FACTOR = 180.0/math.pi # from rad to deg
def translate_sys_to_arena(state):
    arena_x = state[0]*X_SCALE_FACTOR
    arena_y = state[1]*X_SCALE_FACTOR
    arena_t = state[2]*T_SCALE_FACTOR
    return [arena_x, arena_y, arena_t]


def str_list(lnList):
    length = len(lnList)
    ret = ""
    for i in range(length):
        ret += str(round(lnList[i]*100)/100)
        if i<length-1:
            ret += ", "

    return ret

def draw_arena():
    # Draw the bounds
    arcade.draw_rectangle_filled(ZERO_BASE_X + ARENA_WIDTH/2, ZERO_BASE_Y + ARENA_HIGHT/2, ARENA_WIDTH, ARENA_HIGHT, arcade.color.DARK_GRAY)
    arcade.draw_rectangle_outline(ZERO_BASE_X + ARENA_WIDTH/2, ZERO_BASE_Y + ARENA_HIGHT/2, ARENA_WIDTH, ARENA_HIGHT, arcade.color.BLACK)

    # draw grid
    num_y_lines = int(ARENA_HIGHT/Y_GRID)
    for i in range(1,num_y_lines):
        arcade.draw_line(ZERO_BASE_X, ZERO_BASE_Y+i*Y_GRID, ZERO_BASE_X+ARENA_WIDTH, ZERO_BASE_Y+i*Y_GRID, arcade.color.BLACK)
    num_x_lines = int(ARENA_WIDTH/X_GRID)
    for i in range(1,num_x_lines):
        arcade.draw_line(ZERO_BASE_X+i*X_GRID, ZERO_BASE_Y, ZERO_BASE_X+i*X_GRID, ZERO_BASE_Y+ARENA_HIGHT, arcade.color.BLACK)

    # info
    arcade.draw_text('Specification: FG(target) & G(!avoids)', ZERO_BASE_X, ZERO_BASE_Y + ARENA_HIGHT + 5, arcade.color.BLACK, 14)
    arcade.draw_text('target', ZERO_BASE_X + ARENA_WIDTH - 40, ZERO_BASE_Y + ARENA_HIGHT + 20, arcade.color.BLACK, 10)
    arcade.draw_rectangle_filled(ZERO_BASE_X + ARENA_WIDTH - 51, ZERO_BASE_Y + ARENA_HIGHT + 26, 10, 10, arcade. color.AVOCADO)
    arcade.draw_text('avoids', ZERO_BASE_X + ARENA_WIDTH - 40, ZERO_BASE_Y + ARENA_HIGHT + 5, arcade.color.BLACK, 10)
    arcade.draw_rectangle_filled(ZERO_BASE_X + ARENA_WIDTH - 51, ZERO_BASE_Y + ARENA_HIGHT + 11, 10, 10, arcade.color.BARN_RED)

class Omega2dSimulator(arcade.Window):

    def __init__(self, width, height, title):
        super().__init__(width, height, title)
        arcade.set_background_color(arcade.color.WHITE)

    def setup(self):
        self.sys_status = "stopped"
        self.initial_delay = 100
        self.time_elapsed = 0.0
        self.avg_delta = 0.0
        self.step_time = sampling_period
        self.sys_state = x_0
        self.last_action = [0.0]*len(x_lb)
        self.system = arcade.Sprite("vehicle.png", 0.035)
        self.controller = Controller("vehicle_2targets.mdf")

    def get_current_symbol(self):
        return qnt_x.conc_to_flat(self.sys_state)
    
    def print_info(self):
        txt  = "Status: " + self.sys_status
        txt += " | Time (sec.): " + str(round(self.time_elapsed))
        txt += " | FPS: " + str(round(1/self.avg_delta))
        #txt += "\nCurrent: x_" + str(self.get_current_symbol()) + " = (" + str_list(self.sys_state) + ")"
        txt += " | Current action: (" + str_list(self.last_action) + ")"
        arcade.draw_text(txt, 50, 6, arcade.color.BLACK, 10)

    def on_draw(self):
        arcade.start_render()
        draw_arena()
        self.system.draw()
        self.print_info()

    def update_system(self):
        if self.sys_status == "stopped":
            
            sym_state = self.get_current_symbol()
            print("controller got: " + str(sym_state) + " = " + str_list(self.sys_state))
            actions_list = self.controller.get_control_actions(sym_state)
            print("controller sent: " + str(actions_list[1]))
            self.last_action = qnt_u.flat_to_conc(actions_list[1])
            print("u_to_apply: " + str_list(self.last_action))
            #self.last_action = [1.0, 0.0]
            self.sub_steps = 0
            self.steps_in_tau = self.step_time/self.avg_delta
            self.sys_status = "moving"

        # solve the ode for one delta
        self.sys_state = ode.RK4(self.sys_state, self.last_action, self.avg_delta)
        self.sub_steps += 1

        # set state
        state_arena = translate_sys_to_arena(self.sys_state)
        self.system.center_x = state_arena[0]
        self.system.center_y = state_arena[1]
        self.system.angle =  state_arena[2]

        # if number of delta_steps_in_tau is reached, time to stop
        # and wait for new input
        if self.sub_steps >= self.steps_in_tau:
            self.sys_status = "stopped"

    def update(self, delta_time):
    
        if self.avg_delta == 0.0 or self.initial_delay > 0:
            if self.avg_delta == 0.0:
                self.avg_delta = delta_time
            self.initial_delay -= 1
        else:
            self.update_system()
            
        self.time_elapsed += delta_time
        self.avg_delta = (self.avg_delta + delta_time)/2.0

def main():
    game = Omega2dSimulator(SCREEN_WIDTH, SCREEN_HEIGHT, "Vehicle Example")
    game.setup()
    arcade.run()

if __name__ == "__main__":
    main()