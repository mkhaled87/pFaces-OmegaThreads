/*
* omega_pgame_construct.cl
*
*  date    : 15.10.2020
*  author  : M. Khaled
*  details : an OpenCL kernel function to construct parity games.
*/

// sime values to be supplied by the initialization C++ code
#define DPA_NUM_STATES @@DPA_NUM_STATES@@
#define DPA_NUM_LETTERS @@DPA_NUM_LETTERS@@
#define DPA_MAX_COLOR @@DPA_MAX_COLOR@@
#define DPA_PARITY @@DPA_PARITY@@

// some constants
#define MAX_POSTS @@MAX_POSTS@@             // the user provide this based on his knowledge of dynamics
#define NUM_STATE_APS @@NUM_STATE_APS@@
#define SYMBOLIC_MAX @@SYMBOLIC_MAX@@
#define PARITY_EVEN 0
#define PARITY_ODD 1
#define PLAYER_UNKNOWN 0
#define PLAYER_SYS 1
#define PLAYER_ENV -1 
#define NODE_BOTTOM ((SYMBOLIC_MAX) -1)
#define NODE_TOP ((SYMBOLIC_MAX) -2)
#define NODE_NONE ((SYMBOLIC_MAX) -3)

// DUMMY VALUES
#define DUMMY_U_FLAT ???
#define DUMMY_X_FLAT ???
#define DUMMY_Q_FLAT ???

// FLAGS for MODEL NODES
#define MODEL_NODE_NEW 0x01
#define MODEL_NODE_DONE 0x02
#define MODEL_NODE_NONE 0x04
#define MODEL_NODE_TOP 0x10
#define MODEL_NODE_BOTTOM 0x20


// a memory bag for DPA edge info
typedef struct __attribute__((packed)) dpa_state_edges {
    symbolic_t successors[DPA_NUM_LETTERS],
    symbolic_t colors[DPA_NUM_LETTERS]
} dpa_state_edges_t;
// getting edge successor ? all_edges_list[source_state].successor[letter]
// getting edge cs ? all_edges_list[source_state].successor[letter]


// a memory bag for data-parallel processing of any Arena model (env) node (i.e., the tuple (x,u,q))
typedef struct __attribute__((packed)) mdl_node {
    // (b4:is_bottom) || (b3:is_top) || (b2:is_none) || (b1:is_done) || (b0:is_new)
    char flags,
    symbolic_t successor_ctrl_nodes[MAX_POSTS]
} mdl_node_t;

// a memory bag for data-parallel processing of any Arena controller (sys) node (i.e., the tuple (x,u,q))
typedef struct __attribute__((packed)) ctlr_node {
    symbolic_t successor_mdl_nodes[U_SYMBOLS]
} ctlr_node_t;


__kernel void pgame_initialize(
    __global mdl_node_t* mdl_nodes,             // nodes in the domain X \times U \times Q
    __global ctlr_node_t* ctrl_nodes,           // nodes in the domain X \times U \times Q \times MAX_POSTS
    ){
    
    __private concrete_t X0_lb[ssDim] = {@@INITIAL_SET_LB@@};
    __private concrete_t X0_ub[ssDim] = {@@INITIAL_SET_UB@@};
    __private concrete_t x[ssDim];
    __private symbolic_t thread_idx_x;
	__private symbolic_t thread_idx_u;
    __private symbolic_t thread_idx_q;
	__private symbolic_t flat_thread_idx;
    __private symbolic_t xuq_flat_idx = ???;
    __private char flags = MODEL_NODE_NONE;   // all nodes start NONE
    __private bool in_X0 = true;

    thread_idx_x = UNIVERSAL_INDEX_X;
	thread_idx_u = UNIVERSAL_INDEX_Y;
    thread_idx_q = UNIVERSAL_INDEX_Z;
	flat_thread_idx = 
        thread_idx_u + 
        thread_idx_x*UNIVERSAL_WIDTH_Y
        thread_idx_q*UNIVERSAL_WIDTH_Y*UNIVERSAL_WIDTH_X;

    // get concrete value of x
    get_concrete_x(thread_idx_x, x);

    // check if x in X_0
    for (unsigned int i = 0; i<ssDim; i++){
        if(x[i] > X0_ub[i] || x[i] < X0_lb[i] ){
            in_X0 = false;
        }
    }

    // Am I (x_0, dummy_u, q_0) ? PUT me in the new nodes for the first round
    if(in_X0 && thread_idx_u == DUMMY_U_FLAT && thread_idx_q = 0)
        flags |= MODEL_NODE_NEW;

    // we choose to set (dummy,dummy,dummy) as the BUTTOM node
    if(thread_idx_x == DUMMY_X_FLAT && thread_idx_u == DUMMY_U_FLAT && thread_idx_q == DUMMY_Q_FLAT)
        flags = MODEL_NODE_BOTTOM;

    // set the flags
    mdl_nodes[flat_thread_idx].flags = flags;

    // initialize all successor_ctrl_nodes to SYMBOLIC_MAX as
    // indicatior that they aare not yet assigned
    for (unsigned int i = 0; i<MAX_POSTS; i++)
        mdl_nodes[flat_thread_idx].successor_ctrl_nodes[i] = SYMBOLIC_MAX;


    // MUST all successor_ctrl_nodes in ctrl_nodes to a SYMBOLIC_MAX
    // as indicator that they are not yet assigned
    // TODO: is this really needed ??
    for (unsigned int i = 0; i<MAX_POSTS; i++){
        ctrl_node_flat_thread_idx = ???;
        for (unsigned int u = 0; u<U_SYMBOLS; u++){
            ctrl_nodes[ctrl_node_flat_thread_idx].successor_mdl_nodes[u] = SYMBOLIC_MAX;
        }
    }

    // am i the dummy node (dummy, dummy, q_0)
    if(thread_idx_x == DUMMY_X_FLAT && thread_idx_u == DUMMY_U_FLAT && thread_idx_q == 0){
        // add transitons to new ctrl_nodes
        ???        
        // PROBLEM: num_trans is limited to MAX_POSTS which needs that num of initial states be less than this
    }    
}


