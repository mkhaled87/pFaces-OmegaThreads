/*-----------------------------------------------------------------------
 * File: vehicle.cl
 * Date: 30.08.2020
 * Athr: M. Khaled
 * Desc: This file gives description of the model dynamocs.
 *-----------------------------------------------------------------------*/

#define SAMPLING_PERIOD 0.3f

// we use 2 ODEs to compute the behaviour of the model starting with
// a cell in the quantized state represented by a centroid and radius
// more info: https://ieeexplore.ieee.org/document/7519063
void post_dynamics(concrete_t* xx, const concrete_t* x, const concrete_t* u);
void post_dynamics(concrete_t* xx, const concrete_t* x, const concrete_t* u) {
    xx[0] = u[0]*cos(atan((float)(tan(u[1])/2.0))+x[2])/cos((float)atan((float)(tan(u[1])/2.0)));
	xx[1] = u[0]*sin(atan((float)(tan(u[1])/2.0))+x[2])/cos((float)atan((float)(tan(u[1])/2.0)));
	xx[2] = u[0]*tan(u[1]);
}
void radius_dynamics(concrete_t* rr, const concrete_t* r, const concrete_t* u);
void radius_dynamics(concrete_t* rr, const concrete_t* r, const concrete_t* u) {
	rr[0] = r[0]+(fabs((float)(u[0]*sqrt((float)(tan(u[1])*tan(u[1])/4.0+1)))))*r[2]*(SAMPLING_PERIOD);
    rr[1] = r[1]+(fabs((float)(u[0]*sqrt((float)(tan(u[1])*tan(u[1])/4.0+1)))))*r[2]*(SAMPLING_PERIOD);	
	rr[2] = r[2];
}

// Include the RUNGE-KUTTA solver from pFaces
#include "rk4ode.cl"

// the model post computes one upper point and one lower point representing
// the set of reach set when the model starts at (x) and (u) is applied
void model_post(concrete_t* post_x_lb, concrete_t* post_x_ub, const concrete_t* x, const concrete_t* u) {

    // some required vars
    concrete_t Q[ssDim] = {ssQnt};
    concrete_t xx[ssDim];
    concrete_t rr[ssDim];
    concrete_t r[ssDim];

	// initializing the radius starting values for the growth bound computation
	for (unsigned int i = 0; i<ssDim; i++)
		r[i] = Q[i] / (concrete_t)2.0f;

    // solve the ODEs
    rk4OdeSolver(xx, x, u, 'x');
    radius_dynamics(rr, r, u);

	// computing the reach-set
	for (unsigned int i = 0; i<ssDim; i++) {
		post_x_lb[i] = xx[i] - rr[i];
		post_x_ub[i] = xx[i] + rr[i];
	}    
}

