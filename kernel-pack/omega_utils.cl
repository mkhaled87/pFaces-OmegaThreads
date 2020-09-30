/*
* omega_utils.cl
*
*  date    : 19.08.2020
*  author  : M. Khaled
*/

// a shared function to compute current x from the thread index
// here, we assuume that the first thread is for first x and
// the last thread is the last element in X
void get_concrete_x(const symbolic_t thread_idx, concrete_t* x);
void get_concrete_x(const symbolic_t thread_idx, concrete_t* x){
	__private concrete_t sseta[ssDim] = {ssQnt};
	__private concrete_t sslb[ssDim]  = {ssLb};
	__private concrete_t ssub[ssDim]  = {ssUb};	
	__private flat_t	 x_flat[FLAT_TYPE_SIZE];
	
	flat_assign_singleton_symbolic(x_flat, &thread_idx);
	flat_to_concrete_ss(x_flat, sslb, ssub, sseta, x);
}

// a shared function to compute current u from the thread index
// here, we assuume that the first thread is for first u and
// the last thread is the last element in U
void get_concrete_u(const symbolic_t thread_idx, concrete_t* u);
void get_concrete_u(const symbolic_t thread_idx, concrete_t* u){
	__private concrete_t iseta[isDim] = {isQnt};
	__private concrete_t islb[isDim]  = {isLb};
	__private concrete_t isub[isDim]  = {isUb};	
	__private flat_t	 u_flat[FLAT_TYPE_SIZE];

	flat_assign_singleton_symbolic(u_flat, &thread_idx);
	flat_to_concrete_is(u_flat, islb, isub, iseta, u);
}
