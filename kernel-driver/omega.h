/*
 * omega.h
 *
 *  Created on: 27.08.2020
 *      Author: M. Khaled
 */
#pragma once
#include <pfaces-sdk.h>
#include "omegaControlProblem.h"
#include "omegaParityGames.h"
#include "omegaImplementation.h"
#include "omegaUtils.h"

// uncomment this re exclude the unit test functions
#define TEST_FUNCTION

namespace pFacesOmegaKernels {

	// a special types
	typedef std::tuple<std::string, size_t, size_t> arg_info;
	typedef std::pair<std::string, std::vector<arg_info>> func_info;

	// externs for global vars
	extern const std::string discover_x_aps_func_name;
    extern const std::vector<std::string> discover_x_aps_arg_names;
	extern const std::string discover_u_aps_func_name;
    extern const std::vector<std::string> discover_u_aps_arg_names;
	extern const std::string construct_symmodel_func_name;
	extern const std::vector<std::string> construct_symmodel_arg_names;

	// class: pFacesOmega, a 2d-kernel
	class pFacesOmega : public pfaces2DKernel {
	public:
		// the config management object
		const std::shared_ptr<pfacesConfigurationReader> m_spCfg;

		// the memory info
		std::vector<std::pair<char*, size_t>> dataPool;
		pFacesMemoryAllocationReport memReport;	

		// pointers to spec/sym_model/game wrappers
		std::shared_ptr<SymSpec<L_x_func_t, L_u_func_t>> pSymSpec;
		std::shared_ptr<SymModel<post_func_t>> pSymModel;
		std::shared_ptr<PGame<post_func_t, L_x_func_t, L_u_func_t>> pParityGame;
		std::shared_ptr<PGSISolver<post_func_t, L_x_func_t, L_u_func_t>> pParityGameSolver;
		std::shared_ptr<Machine<post_func_t, L_x_func_t, L_u_func_t>> pControllerImpl;

		// some vars
		std::string kernel_path;
		std::string extra_inc_dir;
		size_t verbosity;
		size_t x_dim;
		size_t u_dim;
		std::vector<concrete_t> x_lb, x_ub, x_qs;
		std::vector<concrete_t> u_lb, u_ub, u_qs;
		std::vector<symbolic_t> x_widths;
		std::vector<symbolic_t> u_widths;
		size_t x_symbols;
		size_t u_symbols;
		size_t xu_symbols;
		bool is_realizable = false;
		
		// some ranges/offsets
		cl::NDRange ndKernelRange_X;
		cl::NDRange ndKernelRange_U;
		cl::NDRange ndKernelRange_XU;
		cl::NDRange ndKernelOffset;		

		// some general private functions
		void init_kernel();
		void init_kernel_functions();
		void create_instructions(std::vector<std::shared_ptr<pfacesInstruction>>& instrList, const cl::Device& targetDevice);
		void init_memory();

		// some vars/funcs for: discover_x_aps (see func_discover_x_aps.cpp for implementation)
		size_t x_aps_count;
		size_t size_struct_x_ap_subset;
		bool dont_discover_x_aps = false;
		func_info func_info_discover_x_aps;
		std::vector<std::string> x_aps;
		std::vector<hyperrect> x_aps_subsets;
		std::vector<char> x_aps_subsets_map;
		void write_ss_ap_subset(char* pData, const std::vector<concrete_t> lb, const std::vector<concrete_t> ub);
		void init_discover_x_aps();
		void add_func_discover_x_aps(std::vector<std::shared_ptr<pfacesInstruction>>& instrList, const cl::Device& targetDevice);
		void init_mem_discover_x_aps();
		
		// some vars/funcs for: discover_u_aps (see func_discover_u_aps.cpp for implementation)
		size_t u_aps_count;
		size_t size_struct_u_ap_subset;
		bool dont_discover_u_aps = false;
		func_info func_info_discover_u_aps;
		std::vector<std::string> u_aps;
		std::vector<hyperrect> u_aps_subsets;
		std::vector<char> u_aps_subsets_map;
		void write_is_ap_subset(char* pData, const std::vector<concrete_t> lb, const std::vector<concrete_t> ub);
		void init_discover_u_aps();
		void add_func_discover_u_aps(std::vector<std::shared_ptr<pfacesInstruction>>& instrList, const cl::Device& targetDevice);
		void init_mem_discover_u_aps();	
		
		// some vars/funcs for: construct_symmodel (see func_construct_symmodel.cpp for implementation)
		size_t size_struct_xu_posts;
		symbolic_t x_initial;
		func_info func_info_construct_symmodel;
		void init_construct_symmodel();
		void add_func_construct_symmodel(std::vector<std::shared_ptr<pfacesInstruction>>& instrList, const cl::Device& targetDevice);
		void init_mem_construct_symmodel();	
		

		// some vars/funcs for: construct_pgame (see func_construct_pgame.cpp for implementation)
		void init_construct_pgame();
		void add_func_construct_pgame(std::vector<std::shared_ptr<pfacesInstruction>>& instrList, const cl::Device& targetDevice);

		// some vars/funcs for: solve_pgame (see func_solve_pgame.cpp for implementation)
		void init_solve_pgame();
		void add_func_solve_pgame(std::vector<std::shared_ptr<pfacesInstruction>>& instrList, const cl::Device& targetDevice);

		// some vars/funcs for: implement (see func_implement.cpp for implementation)
		void init_implement();
		void add_func_implement(std::vector<std::shared_ptr<pfacesInstruction>>& instrList, const cl::Device& targetDevice);		


		pFacesOmega(
			const std::shared_ptr<pfacesKernelLaunchState>& spLaunchState, 
			const std::shared_ptr<pfacesConfigurationReader>& spCfg
		);
		~pFacesOmega() = default;

		void configureParallelProgram(
			pfacesParallelProgram& parallelProgram);

		void configureTuneParallelProgram(
			pfacesParallelProgram& tuneParallelProgram, 
			size_t targetFunctionIdx);
	};

	/* a pointer to the only object */
	extern pFacesOmega* pKernel;
}
