/*-----------------------------------------------------------------------
 * File: pickupdelivery.cl
 * Date: 01.10.2020
 * Athr: M. Khaled
 * Desc: This file gives description of the model dynamocs.
 *-----------------------------------------------------------------------*/

#define SAMPLING_PERIOD 0.1f

// we use 2 ODEs to compute the behaviour of the model starting with
// a cell in the quantized state represented by a centroid and radius
// more info: https://ieeexplore.ieee.org/document/7519063
void post_dynamics(concrete_t* xx, const concrete_t* x, const concrete_t* u);
void post_dynamics(concrete_t* xx, const concrete_t* x, const concrete_t* u) {
    xx[0] = 15.0*u[0]*((float)cos(u[1]));
    xx[1] = 15.0*u[0]*((float)sin(u[1]));
    xx[2] = 0.0f;
}
void radius_dynamics(concrete_t* rr, const concrete_t* r, const concrete_t* u);
void radius_dynamics(concrete_t* rr, const concrete_t* r, const concrete_t* u) {
    rr[0] = r[0]+fabs(u[0])*(SAMPLING_PERIOD);
    rr[1] = r[1]+fabs(u[0])*(SAMPLING_PERIOD);
    rr[2] = 0.0f;
}

// Include the RUNGE-KUTTA solver from pFaces
#include "rk4ode.cl"

// the model post computes one upper point and one lower point representing
// the set of reach set when the model starts at (x) and (u) is applied
void model_post(
    concrete_t* post_x_lb,
    concrete_t* post_x_ub,
    const concrete_t* x,
    const concrete_t* u
) {
    __private concrete_t Q[ssDim] = {ssQnt};
    __private concrete_t xx_unicycle[ssDim];
    __private concrete_t rr_unicycle[ssDim];
    __private concrete_t r_unicycle[ssDim];  
    __private char is_charging = (x[0] >= 6.0f && x[0] <= 9.0f) && (x[1] >= 0.0f && x[1] <= 3.0f);

    // solve ode of the unicycle (compute OARS)
	for (unsigned int i = 0; i<ssDim; i++)
		r_unicycle[i] = Q[i] / (concrete_t)2.0f;    
    rk4OdeSolver(xx_unicycle, x, u, 'x');
    radius_dynamics(rr_unicycle, r_unicycle, u);
    post_x_lb[0] = xx_unicycle[0] - rr_unicycle[0];
    post_x_ub[0] = xx_unicycle[0] + rr_unicycle[0];
    post_x_lb[1] = xx_unicycle[1] - rr_unicycle[1];
    post_x_ub[1] = xx_unicycle[1] + rr_unicycle[1];
    
    // change the battery level
    if (is_charging){
        post_x_lb[2] = x[2] + 20.0f;
        if (post_x_lb[2] > 99.0f)
            post_x_lb[2] = 99.0f;
    }
    else {
        post_x_lb[2] = x[2] - 1.0f;
        if (post_x_lb[2] < 0.0f)
            post_x_lb[2] = 0.0f;
    }
    post_x_ub[2] = post_x_lb[2];    
}
