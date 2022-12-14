/*
 * omega.cpp
 *
 *  Created on: 27.08.2020
 *      Author: M. Khaled
 */
#include "omega.h"

namespace pFacesOmegaKernels{

	/* a pointer to the only object */
    pFacesOmega* pKernel;

	/* some constants */
	const std::string kernel_source = "omega";
	const std::string mem_cfg_file = "omega.mem";

	/* a member function to initialize the kernel */
	void pFacesOmega::init_kernel(){

		// load values from the config file
		x_dim = m_spCfg->readConfigValueInt("system.states.dimension");
		u_dim = m_spCfg->readConfigValueInt("system.controls.dimension");
		x_lb = pfacesUtils::sStr2Vector<concrete_t>(m_spCfg->readConfigValueString("system.states.first_symbol"));
		x_ub = pfacesUtils::sStr2Vector<concrete_t>(m_spCfg->readConfigValueString("system.states.last_symbol"));
		x_qs = pfacesUtils::sStr2Vector<concrete_t>(m_spCfg->readConfigValueString("system.states.quantizers"));
		u_lb = pfacesUtils::sStr2Vector<concrete_t>(m_spCfg->readConfigValueString("system.controls.first_symbol"));
		u_ub = pfacesUtils::sStr2Vector<concrete_t>(m_spCfg->readConfigValueString("system.controls.last_symbol"));
		u_qs = pfacesUtils::sStr2Vector<concrete_t>(m_spCfg->readConfigValueString("system.controls.quantizers"));


		// some checks
		if(x_lb.size() != x_dim || x_ub.size() != x_dim || x_qs.size() != x_dim)
			throw std::runtime_error(
					std::string("Invalid number of elements in states.first_symbol, states.last_symbol or states.quantizers.")
				);
		if(u_lb.size() != u_dim || u_ub.size() != u_dim || u_qs.size() != u_dim)
			throw std::runtime_error(
					std::string("Invalid number of elements in controls.first_symbol, controls.last_symbol or controls.quantizers.")
				);				
		for (size_t i=0; i<x_dim; i++){
			if(x_ub[i] < x_lb[i])
				throw std::runtime_error(
					std::string("The provided states.first_symbol (for index=") + std::to_string(i) +
					std::string(") should be less than the states.last_symbol.")
				);

			if(x_qs[i] > (x_ub[i] - x_lb[i]) || x_qs[i] < 0.0)
				throw std::runtime_error(
					std::string("The provided states.quantizers (for index=") + std::to_string(i) +
					std::string(") is not valid: should be positive and less than the width between first and last symbols.")
				);
		}
		for (size_t i=0; i<u_dim; i++){
			if(u_ub[i] < u_lb[i])
				throw std::runtime_error(
					std::string("The provided controls.first_symbol (for index=") + std::to_string(i) +
					std::string(") should be less than the controls.last_symbol.")
				);

			if(u_qs[i] > (u_ub[i] - u_lb[i]) || u_qs[i] < 0.0)
				throw std::runtime_error(
					std::string("The provided controls.quantizers (for index=") + std::to_string(i) +
					std::string(") is not valid: should be positive and less than the width between first and last symbols.")
				);				
		}


		// x symbols
		x_symbols = 1;
		x_widths = OmegaUtils::getWidths(x_lb, x_qs, x_ub);
		for(size_t i=0; i<x_widths.size(); i++)
			x_symbols *= x_widths[i];

		// u symbols
		u_symbols = 1;
		u_widths = OmegaUtils::getWidths(u_lb, u_qs, u_ub);
		for(size_t i=0; i<u_widths.size(); i++)
			u_symbols *= u_widths[i];


		// compare against expected upper bounds
		bool x_ub_updated = false;
		for (size_t i=0; i<x_dim; i++){
			concrete_t x_ub_i_expected = x_lb[i] + (x_widths[i]-1)*x_qs[i];
			if(x_ub_i_expected != x_ub[i]){
				x_ub_updated = true;
				x_ub[i] = x_ub_i_expected;
			}
		}
		if(x_ub_updated)
			pfacesTerminal::showWarnMessage(
				std::string("The states.lasts_symbol is not compliant with states.quantizers and is modified to: ") +
				pfacesUtils::vector2string(x_ub)
			);
		bool u_ub_updated = false;
		for (size_t i=0; i<u_dim; i++){
			concrete_t u_ub_i_expected = u_lb[i] + (u_widths[i]-1)*u_qs[i];
			if(u_ub_i_expected != u_ub[i]){
				u_ub_updated = true;
				u_ub[i] = u_ub_i_expected;
			}
		}
		if(u_ub_updated)
			pfacesTerminal::showWarnMessage(
				std::string("The controls.lasts_symbol is not compliant with controls.quantizers and is modified to: ") +
				pfacesUtils::vector2string(u_ub)
			);	

		// xu symbols
		xu_symbols = x_symbols * u_symbols;	

		// print info
		std::stringstream ss_x_widths, ss_u_widths;
		pfacesUtils::PrintVector(x_widths, 'x', false, ss_x_widths);
		pfacesUtils::PrintVector(u_widths, 'x', false, ss_u_widths);
		pfacesTerminal::showInfoMessage(
			std::string("Symbolic model: X-space (") + std::to_string(x_dim) + 
			std::string("d) is quantized and it has ") + std::to_string(x_symbols) + std::string(" symbols: ") +
			ss_x_widths.str()
		);
		pfacesTerminal::showInfoMessage(
			std::string("Symbolic model: U-space (") + std::to_string(u_dim) + 
			std::string("d) is quantized and it has ") + std::to_string(u_symbols) + std::string(" symbols: ") +
			ss_x_widths.str()
		);
		pfacesTerminal::showInfoMessage(
			std::string("Symbolic model: XU-space should have ") + std::to_string(xu_symbols) + std::string(" symbols.")
		);		


		// check and prepare the dynamics code file
		std::string config_dir = pfacesFileIO::getFileDirectoryPath(m_spCfg->getConfigFilePath());
		if (config_dir.empty() || config_dir == std::string("")) {
			config_dir = std::string(".") + std::string(PFACES_PATH_SPLITTER);
		}
		std::string code_file = config_dir + m_spCfg->readConfigValueString("system.dynamics.code_file");
		if (!pfacesFileIO::isFileExist(code_file))
			throw std::runtime_error(
				std::string("The code file: ") + code_file +
				std::string(" does not exist in the same directory of the config file.")
			);
		extra_inc_dir = std::string(" -I") +pfacesFileIO::getFileDirectoryPath(code_file);		

		// ask each function to init
		init_discover_x_aps();
		init_discover_u_aps();
		init_construct_symmodel();
		init_construct_pgame();
		init_solve_pgame();
		init_implement();
	}

