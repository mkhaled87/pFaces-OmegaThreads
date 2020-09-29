/*
 * func_discover_u_aps.cpp
 *
 *  Created on: 09.09.2020
 *      Author: M. Khaled
 */

#include "omega.h"

namespace pFacesOmegaKernels {
    
	/* constants to represent the function name and the names of its arguments */
	const size_t max_allowed_u_aps = 16;
	const std::string discover_u_aps_func_name = std::string("discover_u_aps");
	const std::vector<std::string> discover_u_aps_arg_names = {
		"u_aps_bags",
		"num_aps_subsets",
		"aps_subsets",
		"aps_subsets_assignments"
	};

#ifdef TEST_FUNCTION
	/* a host-side function to retrieve the memory ant test the output */
	size_t test_discover_u_aps(void* pPackedKernel, void* pPackedParallelProgram){
		static pfacesParallelProgram*  pParallelProgram = (pfacesParallelProgram*)pPackedParallelProgram;
		static pFacesOmega* pKernel = (pFacesOmega*)pPackedKernel;

		if(pKernel->dont_discover_u_aps)
			return 0;

		static size_t buff0Idx = pKernel->getBufferIndex(discover_u_aps_func_name, discover_u_aps_arg_names[0], pKernel->memReport);
		static cl_uint* pData	= (cl_uint*)pParallelProgram->m_dataPool[buff0Idx].first;
        size_t active_symbols = 0;
		for(size_t i=0; i<pKernel->u_symbols; i++){
			if(pData[i] != 0){
				std::vector<concrete_t> u_conc(pKernel->u_dim);
				OmegaUtils::Flat2Conc(u_conc, i, pKernel->u_widths, pKernel->u_lb, pKernel->u_qs);
				
				for(char p=0; p<(char)pKernel->u_aps.size(); p++){
					cl_uint ap_mask = 1 << p;

					// is this x \in AP[p] ?
					if(ap_mask & pData[i]){

                        active_symbols++;

						bool found_in_a_subset = false;
						// we double check it belongs to any of its hyperrects
						for (size_t s=0; s<pKernel->u_aps_subsets_map.size(); s++){
							if(pKernel->u_aps_subsets_map[s] == p){
								auto lb = pKernel->u_aps_subsets[s].first;
								auto ub = pKernel->u_aps_subsets[s].second;

								bool inside_subset = true;
								for(size_t d=0; d<pKernel->u_dim; d++){
									if(u_conc[d] < lb[d] || u_conc[d] > ub[d]){
										inside_subset = false;
									}
								}
								if(inside_subset)
									found_in_a_subset = true;
							}
						}

						if(!found_in_a_subset)
							throw std::runtime_error(
									std::string("test_discover_u_aps: Stat flat=") + std::to_string(i) + 
									std::string(", conc=") + pfacesUtils::vector2string(u_conc) +
									std::string(" supposed to have mask=") + std::to_string(pData[i]) + 
									std::string(" does not belong to any of the subsets of the corresponding AP.")
								);

					}
				}
 			}
		}
        
        pfacesTerminal::showInfoMessage(
            std::string("test_discover_u_aps: Test completed. ") + std::to_string(pKernel->u_symbols) +
            std::string(" U symbols have been tested. ") + std::to_string(active_symbols) +
            std::string(" symbols are found and tested for belonging to some AP.")
        );

		return 0;
	}
#endif

	/* write lb/ub vectors to a struct (is_ap_subset) in the memory */
	void pFacesOmega::write_is_ap_subset(char* pData, const std::vector<concrete_t> lb, const std::vector<concrete_t> ub){
		concrete_t* pItem = (concrete_t*)pData;

		for(size_t i=0; i<u_dim; i++){
			pItem[i] = lb[i];
		}
		for(size_t i=0; i<u_dim; i++){
			pItem[lb.size() + i] = ub[i];
		}
	}
    
