import arcade
import math
import sys
import os

from OmegaInterface import Controller
from OmegaInterface import Quantizer
from OmegaInterface import RungeKuttaSolver

# insert interace folder of pFaces
if 'PFACES_SDK_ROOT' in os.environ:
    pfaces_interface_path = sys.path.insert(1, os.environ['PFACES_SDK_ROOT'] + "../interface/python")
from ConfigReader import ConfigReader

def list2str(lnList):
    length = len(lnList)
    ret = ""
    for i in range(length):
        ret += str(round(lnList[i]*100)/100)
        if i<length-1:
            ret += ", "

    return ret

def str2list(strList):
    out = []
    lst = strList.split(',')
    for s in lst:
        out.append(float(s.replace(' ', "")))
    
    return out

class HyperRect:
    def __init__(self, lb, ub):
        self.lb = lb
        self.ub = ub

    def get_lb(self):
        return self.lb

    def get_ub(self):
        return self.ub

    def set_lb(self, lb):
        self.lb = lb

    def set_ub(self, ub):
        self.ub = ub


def str2hyperrects(strHRs):
    hyperrects =[]
    str_HRs = strHRs.replace(" ", "")
    str_HRs = str_HRs.split("U")
    for str_HR in str_HRs:
        HR_lb = []
        HR_ub = []
        str_Intervals = str_HR.split("x")
        for str_Interval in str_Intervals:
            HR_elms = str_Interval.replace("[","").replace("]","").split(",")
            HR_lb.append(float(HR_elms[0]))
            HR_ub.append(float(HR_elms[1]))

        hr = HyperRect(HR_lb, HR_ub)
        hyperrects.append(hr)

    return hyperrects

COLORS = [
    arcade.color.AMAZON, 
    arcade.color.AZURE,
    arcade.color.BITTERSWEET,
    arcade.color.BUD_GREEN,
    arcade.color.BYZANTINE,
    arcade.color.CANARY_YELLOW,
    arcade.color.CARDINAL,
    arcade.color.CHARCOAL,
    arcade.color.CHESTNUT,
    arcade.color.CORAL
]