__kernel void pgame_construct(
    __global uint* x_aps_bags, 				// a bit for each AP (max 32 APs): x \in AP_in[i] ?
    __global uint* u_aps_bags, 				// a bit for each AP (max 32 APs): u \in AP_out[i] ?
    __global xu_posts_t* xu_posts_bags,     // the symbolic model (LB,UB) for each (x,u)
    __global dpa_state_edges_t dpa_edges,   // the edges of the DPA
    __global mdl_node_t* mdl_nodes,         // the nodes/edges of the model
    __global ctlr_node_t* ctrl_nodes        // the nodes/edges of the controller
    ){  

    symbolic_t x = ???;
    symbolic_t u = ???;
    symbolic_t q = ???;
    symbolic_t xuq_flat_idx = ???;
    
    // should i work in this run ?
    if(mdl_nodes[xuq_flat_idx].flags.is_new == 1){

        // for all posts of (x,u)
        // IDEA: construct the symbolic model on the fly !
        // since you will have to reserve here vectors for LB/UB anyway !
        // FORGET THIS IDEA, we need to cal MAX_POSTS !
        for (symbolic_t x_post in posts(x,u)){

            // make the ctrl_node
            new_sys_node = compute its unique index based on ((x,u,q),x_post);

            // pot it as successor for current mdl_node
            mdl_nodes[xuq_flat_idx].successor_ctrl_nodes[new idx to represent x_post] = new_sys_node;
            
            // for all u \in U
            for (symbolic_t u_appy in U){

                // get letter from L_x and L_u
                letter = x_aps_bags[x_post] | (u_aps_bags[u_appy]  << NUM_STATE_APS);

                // get q_post and color from the DPA edge
                q_post = dpa_edges[q].successors[letter];
                color = dpa_edges[q].colors[letter];

                // the new control system node
                xuq_new_flat = compute it from (x_post,u_apply,q_post)

                if (! q_post is bottom){
                    if (q_post is top){
                        xuq_new_flat = flat_idx_of_top_xuq;
                    }
                    else{
                        if (!mdl_nodes[xuq_new_flat].flags.is_done){
                            mdl_nodes[xuq_new_flat].flags.is_new = 1;
                        }
                    }
                }

                if (!mdl_nodes[xuq_new_flat].flags.is_bottom){
                    ctrl_nodes[new_sys_node].successor_mdl_nodes[u_appy] = xuq_new_flat;
                }
            }
        }

        // mark myself as done
        env_nodes[xuq_flat_idx].is_new = 0;
    }
}


__kernel void pgame_check(
    __global mdl_node_t* mdl_nodes, 
    __global symbolic_t construct_done
    ){
    
    // check if there exist no node in mdl_nodes with flags.is_new
    // => construct_done

    // use reduction !
}