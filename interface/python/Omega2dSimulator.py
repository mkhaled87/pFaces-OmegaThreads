import arcade
import math
import sys
import os

# insert interace folder of pFaces
if 'PFACES_SDK_ROOT' in os.environ:
    pfaces_interface_path = sys.path.insert(1, os.environ['PFACES_SDK_ROOT'] + "/../interface/python")
else:
    raise Exception('pFaces is not installed correctly.')

from ConfigReader import ConfigReader
from OmegaInterface import Controller
from OmegaInterface import Quantizer
from OmegaInterface import RungeKuttaSolver
from OmegaInterface import HyperRect
from OmegaInterface import str2hyperrects
from OmegaInterface import SymbolicModel

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

def shift_and_add(lst, itm):
    out = lst
    lst_len = len(lst)
    
    if lst_len == 0:
        return out

    for i in range(lst_len-1):
        lst[i] = lst[i+1]
    lst[len(lst)-1] = itm

    return out


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

    def __init__(self, sys_dynamics_func, config_file, after_draw=None, after_post=None):

        # local storage of callback functions
        self.sys_dynamics = sys_dynamics_func
        self.after_draw = after_draw
        self.after_post = after_post

        # load the configurations
        print('Loading the configuration from ' + config_file + ' ...')
        self.load_configs(config_file)

        # create a symbolic model if needed
        if(self.use_model_dump):
            print('Loading the symbolic model from ' + self.model_dump_file + ' (may take long time) ...')
            self.sym_model = SymbolicModel(self.model_dump_file, self.qnt_x.get_num_symbols(), self.qnt_u.get_num_symbols())

        # create the controller object
        print('Loading the controller from ' + self.controller_file + ' ...')
        self.controller = Controller(self.controller_file)

        # initialize the arcade thing
        print('Initializing the Arcade simulation ...')
        super().__init__(self.SCREEN_WIDTH, self.SCREEN_HEIGHT, "Omega2dSimulator: " + self.title)
        
        # crete the system sprite
        self.system = arcade.Sprite(self.model_image, self.model_image_scale)
        self.last_action = [0.0]*len(self.x_lb)
        self.last_action_symbol = -1

        # set system's initial state in arrena
        x_0_arena = self.translate_sys_to_arena(self.x_0)
        self.system.center_x = x_0_arena[0]
        self.system.center_y = x_0_arena[1]
        if len(self.x_lb) >= 3 and self.visualize_3rd_dim:
            self.system.angle =  x_0_arena[2]

        # prepare for simulation
        self.sys_state = self.x_0
        self.sys_status = "stopped"
        self.initial_delay = 100
        self.time_elapsed = 0.0
        self.avg_delta = 0.0
        self.total_sim_time = 0.0
        arcade.set_background_color(arcade.color.WHITE)
        self.path_tail = [None]*self.path_tail_length

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
        self.X_initial = self.config_reader.get_value_string("system.states.initial_set")
        self.X_initial_HR = str2hyperrects(self.X_initial)[0]

        self.SCREEN_WIDTH = int(self.config_reader.get_value_string("simulation.window_width"))
        self.SCREEN_HEIGHT = int(self.config_reader.get_value_string("simulation.window_height"))
        self.title = self.config_reader.get_value_string("simulation.widow_title")
        self.step_time = float(self.config_reader.get_value_string("system.dynamics.step_time"))
        self.skip_aps = self.config_reader.get_value_string("simulation.skip_APs").replace(" ", "").split(",")
        self.visualize_3rd_dim = ( "true" == self.config_reader.get_value_string("simulation.visualize_3rdDim"))
        self.model_image = self.config_reader.get_value_string("simulation.system_image")
        self.model_image_scale = float(self.config_reader.get_value_string("simulation.system_image_scale"))
        self.controller_file = self.config_reader.get_value_string("simulation.controller_file")
        self.use_ODE = ( "true" == self.config_reader.get_value_string("simulation.use_ode"))

        strTmp = self.config_reader.get_value_string("simulation.path_tail_length")
        if strTmp == "" or strTmp == None:
            self.path_tail_length = 0
        else:    
            self.path_tail_length = int(strTmp)

        self.model_dump_file = self.config_reader.get_value_string("simulation.model_dump_file")
        if(self.model_dump_file == "" or self.model_dump_file == None):
            self.use_model_dump = False
        else:      
            self.use_model_dump = True

        self.x_0_str = self.config_reader.get_value_string("simulation.initial_state")
        if self.x_0_str == "center":
            self.x_0 = self.X_initial_HR.get_center_element()
        elif self.x_0_str == "random":
            self.x_0 = self.X_initial_HR.get_random_element()
        else:
            self.x_0 = str2list(self.x_0_str)

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

        # draw tail
        for line in self.path_tail:
            if line != None:
                arcade.draw_line(line[0][0], line[0][1], line[1][0], line[1][1], arcade.color.BLUE)

        if self.after_draw is not None:
            self.after_draw(arcade)

    def translate_sys_to_arena(self, state):
        arena_x = self.ZERO_BASE_X + (state[0] - self.x_lb[0] + self.x_eta[0]/2)*self.X_SCALE_FACTOR
        arena_y = self.ZERO_BASE_Y + (state[1] - self.x_lb[1] + self.x_eta[1]/2)*self.Y_SCALE_FACTOR
        if len(state) >= 3:
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
            self.sys_state_step_0 = self.sys_state[:]
            self.steps_in_tau = math.floor(self.step_time/self.avg_delta) - 1
            self.sys_status = "moving"

        # solve the ode for one delta
        if self.use_ODE:
            sim_time = self.avg_delta
            if self.sub_steps == 0:
                self.total_sim_time = 0.0
                left_time = self.step_time - self.steps_in_tau*self.avg_delta
                sim_time += left_time

            if self.sub_steps == self.steps_in_tau:
                sim_time = self.step_time - self.total_sim_time

            if sim_time > 0:
                self.sys_state = self.ode.RK4(self.sys_state, self.last_action, sim_time)
                self.total_sim_time += sim_time
        else:
            if self.sub_steps == 0:
                self.sys_state = self.sys_dynamics(self.sys_state, self.last_action)

        if self.after_post is not None:
            self.sys_state = self.after_post(self.sys_state)

        # increment the sub-step
        self.sub_steps += 1

        # check the post state against the one stored in the model
        if self.use_model_dump:
            if self.last_action_symbol != -1 and self.sub_steps >= self.steps_in_tau:
                x_flat = self.qnt_x.conc_to_flat(self.sys_state_step_0)
                u_flat = self.last_action_symbol
                x_post_HR = self.sym_model.get_HR(x_flat, u_flat)
                
                if not x_post_HR.is_element(self.sys_state):
                    print("Warning: Simulation is unstable due to variations in CPU load causing FPS to affect exact step-time OR the supplied dynamics does not conform with the constructed symbolic model (please double-check!) for x_flat=" + str(x_flat) + ", u_flat=" + str(u_flat) + ". Dynamics report x_post=" + str(self.sys_state) + " which is not inside the post_HR=(lb:" + str(x_post_HR.get_lb()) + ",ub:" + str(x_post_HR.get_ub()) + "). As a repair measure, x_post will be replaced with a value inside post_HR.")
                    self.sys_state = x_post_HR.get_center_element()


        # add to tail
        if self.sub_steps >= self.steps_in_tau:
            start_arena = self.translate_sys_to_arena(self.sys_state_step_0)
            end_arena = self.translate_sys_to_arena(self.sys_state)
            self.path_tail = shift_and_add(self.path_tail,[start_arena,end_arena])

        # set state
        state_arena = self.translate_sys_to_arena(self.sys_state)
        self.system.center_x = state_arena[0]
        self.system.center_y = state_arena[1]
        if len(self.sys_state) >= 3 and self.visualize_3rd_dim:
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
