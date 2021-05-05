#!/usr/bin/env python

# ROS Node: ##NODE_NAME##
#
# This file is generated automatically from tool OmegaThreads
# it represents a ROS-module encapsulating a machine.
# The machine serves as a correct-by-construction controller
#
# The module listens for model state to be supplied to: ##NODE_NAME##_in_mdl_state
# It then updates the machine state and publishes the control actions to: ##NODE_NAME##_out_actions

import threading, queue
import rospy
from std_msgs.msg import String

# configurations
NODE_NAME = '##NODE_NAME##'
SUBSCRIBER_ID = '##NODE_NAME##_in_mdl_state'
PUBLISHER_ID = '##NODE_NAME##_out_actions'
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
##MACHINE_DATA##
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
##NODE_NAME##_delivery = queue.Queue()

def input_callback(data):
    global control_time_idx
    global ##NODE_NAME##_delivery
    mdl_state_symbolic = int(data.data)
    actions = get_inputs(mdl_state_symbolic)
    if actions:
        ##NODE_NAME##_delivery.put([control_time_idx, mdl_state_symbolic, actions])

    control_time_idx += 1

def start_machine():
    rospy.init_node(NODE_NAME, anonymous=ANONYMITY)
    rospy.Subscriber(SUBSCRIBER_ID, String, input_callback)
    pub = rospy.Publisher(PUBLISHER_ID, String, queue_size=QUEUE_LENGTH)
    rate = rospy.Rate(WORKER_RATE_HZ)
    
    while not rospy.is_shutdown():
        if not ##NODE_NAME##_delivery.empty():
            deliver = str(##NODE_NAME##_delivery.get())
            pub.publish(deliver)
            if PRINT_LOGS:
                delivery_log = "delivered: %s" % deliver
                rospy.loginfo(delivery_log)

        rate.sleep()

# MAIN
if __name__ == '__main__':
    start_machine()