	/* a member function to initialize the low-level kernel functions */
	void pFacesOmega::init_kernel_functions(){


		// collect function names and argument data
		std::vector<func_info> funcs;
		if(!dont_discover_x_aps)
			funcs.push_back(func_info_discover_x_aps);
		if(!dont_discover_u_aps)
			funcs.push_back(func_info_discover_u_aps);		
		funcs.push_back(func_info_construct_symmodel);	
			

		// create and add the functions
		std::string memFile = kernel_path + mem_cfg_file;
		for(auto func : funcs){

			auto funcName = func.first;
			std::vector<std::string> argNames;
			std::vector<size_t> argBaseSizes;
			std::vector<size_t> argTypeMultiples;

			for (auto arg_tuple : func.second){
				argNames.push_back(std::get<0>(arg_tuple));
				argBaseSizes.push_back(std::get<1>(arg_tuple));
				argTypeMultiples.push_back(std::get<2>(arg_tuple));
			}

			auto funcArgs = pfacesKernelFunctionArguments::loadFromFile(
				memFile,		/* memory config file with the function memory fingerprint */
				funcName,  		/* name of the function to add */
				argNames,		/* list of the names of its args */
				false			/* do not save memory render files */
			);

			funcArgs.m_baseTypeSize = argBaseSizes;
			funcArgs.m_baseTypeMultiple = argTypeMultiples;
			pfacesKernelFunction theFunction(funcName, funcArgs);
			addKernelFunction(theFunction);
		}
	}

	/* a member function to construct the instruction list */
	void pFacesOmega::create_instructions(std::vector<std::shared_ptr<pfacesInstruction>>& instrList, const cl::Device& targetDevice){

		/* an instruction to show a simple message from the CPU side */
		std::shared_ptr<pfacesInstruction> instrMsg_start = std::make_shared<pfacesInstruction>();
		instrMsg_start->setAsMessage("OmegaThreads kernel started !");
		instrList.push_back(instrMsg_start);

		/* add instructions for the kernel function: discover_x_aps */
		add_func_discover_x_aps(instrList, targetDevice);

		/* add instructions for the kernel function: discover_u_aps */
		add_func_discover_u_aps(instrList, targetDevice);

		/* add instructions for the kernel function: construct_symmodel */
		add_func_construct_symmodel(instrList, targetDevice);

		/* add instructions for constructing the parity game */
		add_func_construct_pgame(instrList, targetDevice);

		/* add instructions for solving the parity game */
		add_func_solve_pgame(instrList, targetDevice);

		/* add instructions for implementing */
		add_func_implement(instrList, targetDevice);		

		/* a synchronization instruction should always end the program */
		std::shared_ptr<pfacesInstruction> instrSyncPoint = std::make_shared<pfacesInstruction>();
		instrSyncPoint->setAsBlockingSyncPoint();		
		instrList.push_back(instrSyncPoint);
	}

