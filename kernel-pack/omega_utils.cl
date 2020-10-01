/*
* omega_utils.cl
*
*  date    : 19.08.2020
*  author  : M. Khaled
*/

// a shared function to compute current x from its flat index (thread index)
// in X-flat-space here, we assuume that the first thread is for first x and
// the last thread is the last element in X
void get_concrete_x(const symbolic_t x_flat,  concrete_t* x_conc);
void get_concrete_x(const symbolic_t x_flat,  concrete_t* x_conc){

	__private symbolic_t fltCurrent;
	__private symbolic_t fltIntial;
	__private symbolic_t fltVolume;
	__private symbolic_t dim_width[ssDim];
	__private symbolic_t x_sym[ssDim];
	__private concrete_t eta[ssDim] = {ssQnt};
	__private concrete_t lb[ssDim]  = {ssLb};
	__private concrete_t ub[ssDim]  = {ssUb};		
	
	fltIntial = x_flat;
	
	for(unsigned int i=0; i<ssDim; i++)
		dim_width[i] = floor((ub[i]-lb[i])/eta[i])+1;

	for(int i=ssDim-1; i>=0; i--){
        fltCurrent = fltIntial;

        fltVolume = 1;
		for(int k=0; k<i; k++){
			fltVolume = fltVolume * dim_width[k];
		}
		
		fltCurrent = fltCurrent / fltVolume;
		fltCurrent = fltCurrent % dim_width[i];
		
		x_sym[i] = fltCurrent; 
		
		fltCurrent = fltCurrent * fltVolume;
		fltIntial = fltIntial - fltCurrent;
	}

	for(unsigned int i=0; i<ssDim; i++)
		x_conc[i] = lb[i] + ((concrete_t)x_sym[i])*eta[i];	
}

// a shared function to compute current u from its flat index (thread index)
// in U-flat-space here, we assuume that the first thread is for first u and
// the last thread is the last element in U
void get_concrete_u(const symbolic_t u_flat, concrete_t* u_conc);
void get_concrete_u(const symbolic_t u_flat, concrete_t* u_conc){

	symbolic_t fltCurrent;
	symbolic_t fltIntial;
	symbolic_t fltVolume;
	symbolic_t dim_width[isDim];
	symbolic_t u_sym[isDim];
	__private concrete_t eta[isDim] = {isQnt};
	__private concrete_t lb[isDim]  = {isLb};
	__private concrete_t ub[isDim]  = {isUb};		
	
	fltIntial = u_flat;
	
	for(unsigned int i=0; i<isDim; i++)
		dim_width[i] = floor((ub[i]-lb[i])/eta[i])+1;

	for(int i=isDim-1; i>=0; i--){
        fltCurrent = fltIntial;

        fltVolume = 1;
		for(int k=0; k<i; k++){
			fltVolume = fltVolume * dim_width[k];
		}
		
		fltCurrent = fltCurrent / fltVolume;
		fltCurrent = fltCurrent % dim_width[i];
		
		u_sym[i] = fltCurrent; 
		
		fltCurrent = fltCurrent % fltVolume;
		fltIntial = fltIntial - fltCurrent;
	}

	for(unsigned int i=0; i<isDim; i++)
		u_conc[i] = lb[i] + ((concrete_t)u_sym[i])*eta[i];	
}

