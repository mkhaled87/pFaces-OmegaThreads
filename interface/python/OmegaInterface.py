import math
import random
import numpy


# a class to represent a hyper rectangle
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

    def is_element(self, elem):
        found = True
        for i in range(len(elem)):
            if(elem[i] < self.lb[i] or elem[i] > self.ub[i]):
                found = False

        return found

    def get_center_element(self):
        ret = []
        for i in range(len(self.lb)):
            c = (self.lb[i] + self.ub[i])/2.0
            ret.append(c)
        return ret

    def get_random_element(self):
        ret = []
        for i in range(len(self.lb)):
            r = random.uniform(self.lb[i], self.ub[i])
            ret.append(r)
        return ret  

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

# a class to represent a symbolic model
class SymbolicModel:
    def __init__(self, filename, x_symbols, u_symbols):
        self.filename = filename
        self.x_symbols = x_symbols 
        self.u_symbols = u_symbols
        self.xu_symbols = x_symbols*u_symbols
        self.xu_HR = [None] * self.xu_symbols
        
        self.load_model()

    def load_model(self):
        model_file = open(self.filename, 'r')
        model_lines = model_file.readlines()
        for line in model_lines:
            head_splitted = line.split('=>')
            xu_info = head_splitted[0].replace(" ","").replace("[","").replace("]","")
            hr_info = head_splitted[1].replace(" ","")
            
            x = int(xu_info.split(',')[0].replace("x_",""))
            u = int(xu_info.split(',')[1].replace("u_",""))
            hr = str2hyperrects(hr_info)[0]

            if(x >= self.x_symbols or u >= self.u_symbols):
                print("SymModel.load_model:: for x=" + str(x) + "/" + str(self.x_symbols) + ", u=" + str(u) + "/"+str(self.u_symbols)+": Invalid index for line: " + line.replace("\n",""))

            self.xu_HR[u + x*self.u_symbols] = hr

    def get_HR(self, x, u):
        return self.xu_HR[u + x*self.u_symbols]

# a class to represent a machine transition
class MachineTransition:
    def __init__(self, next_state, input_value, output_values):
        self.next_state = next_state 
        self.input_value = input_value
        self.output_values = output_values
    
    def get_next_state(self):
        return self.next_state

    def get_input_value(self):
        return self.input_value

    def get_output_values(self):
        return self.output_values

# a class to load the mealy/moore machine from a file and maintain it
class Machine:
    def __init__(self, filename):
        self.filename = filename
        self.semantic = 'undefined'
        self.transitions = []
        self.state_labels = []
        self.state_label_bits = -1
        self.state_label_accumulated_bits = []
        self.n_states = 0
        self.state_transitions = []
        self.load_machine(filename)

    def load_machine(self, filename):
        machine_file = open(filename, 'r')
        machine_lines = machine_file.readlines()
        self.state_transitions = []
        for line in machine_lines:
            head_data = line.split(':')
            head = head_data[0].strip()
            data = head_data[1].strip().replace('{','').replace('}','')

            if head == 'semantic':
                self.semantic = data

            elif head == 'states':
                self.n_states = int(data)
                for s in range(self.n_states):
                    self.transitions.append([])

            elif head == 'state_transitions':
                tmp_list = data.split(',')
                for elm in tmp_list:
                    if elm != '':
                        self.state_transitions.append(int(elm))

                for s in range(self.n_states):
                    for t in range(int(self.state_transitions[s])):
                        self.transitions[s].append([])

            elif head == 'state_labels':
                self.state_labels = data.split(',')

            elif head == 'state_label_accumulated_bits':
                tmp_list = data.split(',')
                for elm in tmp_list:
                    if elm != '':
                        self.state_label_accumulated_bits.append(int(elm))

            elif head == 'state_label_bits':
                self.state_label_bits = int(data)

            else:
                cleaned = head.replace('trans_', '')
                s = int(cleaned.split('_')[0])
                t = int(cleaned.split('_')[1])

                data_list = data.split(';')
                next_state = int(data_list[0])
                input_value = int(data_list[1])
                
                output_values = []
                tmp_list = data_list[2].replace('[','').replace(']','').split(',')
                for elm in tmp_list:
                    if elm != '':
                        output_values.append(int(elm))                

                trans = MachineTransition(next_state, input_value, output_values)
                self.transitions[s][t] = trans

    def get_state_stansitions(self, machine_state):
        return self.transitions[machine_state]

