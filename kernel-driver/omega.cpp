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
		x_lb = pfacesUtils::sStr2Vector<concrete_t>(m_spCfg->readConfigValueString("system.states.lower_bound"));
		x_ub = pfacesUtils::sStr2Vector<concrete_t>(m_spCfg->readConfigValueString("system.states.upper_bound"));
		x_qs = pfacesUtils::sStr2Vector<concrete_t>(m_spCfg->readConfigValueString("system.states.quantizers"));
		u_lb = pfacesUtils::sStr2Vector<concrete_t>(m_spCfg->readConfigValueString("system.controls.lower_bound"));
		u_ub = pfacesUtils::sStr2Vector<concrete_t>(m_spCfg->readConfigValueString("system.controls.upper_bound"));
		u_qs = pfacesUtils::sStr2Vector<concrete_t>(m_spCfg->readConfigValueString("system.controls.quantizers"));


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

		/*
		std::vector<concrete_t> ss_zero(x_dim, 0.0);
		std::vector<concrete_t> is_zero(u_dim, 0.0);

		x_symbols = pfacesBigInt::getPrimitiveValue(
			pfacesFlatSpace::getFlatWidthFromConcreteSpace(
				x_dim, x_qs, x_lb, x_ub, ss_zero, x_widths));

		u_symbols = pfacesBigInt::getPrimitiveValue(
			pfacesFlatSpace::getFlatWidthFromConcreteSpace(
				u_dim, u_qs, u_lb, u_ub, is_zero, u_widths));
		*/

		xu_symbols = x_symbols * u_symbols;	


		std::cout << "Universal X-space (" << x_dim << "d) has " << x_symbols << " symbols: ";
		pfacesUtils::PrintVector(x_widths, 'x');

		std::cout << "Universal U-space (" << u_dim << "d) has " << u_symbols << " symbols: ";
		pfacesUtils::PrintVector(u_widths, 'x');

		std::cout << "Universal XU-space has " << xu_symbols << " symbols." << std::endl;


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

		// initialize the kernel/functions
		init_kernel();
		init_kernel_functions();
			
		// update the params
		updatePrameters(
			{ "@@concrete_t_name@@", "@@symbolic_t_name@@"},
			{ concrete_t_cl_string,  symbolic_t_cl_string}
		);
	}

	/* provide the parallel program */
	void pFacesOmega::configureParallelProgram(pfacesParallelProgram& parallelProgram){

		verbosity = parallelProgram.m_beVerboseLevel;
		
		/* check this is one device */
		bool multiple_devices = parallelProgram.countTargetDevices() > 1;
		if(multiple_devices)
			throw std::runtime_error("This kernel does not currently supposed multiple device.");

		auto thisMachine = parallelProgram.getMachine();
		auto targetDevice = parallelProgram.getTargetDevices()[0];

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
