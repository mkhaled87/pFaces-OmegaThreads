/*
 * func_implement.cpp
 *
 *  Created on: 16.09.2020
 *      Author: M. Khaled
 */

#include "omega.h"

namespace pFacesOmegaKernels {

    // gen controller ?
    bool gen_controller = false;

    /* to init the function */
    void pFacesOmega::init_implement(){

        gen_controller = m_spCfg->readConfigValueBool("implementation.generate_controller");

        if(!gen_controller)
            return;

        std::string str_impl_type = m_spCfg->readConfigValueString("implementation.type");
        MachineSemantic impl_type;
        if(str_impl_type == std::string("mealy_machine"))
            impl_type = MachineSemantic::MEALY;
        else if (str_impl_type == std::string("moore_machine"))
            impl_type = MachineSemantic::MOORE;
        else
            throw std::runtime_error("Invalid implementation type in config file");

		// initiate a machine to cotain the implementation
        std::vector<std::string> in_vars = pKernel->pSymSpec->get_state_APs();
        std::vector<std::string> out_vars = pKernel->pSymSpec->get_control_APs();
        pControllerImpl = std::make_shared<Machine<post_func_t, L_x_func_t, L_u_func_t>>(impl_type);
    }    

    size_t implement(void* pPackedKernel, void* pPackedParallelProgram){

        static pFacesOmega* pKernel = (pFacesOmega*)pPackedKernel;
        static pfacesParallelProgram*  pParallelProgram = (pfacesParallelProgram*)pPackedParallelProgram;

        pKernel->pControllerImpl->construct(*pKernel->pParityGame, *pKernel->pParityGameSolver);

        std::string config_dir = pfacesFileIO::getFileDirectoryPath(pParallelProgram->m_spCfgReader->getConfigFilePath());
        std::string target_file = config_dir + pParallelProgram->m_spCfgReader->readConfigValueString("project_name") +  std::string(".mdf");

        pKernel->pControllerImpl->writeToFile(target_file);


        //  gen code ?
        bool gen_code = pParallelProgram->m_spCfgReader->readConfigValueBool("implementation.generate_code");
        if(gen_code){

            std::string module_name = pParallelProgram->m_spCfgReader->readConfigValueString("implementation.module_name");
            CodeTypes code_type = MachineCodeGenerator::parse_code_type(pParallelProgram->m_spCfgReader->readConfigValueString("implementation.code_type"));
            MachineCodeGenerator codeGenerator(pKernel->kernel_path, module_name, config_dir);
            codeGenerator.machine2code(pKernel->pControllerImpl->getTransitions(), code_type);
        }

		return 0;
	}

    /* add the function to the instruction list */
    void pFacesOmega::add_func_implement(std::vector<std::shared_ptr<pfacesInstruction>>& instrList, const cl::Device& targetDevice){

        (void)targetDevice;

        if(!gen_controller)
            return;

		/* skip the  next if no controller was found */
        std::shared_ptr<pfacesInstruction> instr_jumpSkipGenController = std::make_shared<pfacesInstruction>();
		instr_jumpSkipGenController->setAsJumpNe(instrList.size() + 5);
        instrList.push_back(instr_jumpSkipGenController);

		/* a message to declare start of execution */
		std::shared_ptr<pfacesInstruction> instrMsg_start_implement = std::make_shared<pfacesInstruction>();
		instrMsg_start_implement->setAsMessage("Generating the controller implementation ... ");
		instrList.push_back(instrMsg_start_implement);

		/* a sync point */
		std::shared_ptr<pfacesInstruction> instr_BlockingSyncPoint = std::make_shared<pfacesInstruction>();
		instr_BlockingSyncPoint->setAsBlockingSyncPoint();
		instrList.push_back(instr_BlockingSyncPoint);

		/* a host side function to construct the PGame */
		/* TODO: replace with parallel implementation */
		std::shared_ptr<pfacesInstruction> instr_hostImplement = std::make_shared<pfacesInstruction>();
		instr_hostImplement->setAsHostFunction(implement, "implement");
		instrList.push_back(instr_hostImplement);		

		/* skip the next failure message */
        std::shared_ptr<pfacesInstruction> instr_jumpSkipFailMsg = std::make_shared<pfacesInstruction>();
		instr_jumpSkipFailMsg->setAsJumpUnconditional(instrList.size() + 2);
        instrList.push_back(instr_jumpSkipFailMsg);        

        /* a message to declare failure to find a controller */
		std::shared_ptr<pfacesInstruction> instrMsg_failed = std::make_shared<pfacesInstruction>();
		instrMsg_failed->setAsMessage("A controller could not be found !");
		instrList.push_back(instrMsg_failed);

    }    
}