# a class to simulate a controller
# it maintains the current state of the machine and update it
# after each request for a new control output
class Controller:
    def __init__(self, filename, verbose = False):
        self.machine = Machine(filename)
        self.machine_state = 0
        self.verbose = verbose

    def get_control_actions(self, model_state):
        old_state = self.machine_state
        current_transitions = self.machine.get_state_stansitions(self.machine_state)
        for trans in current_transitions:
            if trans.get_input_value() == model_state:
                self.machine_state = trans.get_next_state()
                ret = trans.get_output_values()
                if self.verbose:
                    print('Controller: q: ' + str(old_state) + ', x_in: ' + str(model_state) + ', u_out: ' + str(ret) + ', q_post: ' + str(self.machine_state))
                return ret 

        raise Exception('Failed to find a control action for x=' + str(model_state) + '!')

# s class to represnet quantizers
class Quantizer:
    def __init__(self, x_lb, x_eta, x_ub):
        self.x_lb = numpy.float32(x_lb)
        self.x_eta = numpy.float32(x_eta)
        self.x_ub = numpy.float32(x_ub)
        self.x_dim = len(x_eta)
        self.x_widths = [0]*self.x_dim
        self.num_symbols = 1

        for i in range(self.x_dim):
            tmp = numpy.floor((self.x_ub[i]-self.x_lb[i])/self.x_eta[i])+1
            self.x_widths[i]  = int(tmp)
            self.num_symbols = self.num_symbols * self.x_widths[i]

    def flat_to_conc(self, flat_val):
        fltInitial = flat_val

        ret = [0.0]*self.x_dim
        for i in range(self.x_dim):
            fltCurrent = fltInitial

            fltVolume = 1
            for k in range(i):
                fltTmp = self.x_widths[k]
                fltVolume = fltVolume*fltTmp

            fltCurrent = int(fltCurrent / fltVolume)
            fltTmp = self.x_widths[i]
            fltCurrent = int(fltCurrent%fltTmp)

            ret[i] = float(self.x_lb[i] + self.x_eta[i]*fltCurrent)

            fltCurrent = fltCurrent * fltVolume
            fltInitial = fltInitial - fltCurrent

        return ret

    def conc_to_flat(self, x_c):
        x_sym = [0.0]*self.x_dim
        x_conc = numpy.float32(x_c)

        for i in range(self.x_dim):
            x_sym[i] = int(numpy.floor((x_conc[i] - self.x_lb[i] + self.x_eta[i]/numpy.float32(2.0))/self.x_eta[i]))

        
        x_flat = 0
        for i in range(self.x_dim):
            fltTmpVolume = 1

            for j in range(i):
                fltTmpVolume *= self.x_widths[j]

            fltTmp = x_sym[i]
            fltTmp *= fltTmpVolume
            x_flat += fltTmp

        return x_flat

    def get_widths(self):
        return self.x_widths

    def get_num_symbols(self):
        return self.num_symbols

# a class for Runge-Kutta solver
class RungeKuttaSolver:
    def __init__(self, dynamics, n_int):
        self.dynamics = dynamics
        self.n_int = n_int

    def RK4(self, x, u, tau):
        h = tau/self.n_int
        k = [[],[],[],[]]
        x_dim = len(x)
        tmp = [0.0]*x_dim
        x_post = x

        for _ in range(self.n_int):
            k[0] = self.dynamics(x,u)
            for i in range(x_dim):
                tmp[i] = x[i] + h/2.0*k[0][i]

            k[1] = self.dynamics(tmp, u)
            for i in range(x_dim):
                tmp[i] = x[i] + h/2.0*k[1][i]

            k[2] = self.dynamics(tmp, u)
            for i in range(x_dim):
                tmp[i] = x[i] + h*k[2][i]

            k[3] = self.dynamics(tmp, u)
            for i in range(x_dim):
                x_post[i] = x[i] + (h/6.0)*(k[0][i] + 2.0*k[1][i] + 2.0*k[2][i] + k[3][i])
        
        return x_post