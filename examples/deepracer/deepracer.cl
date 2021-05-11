/*-----------------------------------------------------------------------
 * File: deepracer.cl
 * Date: 06.05.2021
 * Athr: M. Khaled
 * Desc: The dynamics of the deepracer (4d).
 *-----------------------------------------------------------------------*/

// sampling period for the vehilce dynamics
#define SAMPLING_PERIOD STEP_TIME

// some maps : see the file ./InMap_Ident/Details.pdf for more details
inline concrete_t map_steering(const concrete_t angle_in){
    concrete_t p1 = -0.116;
    concrete_t p2 = -0.02581;
    concrete_t p3 = 0.3895;
    concrete_t p4 = 0.02972;
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
        K = 1.5f; 
		T = 0.25f;
	}
	else if (u_speed == 0.45f){
        K = 1.9353f; 
		T = 0.9092f;
    }
	else if (u_speed == 0.50f){
        K = 2.2458f; 
		T = 0.8942f;
    }
	else if (u_speed == 0.55f){
        K = 2.8922f; 
		T = 0.8508f;
    }
	else if (u_speed == 0.60f){
        K = 3.0332f; 
		T = 0.8642f;
    }
	else if (u_speed == 0.65f){
        K = 3.1274f; 
		T = 0.8419f;
    }
	else if (u_speed == 0.70f){
        K = 3.7112f; 
		T = 0.8822f;
    }
	else if (u_speed == -0.45f){
        K = 1.6392f; 
		T = 1.3008f;
    }
	else if (u_speed == -0.50f){
        K = 2.5998f; 
		T = 0.9882f;
    }
	else if (u_speed == -0.55f){
        K = 2.8032f; 
		T = 0.9640f;
    }
	else if (u_speed == -0.60f){
        K = 3.1457f; 
		T = 0.9741f;
    }
	else if (u_speed == -0.65f){
        K = 3.6170f; 
		T = 0.9481f;
    }
	else if (u_speed == -0.70f){
        K = 3.6391f; 
		T = 0.9285f;
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
void radius_dynamics(concrete_t* rr, const concrete_t* r, const concrete_t* u);
void radius_dynamics(concrete_t* rr, const concrete_t* r, const concrete_t* u) {
	rr[0] = r[0];
	rr[1] = r[1];
	rr[2] = r[2];
	rr[3] = r[3];
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
    radius_dynamics(rr, r, u);

	// wrap the orrinetation angle xx[2] in [-pi,pi]
	wrapToPi(&xx[2]);

	// computing the over-approx of reach-set
	for (unsigned int i = 0; i<ssDim; i++) {
		post_x_lb[i] = xx[i] - rr[i];
		post_x_ub[i] = xx[i] + rr[i];
	}    
}

