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
[[[1], [21901], [40]],[[2], [21902], [40]],[[3], [21903], [62]],[[4], [21912], [36]],[[5], [21913], [36]],[[6], [21914], [36]],[[7], [21923], [36]],[[8], [21924], [62]],[[9], [21925], [36]]],
[[[10], [25895], [58]]],
[[[11], [25896], [43]]],
[[[12], [30132], [56]]],
[[[13], [25785], [56]]],
[[[14], [25786], [37]]],
[[[15], [25787], [58]]],
[[[16], [25796], [54]]],
[[[17], [30153], [56]]],
[[[18], [25798], [58]]],
[[[19], [33883], [59]]],
[[[12], [30132], [56]]],
[[[20], [33896], [60]]],
[[[21], [33651], [60]]],
[[[22], [29659], [51]]],
[[[23], [33775], [62]]],
[[[24], [33530], [61]]],
[[[25], [33917], [41]]],
[[[26], [33786], [61]]],
[[[27], [38121], [41]]],
[[[28], [38266], [17]]],
[[[29], [38010], [41]]],
[[[20], [33896], [60]]],
[[[28], [38266], [17]]],
[[[30], [37999], [43]]],
[[[31], [34040], [62]]],
[[[28], [38266], [17]]],
[[[32], [38387], [15]]],
[[[33], [26662], [61]]],
[[[28], [38266], [17]]],
[[[34], [38507], [49]]],
[[[35], [38541], [61]]],
[[[33], [26662], [61]]],
[[[36], [34912], [60]]],
[[[37], [38662], [44]]],
[[[38], [43172], [44]]],
[[[39], [39300], [43]]],
[[[39], [39300], [43]]],
[[[40], [39924], [61]]],
[[[41], [39793], [43]]],
[[[42], [40625], [23]]],
[[[43], [36512], [51]]],
[[[44], [32847], [62]]],
[[[45], [36841], [43]]],
[[[46], [37292], [44]]],
[[[46], [37292], [44]]],
[[[47], [37888], [42]]],
[[[48], [38265], [60]]],
[[[49], [42656], [26]]],
[[[50], [35308], [61]]],
[[[51], [39794], [43]]],
[[[52], [36513], [59]]],
[[[53], [40713], [43]]],
[[[46], [37292], [44]]]
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
