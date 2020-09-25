/*
* omega_aps.cl
*
*  date    : 19.08.2020
*  author  : M. Khaled
*  details : OpenCL kernel functions to identify a symbolic
*            the atomic propositon in X and U spaces.
*/

// a memory bag for AP bounds (lower and upper bounds)
typedef struct __attribute__((packed)) ss_ap_subset {
	concrete_t interval_lb[ssDim];
	concrete_t interval_ub[ssDim];
} x_ap_subset_t;
typedef struct __attribute__((packed)) is_ap_subset {
	concrete_t interval_lb[isDim];
	concrete_t interval_ub[isDim];
} u_ap_subset_t;


// K E R N E L   F U N C T I O N
// over: symbolic x space
// info: this kernel function discovers which APs the current 
//       x belong to.
__kernel void discover_x_aps(
	__global uint* x_aps_bags, 					// a bit for each AP (max 32 APs): x \in AP_in[i] ?
	__constant uint* num_aps_subsets,			// number of subsets info data sets
	__constant x_ap_subset_t* aps_subsets,		// interval info for inp-APs
	__constant char* aps_subsets_assignments	// to which inp-AP does each interval belong
	){

	__private concrete_t  x[ssDim];
	__private char ap_maps_to;
	__private uint ap_mask;
	__private symbolic_t thread_idx;

	thread_idx = UNIVERSAL_INDEX_X;
	get_concrete_x(thread_idx, x);

	// for eachh interval
	for(uint ap_i=0;  ap_i < *num_aps_subsets; ap_i++){

		// is x \in AP[i] ?
		bool is_inside = true;
		for(uint d=0;  d<ssDim; d++){
			if(
				x[d] < aps_subsets[ap_i].interval_lb[d] ||
				x[d] > aps_subsets[ap_i].interval_ub[d]
			){
				is_inside = false;
			}
		}

		// if x \in AP[i], set its flag
		if(is_inside){
			ap_maps_to = aps_subsets_assignments[ap_i];
			ap_mask = 1 << ap_maps_to;
			x_aps_bags[thread_idx] = x_aps_bags[thread_idx] | ap_mask;
		}
	}
}

// K E R N E L   F U N C T I O N
// over: symbolic u space
// info: this kernel function discovers which APs the current 
//       u belong to.
__kernel void discover_u_aps(
	__global uint* u_aps_bags, 					// a bit for each AP (max 32 APs): u \in AP_out[i] ?
	__constant uint* num_aps_subsets,			// number of subsets info data sets
	__constant u_ap_subset_t* aps_subsets,		// interval info for out-APs
	__constant char* aps_subsets_assignments	// to which out-AP does each interval belong
){
	__private concrete_t  u[isDim];
	__private char ap_maps_to;
	__private uint ap_mask;
	__private symbolic_t thread_idx;

	thread_idx = UNIVERSAL_INDEX_X;
	get_concrete_u(thread_idx, u);

	// for eachh interval
	for(uint ap_i=0;  ap_i<*num_aps_subsets; ap_i++){

		// is u \in AP[i] ?
		bool is_inside = true;
		for(uint d=0;  d<isDim; d++){
			if(
				u[d] < aps_subsets[ap_i].interval_lb[d] ||
				u[d] > aps_subsets[ap_i].interval_ub[d]
			){
				is_inside = false;
			}
		}

		// if u \in AP[i], set its flag
		if(is_inside){
			ap_maps_to = aps_subsets_assignments[ap_i];
			ap_mask = 1 << ap_maps_to;
			u_aps_bags[thread_idx] = u_aps_bags[thread_idx] | ap_mask;
		}
	}
}