	/* a member function to initialize the memory */
	void pFacesOmega::init_memory(){

		// ask the kernel functions to init the memories
		init_mem_discover_x_aps();
		init_mem_discover_u_aps();
		init_mem_construct_symmodel();
	}

	/* the driver constructor */
	pFacesOmega::pFacesOmega(
		const std::shared_ptr<pfacesKernelLaunchState>& spLaunchState, 
		const std::shared_ptr<pfacesConfigurationReader>& spCfg) :
			pfaces2DKernel(
				spLaunchState->getDefaultSourceFilePath(kernel_source, spLaunchState->kernelScope, spLaunchState->kernelPackPath), 
				spCfg
			), 
			m_spCfg(spCfg){

		kernel_path = spLaunchState->kernelPackPath;
		pKernel = this;

		// initialize the params
		param_names = { "@@concrete_t_name@@", "@@symbolic_t_name@@"};
		param_values = { concrete_t_cl_string,  symbolic_t_cl_string};

		// initialize the kernel/functions
		init_kernel();
		init_kernel_functions();
			
		// update the params
		updatePrameters(param_names, param_values);
	}

	/* provide the parallel program */
	void pFacesOmega::configureParallelProgram(pfacesParallelProgram& parallelProgram){

		verbosity = parallelProgram.m_beVerboseLevel;
		
		/* check this is one device */
		bool multiple_devices = parallelProgram.countTargetDevices() > 1;
		if(multiple_devices)
			throw std::runtime_error("This kernel does not currently supposed multiple device.");

		const auto& thisMachine = parallelProgram.getMachine();
		const auto& targetDevice = parallelProgram.getTargetDevices()[0];

		/* init the ranges/offsets to be used */
		ndKernelRange_X  = cl::NDRange(x_symbols, 1 ,1);
		ndKernelRange_U  = cl::NDRange(u_symbols, 1, 1);
		ndKernelRange_XU = cl::NDRange(x_symbols, u_symbols, 1);
		ndKernelOffset   = cl::NDRange(0,0,0);

		/* allocate memory */
		memReport = allocateMemory(dataPool, thisMachine, parallelProgram.getTargetDevicesIndicies(), 1, false);
		if(verbosity >= 3)
			memReport.PrintReport();

		// initialize the memory
		init_memory();

		/* the parallel program needs a alist of instructions */
		std::vector<std::shared_ptr<pfacesInstruction>> instrList;	
		create_instructions(instrList, targetDevice);

		/* add any extra defines */
		std::string extra_defines_str = parallelProgram.m_spCfgReader->readConfigValueString("system.dynamics.code_defines");
		std::vector<std::string> extra_defines_list = pfacesUtils::strSplit(extra_defines_str, ";", false);
		for(size_t i=0; i<extra_defines_list.size(); i++){
			auto name_value = pfacesUtils::strSplit(extra_defines_list[i], "=", false);
			std::string name = name_value[0];
			std::string value = "";
			if(name_value.size() == 2)
				value = name_value[1];
			parallelProgram.m_compilerDefinesList.push_back(std::make_pair(name, value));
		}
			

		/* complete the required settings in the parallel program */
		parallelProgram.m_Universal_globalNDRange = ndKernelRange_XU;
		parallelProgram.m_Universal_offsetNDRange = ndKernelOffset;
		parallelProgram.m_Process_globalNDRange = ndKernelRange_XU;
		parallelProgram.m_Process_offsetNDRange = ndKernelOffset;		
		parallelProgram.m_dataPool = dataPool;
		parallelProgram.m_spInstructionList = instrList;
		parallelProgram.m_oclOptions += extra_inc_dir;		
	}

	/* provide a tune program */
	void pFacesOmega::configureTuneParallelProgram(pfacesParallelProgram& tuneParallelProgram, size_t targetFunctionIdx) {
		(void)tuneParallelProgram;
		(void)targetFunctionIdx;
	}
}

/* register the kernel */
PFACES_REGISTER_LOADABLE_KERNEL(pFacesOmegaKernels::pFacesOmega)