class Omega2dSimulator(arcade.Window):

    def __init__(self, width, height, title, sys_dynamics_func, initial_state, sampling_period, config_file, controller_file, model_image, model_image_scale, visualize_3rd_dim = True, use_ODE = True, skip_aps = []):
        super().__init__(width, height, "Omega2dSimulator: " + title)

        # local storage
        self.SCREEN_WIDTH = width
        self.SCREEN_HEIGHT = height
        self.step_time = sampling_period
        self.sys_dynamics = sys_dynamics_func
        self.skip_aps = skip_aps
        self.use_ODE = use_ODE
        self.visualize_3rd_dim = visualize_3rd_dim
        self.x_0 = initial_state
        self.sys_state = self.x_0

        # create the controller object
        self.controller = Controller(controller_file)

        # crete the system sprite
        self.system = arcade.Sprite(model_image, model_image_scale)

        # load the configurations
        self.load_configs(config_file)
        self.last_action = [0.0]*len(self.x_lb)
        self.last_action_symbol = -1

        # prepare for simulation
        self.sys_status = "stopped"
        self.initial_delay = 100
        self.time_elapsed = 0.0
        self.avg_delta = 0.0
        arcade.set_background_color(arcade.color.WHITE)

    def load_configs(self, config_file):
        # load some values form the config file
        self.config_reader = ConfigReader(config_file)
        self.x_lb = str2list(self.config_reader.get_value_string("system.states.first_symbol"))
        self.x_ub = str2list(self.config_reader.get_value_string("system.states.last_symbol"))
        self.x_eta = str2list(self.config_reader.get_value_string("system.states.quantizers"))
        self.u_lb = str2list(self.config_reader.get_value_string("system.controls.first_symbol"))
        self.u_ub = str2list(self.config_reader.get_value_string("system.controls.last_symbol"))
        self.u_eta = str2list(self.config_reader.get_value_string("system.controls.quantizers"))
        self.specs_formula = self.config_reader.get_value_string("specifications.ltl_formula")

        # create a quantizer and an ode solver
        self.qnt_x = Quantizer(self.x_lb, self.x_eta, self.x_ub)
        self.qnt_u = Quantizer(self.u_lb, self.u_eta, self.u_ub)
        if self.use_ODE:
            self.ode = RungeKuttaSolver(self.sys_dynamics, 5)

        # configs for the arena
        self.PADDING = 40
        self.ZERO_BASE_X = self.PADDING
        self.ZERO_BASE_Y = self.PADDING
        self.X_GRID = (self.SCREEN_WIDTH-2*self.PADDING)/self.qnt_x.get_widths()[0]
        self.Y_GRID = (self.SCREEN_HEIGHT-2*self.PADDING)/self.qnt_x.get_widths()[1]
        self.ARENA_WIDTH = (self.qnt_x.get_widths()[0])*self.X_GRID
        self.ARENA_HIGHT = (self.qnt_x.get_widths()[1])*self.Y_GRID
        self.X_SCALE_FACTOR = self.ARENA_WIDTH/(self.x_ub[0] - self.x_lb[0] + self.x_eta[0])
        self.Y_SCALE_FACTOR = self.ARENA_HIGHT/(self.x_ub[1] - self.x_lb[1] + self.x_eta[1])
        self.Z_SCALE_FACTOR = 180.0/math.pi # from rad to deg   
        self.arena_mdl_lb = self.translate_sys_to_arena(self.x_lb)
        self.arena_mdl_ub = self.translate_sys_to_arena(self.x_ub)     

        # set system's initial state
        state_arena = self.translate_sys_to_arena(self.x_0)
        self.system.center_x = state_arena[0]
        self.system.center_y = state_arena[1]
        if len(self.x_lb) == 3 and self.visualize_3rd_dim:
            self.system.angle =  state_arena[2]

        # others: subsets
        self.subset_names = self.config_reader.get_value_string("system.states.subsets.names").replace(" ","").split(",")
        self.subset_HRs = []
        for subset_name in self.subset_names:
            skip = False
            for skip_ap in self.skip_aps:
                if skip_ap == subset_name:
                    skip = True
            if skip:
                continue
            subset_str = self.config_reader.get_value_string("system.states.subsets.mapping_" + subset_name)
            if subset_str  == '':
                continue
            HRs = str2hyperrects(subset_str)
            HRs_modified = []
            for HR in HRs:
                conc_lb = HR.get_lb()
                conc_ub = HR.get_ub()

                for i in range(len(self.x_lb)):
                    if conc_lb[i] < self.x_lb[i]:
                        conc_lb[i] = self.x_lb[i] - self.x_eta[i]/2
                    if conc_ub[i] > self.x_ub[i]:
                        conc_ub[i] = self.x_ub[i] + self.x_eta[i]/2

                lb = self.translate_sys_to_arena(conc_lb)
                ub = self.translate_sys_to_arena(conc_ub)

                HR.set_lb(lb)
                HR.set_ub(ub)
                HRs_modified.append(HR)

            self.subset_HRs.append(HRs_modified)
        
    def draw_subsets(self):
        idx = 0
        for subset_hyperrects in self.subset_HRs:
            for HR in subset_hyperrects:
                lb = HR.get_lb()
                ub = HR.get_ub()
                width = ub[0] - lb[0]
                hight = ub[1] - lb[1]
                arcade.draw_rectangle_filled(lb[0] + width/2, lb[1] + hight/2, width, hight, COLORS[idx % len(COLORS)])
                arcade.draw_text(self.subset_names[idx], lb[0] + 5, lb[1] + 5, arcade.color.BLACK, 16)

            idx += 1
            

        pass

    def draw_arena(self):
        
        # Draw the background
        arcade.draw_rectangle_filled(self.ZERO_BASE_X + self.ARENA_WIDTH/2, self.ZERO_BASE_Y + self.ARENA_HIGHT/2, self.ARENA_WIDTH, self.ARENA_HIGHT, arcade.color.LIGHT_GRAY)
        
        # Draw the subsets
        self.draw_subsets()
        
        # draw grid
        num_x_lines = self.qnt_x.get_widths()[0] + 1
        for i in range(num_x_lines):
            arcade.draw_line(self.ZERO_BASE_X+i*self.X_GRID, self.ZERO_BASE_Y, self.ZERO_BASE_X+i*self.X_GRID, self.ZERO_BASE_Y+self.ARENA_HIGHT, arcade.color.BLACK)
        num_y_lines = self.qnt_x.get_widths()[1] + 1
        for i in range(num_y_lines):
            arcade.draw_line(self.ZERO_BASE_X, self.ZERO_BASE_Y+i*self.Y_GRID, self.ZERO_BASE_X+self.ARENA_WIDTH, self.ZERO_BASE_Y+i*self.Y_GRID, arcade.color.BLACK)        

        # info
        arcade.draw_text("Spec.: " + self.specs_formula, self.ZERO_BASE_X, self.ZERO_BASE_Y + self.ARENA_HIGHT + 15, arcade.color.BLACK, 12)
        
        # draw lb/ub markers
        arcade.draw_rectangle_filled(self.arena_mdl_lb[0], self.arena_mdl_lb[1], 5, 5, arcade.color.BLUE)
        arcade.draw_rectangle_filled(self.arena_mdl_ub[0], self.arena_mdl_ub[1], 5, 5, arcade.color.BLUE)
        arcade.draw_text("x_first", self.arena_mdl_lb[0] - 15, self.arena_mdl_lb[1] - 17, arcade.color.BLACK, 14)
        arcade.draw_text("x_last", self.arena_mdl_ub[0] - 15, self.arena_mdl_ub[1] - 17, arcade.color.BLACK, 14)

    def start(self):
        arcade.run()

    def get_current_symbol(self):
        return self.qnt_x.conc_to_flat(self.sys_state)
    
    def print_info(self):
        txt  = "Status: " + self.sys_status
        txt += " | Time (sec.): " + str(round(self.time_elapsed))
        txt += " | FPS: " + str(round(1/self.avg_delta))
        txt += "\nState: x_" + str(self.get_current_symbol()) + " = (" + list2str(self.sys_state) + ")"
        txt += " | Control action: "
        
        if self.last_action_symbol == -1:
            txt += "not_issued"
        else:
            txt += "u_" + str(self.last_action_symbol) + " (" + list2str(self.last_action) + ")"

        arcade.draw_text(txt, self.ZERO_BASE_X, self.ZERO_BASE_Y - 35, arcade.color.BLACK, 10)

    def on_draw(self):
        arcade.start_render()
        self.draw_arena()
        self.system.draw()
        self.print_info()

    def translate_sys_to_arena(self, state):
        arena_x = self.ZERO_BASE_X + (state[0] - self.x_lb[0] + self.x_eta[0]/2)*self.X_SCALE_FACTOR
        arena_y = self.ZERO_BASE_Y + (state[1] - self.x_lb[1] + self.x_eta[1]/2)*self.Y_SCALE_FACTOR
        if len(state) == 3:
            arena_t = state[2]*self.Z_SCALE_FACTOR
            return [arena_x, arena_y, arena_t]
        else:
            return [arena_x, arena_y]

    def update_system(self):
        if self.sys_status == "stopped":
            sym_state = self.get_current_symbol()
            actions_list = self.controller.get_control_actions(sym_state)
            self.last_action_symbol = actions_list[0]
            self.last_action = self.qnt_u.flat_to_conc(self.last_action_symbol)
            self.sub_steps = 0
            self.steps_in_tau = self.step_time/self.avg_delta
            self.sys_status = "moving"

        # solve the ode for one delta
        if self.use_ODE:
            self.sys_state = self.ode.RK4(self.sys_state, self.last_action, self.avg_delta)
        else:
            if self.sub_steps == 0:
                self.sys_state = self.sys_dynamics(self.sys_state, self.last_action)
        self.sub_steps += 1

        # set state
        state_arena = self.translate_sys_to_arena(self.sys_state)
        self.system.center_x = state_arena[0]
        self.system.center_y = state_arena[1]
        if len(self.sys_state) == 3 and self.visualize_3rd_dim:
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
