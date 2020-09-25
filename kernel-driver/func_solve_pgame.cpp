/*
 * func_construct_symmodel.cpp
 *
 *  Created on: 10.09.2020
 *      Author: M. Khaled
 */

#include "omega.h"

namespace pFacesOmegaKernels {

    /* to init the function */
    void pFacesOmega::init_solve_pgame(){

		// create the parity game solver
        pParityGameSolver = std::make_shared<PGSISolver<post_func_t, L_x_func_t, L_u_func_t>>(*pParityGame);
    }

	size_t solve_pgame(void* pPackedKernel, void* pPackedParallelProgram){
        
        (void)pPackedParallelProgram;
		static pFacesOmega* pKernel = (pFacesOmega*)pPackedKernel;
		pKernel->pParityGameSolver->solve();
		strix_aut::Player winner = pKernel->pParityGameSolver->getWinner();
		if(winner == strix_aut::Player::SYS_PLAYER){
			pfacesTerminal::showInfoMessage("solve_pgame: A controller is found !");
			pKernel->is_realizable = true;
			return 0;
		}
		else {
			pfacesTerminal::showInfoMessage("solve_pgame: A controller is not found !");
			pKernel->is_realizable = false;
			return 1;
		}
	}

    /* add the function to the instruction list */
    void pFacesOmega::add_func_solve_pgame(std::vector<std::shared_ptr<pfacesInstruction>>& instrList, const cl::Device& targetDevice){

        (void)targetDevice;

		/* a message to declare start of execution */
		std::shared_ptr<pfacesInstruction> instrMsg_start_solve_pgame = std::make_shared<pfacesInstruction>();
		instrMsg_start_solve_pgame->setAsMessage("Solving the parity game ... ");
		instrList.push_back(instrMsg_start_solve_pgame);	

		/* a sync point */
		std::shared_ptr<pfacesInstruction> instr_BlockingSyncPoint = std::make_shared<pfacesInstruction>();
		instr_BlockingSyncPoint->setAsBlockingSyncPoint();
		instrList.push_back(instr_BlockingSyncPoint);

		/* a host side function to solve the PGame */
        /* TODO: replace with parallel implementation */
		std::shared_ptr<pfacesInstruction> instr_hostSolvePGame = std::make_shared<pfacesInstruction>();
		instr_hostSolvePGame->setAsHostFunction(solve_pgame, "solve_pgame");
		instrList.push_back(instr_hostSolvePGame);	
    }    
}