/*-----------------------------------------------------------------------
 * File: deepracer.cl
 * Date: 06.05.2021
 * Athr: M. Khaled
 * Desc: The dynamics of the deepracer (4d).
 *-----------------------------------------------------------------------*/

#ifndef M_PI
#define M_PI 3.1415926f
#endif


// sampling period for the vehilce dynamics
#define SAMPLING_PERIOD STEP_TIME

// some maps : see the file ./InMap_Ident/Details.pdf for more details
inline concrete_t map_steering(const concrete_t angle_in){
    concrete_t p1 = -0.1167;
    concrete_t p2 = 0.01949;
    concrete_t p3 = 0.3828;
    concrete_t p4 = -0.0293;
    concrete_t x = angle_in;
    return p1*x*x*x + p2*x*x + p3*x + p4;
}
inline concrete_t map_speed(const symbolic_t speed_in){
	switch(speed_in) {
		case  6: return  0.70f;
		case  5: return  0.65f;
		case  4: return  0.60f;
		case  3: return  0.55f;
		case  2: return  0.50f;
		case  1: return  0.45f;
		case  0: return  0.00f;
		case -1: return -0.45f;
		case -2: return -0.50f;
		case -3: return -0.55f;
		case -4: return -0.60f;
		case -5: return -0.65f;
		case -6: return -0.70f;
	}
	return 0.0f;
}
inline void get_v_params(concrete_t u_speed, concrete_t* a, concrete_t* b){
	concrete_t K,T;
    if (u_speed == 0.0f){
        K = 0.0f; 
		T = 0.25f;
	}
	else if (u_speed == 0.45f){
        K = 1.9953f; 
		T = 0.9933f;
    }
	else if (u_speed == 0.50f){
        K = 2.3567f;
		T = 0.8943f;
    }
	else if (u_speed == 0.55f){
        K = 3.0797f; 
		T = 0.88976f;
    }
	else if (u_speed == 0.60f){
        K = 3.2019f; 
		T = 0.87595f;
    }
	else if (u_speed == 0.65f){
        K = 3.3276f; 
		T = 0.89594f;
    }
	else if (u_speed == 0.70f){
        K = 3.7645f; 
		T = 0.92501f;
    }
	else if (u_speed == -0.45f){
        K = 1.8229f; 
		T = 1.8431f;
    }
	else if (u_speed == -0.50f){
        K = 2.3833f; 
		T = 1.2721f;
    }
	else if (u_speed == -0.55f){
        K = 2.512f; 
		T = 1.1403f;
    }
	else if (u_speed == -0.60f){
        K = 3.0956f; 
		T = 1.1278f;
    }
	else if (u_speed == -0.65f){
        K = 3.55f; 
		T = 1.1226f;
    }
	else if (u_speed == -0.70f){
        K = 3.6423f; 
		T = 1.1539f;
	}
    else{
        printf("get_v_params: Invalid input !\n");
		K = 0.0f;
		T = 0.0f;
	}
    *a = -1.0f/T;
    *b = K/T;
}

// post dynamics (ODE)
// u[0] steering angle in [-1,1] (must be mapped -> [-2.8,2.8])
// u[1] speed in [-6;6] (must be mapped -> [-0.7,-0.45]U{0.0}U[0.45,0.7])
// x[0] x-pos
// x[1] y-pos
// x[2] theta (orientation)
// x[3] forward velocity
void post_dynamics(concrete_t* xx, const concrete_t* x, const concrete_t* u);
void post_dynamics(concrete_t* xx, const concrete_t* x, const concrete_t* u) {
	
	concrete_t u_steer = map_steering(u[0]);
	concrete_t u_speed = map_speed(u[1]);
	concrete_t L = 0.165f;
	concrete_t a,b;
	get_v_params(u_speed, &a, &b);

	xx[0] = x[3]*cos(x[2]);
	xx[1] = x[3]*sin(x[2]);
	xx[2] = (x[3]/L)*tan(u_steer);
	xx[3] = a*x[3] + b*u_speed;
}

// radius dynamics (difference equation)
void xu_radius_dynamics(concrete_t* rr, const concrete_t* r, const concrete_t* x, const concrete_t* u);
void xu_radius_dynamics(concrete_t* rr, const concrete_t* r, const concrete_t* x, const concrete_t* u) {
#if defined(USE_DETERMINISTIC_DYNAMICS)
	rr[0] = 0.0f;
	rr[1] = 0.0f;
	rr[2] = 0.0f;
	rr[3] = 0.0f;
#elif defined(USE_RADIUS_DYNAMICS_R1123)
	// i got thse values impirically (see: InMap_Ident/r-data/)
	// briefly, i construct the symmodel first as deterministic and
	// then i  run it for hunderends of simulations to get these
	// deviatin values compared to solving the ode starting from 
	// different points in the initial cell
	// ---------------------------------------------------------------
	// THIS IS ONLY VALID FOR THE THE CONFIG FILE deepracer_small.cfg
	// ---------------------------------------------------------------
	rr[0] = 0.1319f;
	rr[1] = 0.1285f;
	rr[2] = 0.2278f;
	rr[3] = 0.0943f;
#elif defined(USE_RADIUS_LINREGRESS_XU_AWARE)
	// i got thse params a-e using Monecarlo and Lienar Regression 
	// (see: InMap_Ident/r-data/). Note states x1,x2 have no varzing 
	// effect on rr.
	// ---------------------------------------------------------------
	// THIS IS ONLY VALID FOR THE THE CONFIG FILE deepracer_small.cfg
	// ---------------------------------------------------------------
	double a1=-2.0764e-06, b1=0.00034085, c1=-0.00014002, d1=-0.0003563,  e1=0.096095;
	double a2= 4.0681e-06, b2=0.00035907, c2=-3.4867e-05, d2=-0.00036214, e2=0.095512;
	double a3= 7.1179e-06, b3=1.6091e-05, c3= 0.010564,   d3=-0.00041785, e3=0.14706;
	double a4=-0.001525,   b4=-2.6201e-06,c4=3.4513e-07,  d4=-4.8414e-07, e4=0.079527;
	rr[0] = a1*x[2] + b1*x[3] + c1*u[0] + d1*u[1] + e1;
	rr[1] = a2*x[2] + b2*x[3] + c2*u[0] + d2*u[1] + e2;
	rr[2] = a3*x[2] + b3*x[3] + c3*u[0] + d3*u[1] + e3;
	rr[3] = a4*x[2] + b4*x[3] + c4*u[0] + d4*u[1] + e4;
#else
	#error Radius Dynamics Not Defined!
#endif
}

// dummy radius dynamics function so that the solver does not complain
void radius_dynamics(concrete_t* rr, const concrete_t* r, const concrete_t* u);
void radius_dynamics(concrete_t* rr, const concrete_t* r, const concrete_t* u) {
}

// include the Runge-Kutta solver from pFaces
#include "rk4ode.cl"

#define M_2PI ((2.0f*(M_PI)))
inline void wrapToPi(concrete_t* rad){
	*rad -= M_2PI * floor((*rad + M_PI) * (1.0f/M_2PI));
}

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
    xu_radius_dynamics(rr, r, x, u);

	// wrap the orrinetation angle xx[2] in [-pi,pi]
	wrapToPi(&xx[2]);

	// computing the over-approx of reach-set
	for (unsigned int i = 0; i<ssDim; i++) {
		post_x_lb[i] = xx[i] - rr[i];
		post_x_ub[i] = xx[i] + rr[i];
	}    
}

