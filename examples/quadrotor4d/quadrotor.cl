/*-----------------------------------------------------------------------
 * File: quadrotor.cl
 * Date: 23.05.2021
 * Athr: B. Zhong
 * Desc: This file gives description of the model dynamocs.
 *-----------------------------------------------------------------------*/

// sampling period for the vehilce dynamics
#define SAMPLING_PERIOD STEP_TIME

// post dynamics (ODE)
void post_dynamics(concrete_t* xx, const concrete_t* x, const concrete_t* u);
void post_dynamics(concrete_t* xx, const concrete_t* x, const concrete_t* u) {
    xx[0] = x[1];
    xx[1] = 9.8f*u[0];
    xx[2] = x[3];
    xx[3] = 9.8f*u[1];
}

// radius dynamics (difference equation)
void radius_dynamics(concrete_t* rr, const concrete_t* r, const concrete_t* u);
void radius_dynamics(concrete_t* rr, const concrete_t* r, const concrete_t* u) {
	rr[0] = r[0]+(SAMPLING_PERIOD)*r[1];
    rr[1] = r[1];
    rr[2] = r[2]+(SAMPLING_PERIOD)*r[3];
    rr[3] = r[3];
}

// include the Runge-Kutta solver from pFaces
#include "rk4ode.cl"

// the model post computes one upper point and one lower point representing
// the set of reach set when the model starts at (x) and (u) is applied
void model_post(concrete_t* post_x_lb, concrete_t* post_x_ub, const concrete_t* x, const concrete_t* u) {

    // some required vars
    concrete_t Q[ssDim] = {ssQnt};
    concrete_t xx[ssDim];
    concrete_t rr[ssDim];
    concrete_t r[ssDim];

	// initialization
	for (unsigned int i = 0; i<ssDim; i++)
		r[i] = Q[i] / 2.0f;

    // solve the ODEs and compute the growth-bound
    rk4OdeSolver(xx, x, u, 'x');
    radius_dynamics(rr, r, u);

	// computing the over-approx of reach-set
	for (unsigned int i = 0; i<ssDim; i++) {
		post_x_lb[i] = xx[i] - rr[i];
		post_x_ub[i] = xx[i] + rr[i];
	}    
}

