/*-----------------------------------------------------------------------
 * File: robot.cl
 * Date: 07.08.2020
 * Athr: M. Khaled
 * Desc: This file gives description of the model dynamocs.
 *-----------------------------------------------------------------------*/

// the model post computes one upper point and one lower point representing
// the set of reach set when the model starts at (x) and (u) is applied
void model_post(
    concrete_t* post_x_lb, 
    concrete_t* post_x_ub, 
    const concrete_t* x, 
    const concrete_t* u
) {
    __private concrete_t Q[ssDim] = {ssQnt};
    __private symbolic_t sym_u;

    // which control ?
    sym_u = trunc(u[0]);
    switch(sym_u){

        // move left
        case 0: {
            post_x_lb[0] = x[0] - Q[0];
            post_x_lb[1] = x[1];
        }
        break;

        // move up
        case 1: {
            post_x_lb[0] = x[0];
            post_x_lb[1] = x[1] + Q[1];
        }
        break;

        // move right
        case 2: {
            post_x_lb[0] = x[0] + Q[0];
            post_x_lb[1] = x[1];
        }
        break;

        // move down
        case 3: {
            post_x_lb[0] = x[0];
            post_x_lb[1] = x[1] - Q[1];
        }
        break;
        
        default:
            printf("Invalid model input !");
        break;
    }

    // the robot is deterministic
    post_x_ub[0] = post_x_lb[0];
    post_x_ub[1] = post_x_lb[1];
}

