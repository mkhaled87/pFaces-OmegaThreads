/*
 * func_construct_symmodel.cpp
 *
 *  Created on: 10.09.2020
 *      Author: M. Khaled
 */

#include "omega.h"

namespace pFacesOmegaKernels {
    
	/* constants to represent the function name and the names of its arguments */
    const concrete_t inf_val = std::numeric_limits<concrete_t>::infinity();
	const std::string construct_symmodel_func_name = std::string("construct_symmodel");
	const std::vector<std::string> construct_symmodel_arg_names = {
		"xu_posts_bags"
	};

#ifdef TEST_FUNCTION
	/* a host-side function to retrieve the memory ant test the output */
	size_t test_construct_symmodel(void* pPackedKernel, void* pPackedParallelProgram){
		static pfacesParallelProgram*  pParallelProgram = (pfacesParallelProgram*)pPackedParallelProgram;
		static pFacesOmega* pKernel = (pFacesOmega*)pPackedKernel;

        size_t buff0Idx = pKernel->getBufferIndex(construct_symmodel_func_name, construct_symmodel_arg_names[0], pKernel->memReport);
		static concrete_t* pData	= (concrete_t*)pParallelProgram->m_dataPool[buff0Idx].first;

        size_t num_cons_in_struct = pKernel->size_struct_xu_posts / sizeof(concrete_t);
        size_t num_xu_concs = pKernel->xu_symbols * num_cons_in_struct;
		for (size_t i=0; i<num_xu_concs; i++){
			if(pData[i] == inf_val){
                throw std::runtime_error(
                    std::string("test_construct_symmodel: XU element ") +
                    std::to_string((size_t)(i/num_cons_in_struct)) +
                    std::string(" has element in struct xu_posts lefft unwritten.")
                );
            }
		}
        
        pfacesTerminal::showInfoMessage(
            std::string("test_construct_symmodel: Test completed.")
        );
		return 0;
	}
#endif
    
	/* to init the function */
	void pFacesOmega::init_construct_symmodel(){

		// size of the xu_posts struct in bytes
		size_struct_xu_posts = 2 * x_dim * sizeof(concrete_t);

		// get the initial state
		auto x_conc_initial = pfacesUtils::sStr2Vector<concrete_t>(m_spCfg->readConfigValueString("system.states.initial_state"));
		x_initial = OmegaUtils::Conc2Flat(x_conc_initial, x_qs, x_lb, x_widths);
		std::cout << std::endl << std::endl <<  "x_initial = " << x_initial << std::endl << std::endl;

		// fill the func info
    	func_info_construct_symmodel = 
        	std::make_pair<std::string, std::vector<arg_info>>(
				construct_symmodel_func_name.c_str(), {
					std::make_tuple(construct_symmodel_arg_names[0], size_struct_xu_posts, xu_symbols)
				});
	}

	/* add the function to the instruction list */
    void pFacesOmega::add_func_construct_symmodel(std::vector<std::shared_ptr<pfacesInstruction>>& instrList, const cl::Device& targetDevice){

		// index of the function and number of its arguments in the kernel
		size_t funcIdx = getKernelFunctionIndex(func_info_construct_symmodel.first);
		size_t numArgs = func_info_construct_symmodel.second.size();

		/* a message to declare start of execution */
		std::shared_ptr<pfacesInstruction> instrMsg_start_construct_symmodel = std::make_shared<pfacesInstruction>();
		instrMsg_start_construct_symmodel->setAsMessage("Constructing the symbolic model in parallel ... ");
		instrList.push_back(instrMsg_start_construct_symmodel);	

		/* an execute-instruction to launch parallel threads in the device */
		auto null_range = cl::NullRange;
		std::shared_ptr<pfacesDeviceExecuteTask> task = std::make_shared<pfacesDeviceExecuteTask>(ndKernelOffset, ndKernelRange_XU, null_range);
		std::shared_ptr<pfacesDeviceExecuteJob> job = std::make_shared<pfacesDeviceExecuteJob>(targetDevice);
		job->addTask(task);
		job->setKernelFunctionIdx(funcIdx, numArgs);		
		std::shared_ptr<pfacesInstruction> devFunctionInstruction = std::make_shared<pfacesInstruction>();
		devFunctionInstruction->setAsDeviceExecute(job);
		instrList.push_back(devFunctionInstruction);


		/* read the result memory buffer */
		std::shared_ptr<pfacesInstruction> instr_readArg0 = std::make_shared<pfacesInstruction>();
		instr_readArg0->setAsReadDeviceBuffer(
			std::make_shared<pfacesDeviceReadJob>(targetDevice, funcIdx, numArgs, 0));
		instrList.push_back(instr_readArg0);

#ifdef TEST_FUNCTION
		/* a sync point */
		std::shared_ptr<pfacesInstruction> instr_BlockingSyncPoint = std::make_shared<pfacesInstruction>();
		instr_BlockingSyncPoint->setAsBlockingSyncPoint();
		instrList.push_back(instr_BlockingSyncPoint);

		/* a host side function to read back the data */
		std::shared_ptr<pfacesInstruction> instr_hostTest = std::make_shared<pfacesInstruction>();
		instr_hostTest->setAsHostFunction(test_construct_symmodel, "test_construct_symmodel");
		instrList.push_back(instr_hostTest);
#endif
    }

	/* to init the function memory */
    void pFacesOmega::init_mem_construct_symmodel(){
#ifdef TEST_FUNCTION
    	// INF to all values
		size_t buff0Idx = getBufferIndex(construct_symmodel_func_name, construct_symmodel_arg_names[0], memReport);
		concrete_t* pBuff0 = (concrete_t*)dataPool[buff0Idx].first;

        size_t num_xu_concs = xu_symbols*(size_struct_xu_posts/sizeof(concrete_t));
		for (size_t i=0; i<num_xu_concs; i++){
			pBuff0[i] = inf_val;
		}
#endif	
	}
    
}