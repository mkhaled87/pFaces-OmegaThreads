import math

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
    def __init__(self, filename):
        self.machine = Machine(filename)
        self.machine_state = 0

    def get_control_actions(self, model_state):
        current_transitions = self.machine.get_state_stansitions(self.machine_state)
        for trans in current_transitions:
            if trans.get_input_value() == model_state:
                self.machine_state = trans.get_next_state()
                return trans.get_output_values()

        raise Exception('Failed to find a control action !')

# s class to represnet quantizers
class Quantizer:
    def __init__(self, x_lb, x_eta, x_ub):
        self.x_lb = x_lb
        self.x_eta = x_eta
        self.x_ub = x_ub
        self.x_dim = len(x_eta)
        self.x_widths = [0]*self.x_dim

        for i in range(self.x_dim):
            tmp = math.floor((x_ub[i]-x_lb[i])/x_eta[i])+1
            self.x_widths[i]  = int(tmp)

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

            ret[i] = self.x_lb[i] + self.x_eta[i]*fltCurrent

            fltCurrent = fltCurrent * fltVolume
            fltInitial = fltInitial - fltCurrent

        return ret

    def conc_to_flat(self, x_conc):
        x_sym = [0.0]*self.x_dim


        for i in range(self.x_dim):
            x_sym[i] = math.floor((x_conc[i] - self.x_lb[i] + self.x_eta[i]/2.0)/self.x_eta[i])

        
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