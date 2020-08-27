/*
 * omega.cpp
 *
 *  Created on: 27.08.2020
 *      Author: M. Khaled
 */
#include "omega.h"

namespace pFacesOmegaKernels{

	/* some constants */
	const std::string kernel_source = "omega";
	const std::string mem_cfg_file = "omega.mem";
	const size_t thread_grid_x = 1;
	const size_t thread_grid_y = 1;

	/* the driver constructor */
	pFacesOmega::pFacesOmega(
		const std::shared_ptr<pfacesKernelLaunchState>& spLaunchState, 
		const std::shared_ptr<pfacesConfigurationReader>& spCfg) :
		  pfaces2DKernel(
			  spLaunchState->getDefaultSourceFilePath(
				  kernel_source, 
				  spLaunchState->kernelScope, 
				  spLaunchState->kernelPackPath), 
				  thread_grid_x,
				  thread_grid_y, 
				  spCfg), m_spCfg(spCfg){


		// Create a kernel function and load its memory fingerprints
		std::string memFile = spLaunchState->kernelPackPath + mem_cfg_file;
		std::string funcName = "exampleKernelFunction";
		std::vector<std::string> argNames = {"data_in"};
		auto funcArgs = pfacesKernelFunctionArguments::loadFromFile(
			memFile,		/* memory config file with the function memory fingerprint */
			funcName,  		/* name of the function to add */
			argNames		/* list of the names of its args */
		);
		pfacesKernelFunction theFunction(funcName, funcArgs);
		addKernelFunction(theFunction);	  

		// update params in the kernel source
		updatePrameters({"@@param_message@@"}, {"\"Hello from the driver !\""});
	}

	/* provide an implementation of the virtual method: configureParallelProgram*/
	void pFacesOmega::configureParallelProgram(pfacesParallelProgram& parallelProgram){
		
		/* check this is one device */
		bool multiple_devices = parallelProgram.countTargetDevices() > 1;
		if(multiple_devices)
			throw std::runtime_error("This example is supposed to work only within one device.");

		auto thisMachine = parallelProgram.getMachine();
		auto thisDevice = parallelProgram.getTargetDevices()[0];

		/* the range/offset of this kernel : we just need one thread */
		cl::NDRange ndKernelRange(thread_grid_x, thread_grid_y);
		cl::NDRange ndKernelOffset(0,0);

		/* allocate memory */
		std::vector<std::pair<char*, size_t>> dataPool;
		pFacesMemoryAllocationReport memReport;
		memReport = allocateMemory(dataPool, thisMachine, parallelProgram.getTargetDevicesIndicies(), 1, false);


		/* the parallel program needs a alist of instructionss */
		std::vector<std::shared_ptr<pfacesInstruction>> instrList;	

		/* an instruction to show a simple message from the CPU side */
		std::shared_ptr<pfacesInstruction> instrMsg_start = std::make_shared<pfacesInstruction>();
		instrMsg_start->setAsMessage("Hello World from host side !");

		/* an instruction to lauch parallel threadss in the device */
		std::shared_ptr<pfacesDeviceExecuteTask> singleTask = std::make_shared<pfacesDeviceExecuteTask>(ndKernelOffset, ndKernelRange, ndKernelRange);
		std::shared_ptr<pfacesDeviceExecuteJob> exampleFunctionJob = std::make_shared<pfacesDeviceExecuteJob>(thisDevice);
		exampleFunctionJob->addTask(singleTask);
		exampleFunctionJob->setKernelFunctionIdx(
			0,  /* index of the kernel function we're to write data to */
			1  /* how many arguments it has */
		);		
		std::shared_ptr<pfacesInstruction> devFunctionInstruction = std::make_shared<pfacesInstruction>();
		devFunctionInstruction->setAsDeviceExecute(exampleFunctionJob);		

		/* a synchronization instruction */
		std::shared_ptr<pfacesInstruction> instrSyncPoint = std::make_shared<pfacesInstruction>();
		instrSyncPoint->setAsBlockingSyncPoint();

		/* build the parallel program */
		instrList.push_back(instrMsg_start);
		instrList.push_back(devFunctionInstruction);
		instrList.push_back(instrSyncPoint);

		/* complete the required settings in the parallel program */
		parallelProgram.m_Universal_globalNDRange = ndKernelRange;
		parallelProgram.m_Universal_offsetNDRange = ndKernelOffset;
		parallelProgram.m_dataPool = dataPool;
		parallelProgram.m_spInstructionList = instrList;
	}

	/* provide an implementation of the virtual method: configureTuneParallelProgram*/
	void pFacesOmega::configureTuneParallelProgram(pfacesParallelProgram& tuneParallelProgram, size_t targetFunctionIdx) {
		(void)tuneParallelProgram;
		(void)targetFunctionIdx;
	}
}

/* register the kernel */
PFACES_REGISTER_LOADABLE_KERNEL(pFacesOmegaKernels::pFacesOmega)
