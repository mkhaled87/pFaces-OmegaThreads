#!/usr/bin/env python

# ROS Node: deepracer_controller
#
# This file is generated automatically from tool OmegaThreads
# it represents a ROS-module encapsulating a machine.
# The machine serves as a correct-by-construction controller
#
# The module listens for model state to be supplied to: deepracer_controller_in_mdl_state
# It then updates the machine state and publishes the control actions to: deepracer_controller_out_actions

import threading, queue
import rospy
from std_msgs.msg import String

# configurations
NODE_NAME = 'deepracer_controller'
SUBSCRIBER_ID = 'deepracer_controller_in_mdl_state'
PUBLISHER_ID = 'deepracer_controller_out_actions'
QUEUE_LENGTH = 10
WORKER_RATE_HZ = 10
ANONYMITY = True
PRINT_LOGS = True

# machine_data: 
# the transitions of the machine. Each line is a list of transitions for
# state with the index of the line index. Each transition has:
# 0- next state
# 1- expected input model-state
# 2- list of control inputs to be generated
machine_data = [
[[[1], [21901], [41]],[[2], [21902], [41]],[[3], [21903], [50]],[[4], [21912], [36]],[[5], [21913], [36]],[[6], [21914], [45]],[[7], [21923], [36]],[[8], [21924], [36]],[[9], [21925], [45]]],
[[[10], [25894], [59]]],
[[[11], [25895], [59]]],
[[[12], [29890], [60]]],
[[[13], [25784], [58]]],
[[[14], [25785], [57]]],
[[[15], [29780], [50]]],
[[[16], [25795], [58]]],
[[[17], [25796], [57]]],
[[[18], [29791], [49]]],
[[[19], [33882], [60]]],
[[[20], [33883], [59]]],
[[[21], [38120], [42]]],
[[[22], [33772], [59]]],
[[[23], [33651], [61]]],
[[[24], [33896], [61]]],
[[[25], [33783], [58]]],
[[[26], [33662], [59]]],
[[[27], [33786], [62]]],
[[[28], [38131], [41]]],
[[[29], [38000], [44]]],
[[[30], [38386], [51]]],
[[[31], [37889], [43]]],
[[[32], [38010], [42]]],
[[[33], [38266], [44]]],
[[[34], [37768], [44]]],
[[[34], [37768], [44]]],
[[[33], [38266], [44]]],
[[[33], [38266], [44]]],
[[[35], [38508], [41]]],
[[[36], [38663], [62]]],
[[[33], [38266], [44]]],
[[[33], [38266], [44]]],
[[[37], [38773], [42]]],
[[[33], [38266], [44]]],
[[[36], [38663], [62]]],
[[[38], [43294], [44]]],
[[[39], [39048], [43]]],
[[[40], [36042], [43]]],
[[[41], [39432], [44]]],
[[[42], [36391], [61]]],
[[[43], [36173], [61]]],
[[[44], [40833], [44]]],
[[[45], [40625], [42]]],
[[[46], [37413], [44]]],
[[[47], [36840], [62]]],
[[[31], [37889], [43]]],
[[[48], [41406], [43]]],
[[[31], [37889], [43]]]
]

# initial state of the machine is always 0
machine_state = 0

# control time index startswith 0
control_time_idx = 0

# access the machine data and update its state
def get_inputs(sym_mdl_state):
    global machine_state
    global machine_data
    found_mdl_state = False
    actions = []
    for trans in machine_data[machine_state]:
        if sym_mdl_state in trans[1]:
            found_mdl_state = True
            actions = trans[2]
            machine_state = trans[0][0]
            break

    if not found_mdl_state:
        if PRINT_LOGS:
            msg = "Failed to find control inputs for (machine_state=%s, input_mdl_state=%s)" %(machine_state, sym_mdl_state)
            rospy.loginfo(msg)

    return actions

# delivery queue
deepracer_controller_delivery = queue.Queue()

def input_callback(data):
    global control_time_idx
    global deepracer_controller_delivery
    mdl_state_symbolic = int(data.data)
    actions = get_inputs(mdl_state_symbolic)
    if actions:
        deepracer_controller_delivery.put([control_time_idx, mdl_state_symbolic, actions])

    control_time_idx += 1

def start_machine():
    rospy.init_node(NODE_NAME, anonymous=ANONYMITY)
    rospy.Subscriber(SUBSCRIBER_ID, String, input_callback)
    pub = rospy.Publisher(PUBLISHER_ID, String, queue_size=QUEUE_LENGTH)
    rate = rospy.Rate(WORKER_RATE_HZ)
    
    while not rospy.is_shutdown():
        if not deepracer_controller_delivery.empty():
            deliver = str(deepracer_controller_delivery.get())
            pub.publish(deliver)
            if PRINT_LOGS:
                delivery_log = "delivered: %s" % deliver
                rospy.loginfo(delivery_log)

        rate.sleep()

# MAIN
if __name__ == '__main__':
    start_machine()
