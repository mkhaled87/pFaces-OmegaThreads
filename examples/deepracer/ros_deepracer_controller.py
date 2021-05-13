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
[[[1], [247789], [40]],[[2], [247790], [40]],[[3], [247791], [40]],[[4], [247792], [40]],[[5], [247793], [59]],[[6], [247826], [36]],[[7], [247827], [36]],[[8], [247828], [36]],[[9], [247829], [36]],[[10], [247830], [57]],[[11], [247863], [36]],[[12], [247864], [36]],[[13], [247865], [36]],[[14], [247866], [59]],[[15], [247867], [51]],[[16], [247900], [36]],[[17], [247901], [36]],[[18], [247902], [59]],[[19], [247903], [51]],[[20], [247904], [47]],[[21], [247937], [36]],[[22], [247938], [36]],[[23], [247939], [57]],[[24], [247940], [47]],[[25], [247941], [38]]],
[[[26], [292967], [58]]],
[[[27], [292968], [58]]],
[[[28], [292969], [58]]],
[[[29], [292970], [41]]],
[[[30], [339518], [60]]],
[[[31], [291635], [57]]],
[[[32], [291636], [56]]],
[[[33], [291637], [56]]],
[[[34], [291638], [41]]],
[[[35], [338186], [61]]],
[[[36], [291672], [56]]],
[[[37], [291673], [54]]],
[[[38], [291674], [43]]],
[[[39], [339591], [60]]],
[[[39], [339591], [60]]],
[[[40], [291709], [54]]],
[[[41], [291710], [43]]],
[[[42], [339627], [60]]],
[[[42], [339627], [60]]],
[[[43], [338259], [61]]],
[[[44], [291746], [54]]],
[[[45], [291747], [41]]],
[[[46], [338295], [61]]],
[[[46], [338295], [61]]],
[[[47], [293119], [62]]],
[[[48], [383324], [59]]],
[[[49], [383325], [58]]],
[[[50], [383326], [51]]],
[[[30], [339518], [60]]],
[[[51], [387511], [57]]],
[[[52], [381955], [61]]],
[[[53], [380587], [62]]],
[[[54], [380588], [62]]],
[[[35], [338186], [61]]],
[[[51], [387511], [57]]],
[[[55], [380623], [61]]],
[[[56], [379255], [62]]],
[[[39], [339591], [60]]],
[[[57], [387584], [57]]],
[[[58], [379291], [61]]],
[[[42], [339627], [60]]],
[[[59], [387620], [57]]],
[[[57], [387584], [57]]],
[[[60], [379328], [60]]],
[[[46], [338295], [61]]],
[[[59], [387620], [57]]],
[[[59], [387620], [57]]],
[[[61], [431281], [58]]],
[[[62], [428507], [61]]],
[[[63], [387475], [59]]],
[[[64], [431435], [59]]],
[[[65], [432650], [59]]],
[[[66], [431245], [58]]],
[[[67], [431246], [58]]],
[[[61], [431281], [58]]],
[[[68], [429876], [59]]],
[[[69], [431508], [58]]],
[[[70], [429912], [59]]],
[[[71], [431544], [48]]],
[[[72], [428543], [61]]],
[[[73], [477944], [41]]],
[[[74], [480646], [48]]],
[[[75], [435542], [46]]],
[[[76], [479503], [48]]],
[[[77], [480754], [46]]],
[[[78], [477908], [41]]],
[[[79], [477909], [41]]],
[[[78], [477908], [41]]],
[[[80], [478171], [41]]],
[[[73], [477944], [41]]],
[[[81], [430255], [59]]],
[[[82], [480682], [38]]],
[[[83], [435695], [56]]],
[[[84], [434326], [57]]],
[[[85], [431588], [59]]],
[[[86], [433147], [51]]],
[[[81], [430255], [59]]],
[[[87], [435659], [57]]],
[[[88], [435660], [57]]],
[[[89], [435922], [48]]],
[[[90], [478287], [51]]],
[[[85], [431588], [59]]],
[[[90], [478287], [51]]],
[[[90], [478287], [51]]],
[[[91], [479656], [41]]],
[[[92], [437443], [57]]],
[[[91], [479656], [41]]],
[[[93], [479657], [41]]],
[[[94], [434742], [58]]],
[[[95], [438812], [56]]],
[[[92], [437443], [57]]],
[[[96], [481476], [53]]],
[[[97], [437444], [57]]],
[[[98], [481477], [53]]],
[[[96], [481476], [53]]],
[[[99], [444772], [44]]],
[[[98], [481477], [53]]],
[[[100], [444773], [62]]],
[[[101], [451686], [59]]],
[[[102], [454424], [50]]],
[[[103], [457082], [40]]],
[[[104], [411867], [40]]],
[[[105], [413120], [58]]],
[[[105], [413120], [58]]],
[[[106], [459476], [41]]],
[[[107], [416812], [56]]],
[[[108], [459026], [53]]],
[[[109], [421804], [44]]],
[[[72], [428543], [61]]]
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