	/* to init the function */
	void pFacesOmega::init_discover_u_aps(){

		// size of the subset struct in bytes
		size_struct_u_ap_subset = 2 * u_dim * sizeof(concrete_t);		

		// read the APs
		std::string u_aps_names = m_spCfg->readConfigValueString("system.controls.subsets.names");
		auto u_aps_temp = pfacesUtils::strSplit(u_aps_names, ",", false);
		if(u_aps_temp.size() == 0){
			dont_discover_u_aps = true;
			return;
		}
		if(u_aps_temp.size() >= max_allowed_u_aps)
			throw std::runtime_error(
				std::string("pFacesOmega::init_discover_u_aps: Number of input (U) atomic proposition exceeds the maximum=") + std::to_string(max_allowed_u_aps)
				);

		// read the subsets
		for (std::string ap : u_aps_temp){

			std::string hyperrects_line = m_spCfg->readConfigValueString(
				std::string("system.controls.subsets.mapping_") + 
				pfacesUtils::strTrim(ap)
			);

			char ap_idx = u_aps.size();
			auto hrs = OmegaUtils::extractHyperrects(hyperrects_line, u_dim);
			for(auto hr : hrs){
				u_aps_subsets.push_back(hr);
				u_aps_subsets_map.push_back(ap_idx);
			}

			u_aps.push_back(pfacesUtils::strTrim(ap));
		}

		// fill the func info
    	func_info_discover_u_aps = 
        	std::make_pair<std::string, std::vector<arg_info>>(
				discover_u_aps_func_name.c_str(), {
					std::make_tuple(discover_u_aps_arg_names[0], sizeof(cl_uint), u_symbols),
					std::make_tuple(discover_u_aps_arg_names[1], sizeof(cl_uint), 1),
					std::make_tuple(discover_u_aps_arg_names[2], size_struct_u_ap_subset, u_aps_subsets.size()),
					std::make_tuple(discover_u_aps_arg_names[3], sizeof(char), u_aps_subsets.size()),
				});
	}

	/* add the function to the instruction list */
    void pFacesOmega::add_func_discover_u_aps(std::vector<std::shared_ptr<pfacesInstruction>>& instrList, const cl::Device& targetDevice){

		if(dont_discover_u_aps)
			return;

		// index of the function and number of its arguments in the kernel
		size_t funcIdx = getKernelFunctionIndex(func_info_discover_u_aps.first);
		size_t numArgs = func_info_discover_u_aps.second.size();

		/* a message to declare start of execution */
		std::shared_ptr<pfacesInstruction> instrMsg_start_discover_u_aps = std::make_shared<pfacesInstruction>();
		instrMsg_start_discover_u_aps->setAsMessage("Identifying, in parallel, the input (U) atomic propositions ... ");
		instrList.push_back(instrMsg_start_discover_u_aps);	


		/* write the memory buffers */
		std::shared_ptr<pfacesInstruction> instr_writeArg1 = std::make_shared<pfacesInstruction>();
		std::shared_ptr<pfacesInstruction> instr_writeArg2 = std::make_shared<pfacesInstruction>();
		std::shared_ptr<pfacesInstruction> instr_writeArg3 = std::make_shared<pfacesInstruction>();
		instr_writeArg1->setAsWriteDeviceBuffer(
			std::make_shared<pfacesDeviceWriteJob>(targetDevice, funcIdx, numArgs, 1));
		instr_writeArg2->setAsWriteDeviceBuffer(
			std::make_shared<pfacesDeviceWriteJob>(targetDevice, funcIdx, numArgs, 2));
		instr_writeArg3->setAsWriteDeviceBuffer(
			std::make_shared<pfacesDeviceWriteJob>(targetDevice, funcIdx, numArgs, 3));
		instrList.push_back(instr_writeArg1);
		instrList.push_back(instr_writeArg2);
		instrList.push_back(instr_writeArg3);

		/* an execute-instruction to launch parallel threads in the device */
		auto null_range = cl::NullRange;
		std::shared_ptr<pfacesDeviceExecuteTask> task = std::make_shared<pfacesDeviceExecuteTask>(ndKernelOffset, ndKernelRange_U, null_range);
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
		instr_hostTest->setAsHostFunction(test_discover_u_aps, "test_discover_u_aps");
		instrList.push_back(instr_hostTest);
#endif
    }

	/* to init the function memory */
    void pFacesOmega::init_mem_discover_u_aps(){
		
		if(dont_discover_u_aps)
			return;

		// write to num_aps_subsets
		size_t buff1Idx = getBufferIndex(discover_u_aps_func_name, discover_u_aps_arg_names[1], memReport);
		cl_uint* pBuff1 = (cl_uint*)dataPool[buff1Idx].first;
		*pBuff1 = u_aps_subsets.size();

		// write to aps_subsets[]
		size_t buff2Idx = getBufferIndex(discover_u_aps_func_name, discover_u_aps_arg_names[2], memReport);
		char* pBuff2 = (char*)dataPool[buff2Idx].first;
		for (size_t i=0; i<u_aps_subsets.size(); i++){
			write_is_ap_subset(pBuff2, u_aps_subsets[i].first, u_aps_subsets[i].second);
			pBuff2 += size_struct_u_ap_subset;
		}

		// write to aps_subsets_assignments[]
		size_t buff3Idx = getBufferIndex(discover_u_aps_func_name, discover_u_aps_arg_names[3], memReport);
		char* pBuff3 = (char*)dataPool[buff3Idx].first;
		for (size_t i=0; i<u_aps_subsets.size(); i++)
			pBuff3[i] = u_aps_subsets_map[i];
		
	}
    
}