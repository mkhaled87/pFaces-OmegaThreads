/*
* omega_symmodel.cl
*
*  date    : 9.09.2020
*  author  : M. Khaled
*  details : an OpenCL kernel function to construct a symbolic
*            model from the concrete system provided by the user.
*/

// prototype of the post function from the included code file
void model_post(concrete_t* post_x_lb, concrete_t* post_x_ub, const concrete_t* x, const concrete_t* u);

// include the dynamics source file
#include @pfaces-configValueString:"system.dynamics.code_file"

// a memory bag for post-state information per (x,y)
typedef struct __attribute__((packed)) xu_posts {
	concrete_t  cnc_dest_states_lb[ssDim];
	concrete_t  cnc_dest_states_ub[ssDim];
} xu_posts_t;

// K E R N E L   F U N C T I O N
// over: symbolic xu space
// info: this kernel function computes the posts of symbolic models
//       the cuntion operates on only on (x,u) and stores the posts
//       to the corresponding xu-bag in xu_bags
__kernel void construct_symmodel(__global xu_posts_t* xu_posts_bags){

	__private concrete_t x[ssDim];
	__private concrete_t u[isDim];
	__private concrete_t post_x_lb[ssDim];
	__private concrete_t post_x_ub[ssDim];
	__private symbolic_t thread_idx_x;
	__private symbolic_t thread_idx_u;
	__private symbolic_t flat_thread_idx;

	thread_idx_x = UNIVERSAL_INDEX_X;
	thread_idx_u = UNIVERSAL_INDEX_Y;
	flat_thread_idx = thread_idx_u + thread_idx_x*UNIVERSAL_WIDTH_Y;

	get_concrete_x(thread_idx_x, x);
	get_concrete_u(thread_idx_u, u);
	model_post(post_x_lb, post_x_ub, x, u);

	for (unsigned int i = 0; i<ssDim; i++) {
		xu_posts_bags[flat_thread_idx].cnc_dest_states_lb[i] = post_x_lb[i];
		xu_posts_bags[flat_thread_idx].cnc_dest_states_ub[i] = post_x_ub[i];
	}	
}