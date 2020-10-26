/*
 * func_construct_symmodel.cpp
 *
 *  Created on: 10.09.2020
 *      Author: M. Khaled
 */

#include "omega.h"

// uncomment this to get the parallel implementation (Experimental!)
//#define PARALLEL_IMPLEMENTATION


namespace pFacesOmegaKernels {

#ifndef PARALLEL_IMPLEMENTATION
	/* a function to retrive the posts (must conform with omegaControlProblem.h) */
	std::vector<symbolic_t> get_sym_posts(symbolic_t x_flat, symbolic_t u_flat){
		
		static size_t buff0Idx = pKernel->getBufferIndex(construct_symmodel_func_name, construct_symmodel_arg_names[0], pKernel->memReport);
		static concrete_t* pData	= (concrete_t*)pKernel->dataPool[buff0Idx].first;
		static const std::vector<concrete_t>& ssLb		= pKernel->x_lb;
		static const std::vector<concrete_t>& ssUb		= pKernel->x_ub;
		static const std::vector<concrete_t>& ssEta		= pKernel->x_qs;
		static const std::vector<symbolic_t>& ssWidths	= pKernel->x_widths;
		static size_t ssDim = pKernel->x_dim;
		static size_t num_cons_in_xu_struct = pKernel->size_struct_xu_posts / sizeof(concrete_t);
		
		static bool calledBefore = false;
		static std::vector<symbolic_t> first_cell_in_dim;
		static std::vector<symbolic_t> dim_posts_count;
		static std::vector<symbolic_t> postcell;
		static std::vector<concrete_t> rectLb;
		static std::vector<concrete_t> rectUb;

		// prepare some required static data
		if (!calledBefore) {

			first_cell_in_dim.resize(ssDim);
			dim_posts_count.resize(ssDim);
			postcell.resize(ssDim);
			rectLb.resize(ssDim);
			rectUb.resize(ssDim);

			for (size_t i = 0; i<ssDim; i++) {
				if (i == 0)
					first_cell_in_dim[i] = 1;
				else {
					first_cell_in_dim[i] = first_cell_in_dim[i - 1] * ssWidths[i - 1];
				}
			}
			calledBefore = true;
		}

		// get the lb/ub of OARS rectangle using x_flat and u_flat
		bool out_of_domain = false;
		size_t xu_flat = x_flat*pKernel->u_symbols + u_flat;
		for (size_t i=0; i<ssDim; i++){
			rectLb[i] = pData[xu_flat*num_cons_in_xu_struct + i];
			rectUb[i] = pData[xu_flat*num_cons_in_xu_struct + ssDim + i];

			if(rectLb[i] < ssLb[i] || rectUb[i] > ssUb[i]){
				out_of_domain = true;
				break;
			}
		}
		if(out_of_domain){
			return {std::numeric_limits<symbolic_t>::max()};
		}

		// Now, we have to symbolize the OARS
		std::vector<symbolic_t> sym_lb = OmegaUtils::Conc2Symbolic(rectLb, ssEta, ssLb);
		std::vector<symbolic_t> sym_ub = OmegaUtils::Conc2Symbolic(rectUb, ssEta, ssLb);
		symbolic_t n_rect_symbols = 1;
		for (unsigned int j = 0; j<ssDim; j++) {
			/* compute width per dimension */
			dim_posts_count[j] = sym_ub[j] - sym_lb[j] + 1;

			/* update number of posts */
			n_rect_symbols *= dim_posts_count[j];

			/* initialize postcell */
			postcell[j] = 0;
		}		

		// collect posts
		std::vector<symbolic_t> ret(n_rect_symbols);
		for (size_t p = 0; p < n_rect_symbols; p++)
		{
			/* compute the symbolic index of the post from its flat index */
			size_t post_x_flat = 0;
			for (size_t i = 0; i<ssDim; i++)
				post_x_flat += (sym_lb[i] + postcell[i])* first_cell_in_dim[i];

			ret[p] = post_x_flat;

			// prepare the counters for other posts
			postcell[0]++;
			for (size_t i = 0; i<ssDim - 1; i++) {
				if (postcell[i] == dim_posts_count[i]) {
					postcell[i] = 0;
					postcell[i + 1]++;
				}
			}
		}		

		
		return ret;
	}

	/* map L_x returns the APs mask for a state x (must conform with omegaControlProblem.h) */
	symbolic_t L_x(symbolic_t x){
		static size_t buff0Idx = pKernel->getBufferIndex(discover_x_aps_func_name, discover_x_aps_arg_names[0], pKernel->memReport);
		static cl_uint* pMasks	= (cl_uint*)pKernel->dataPool[buff0Idx].first;
		
		if(x >= pKernel->x_symbols)
			throw std::runtime_error(
				std::string("pFacesOmega::L_x: invalid symbolic state x=") +
				std::to_string(x)
			);
		
		return (symbolic_t)pMasks[x];
	}    

	/* map L_u returns the APs mask for a control u (must conform with omegaControlProblem.h) */
	symbolic_t L_u(symbolic_t u){
		static size_t buff0Idx = pKernel->getBufferIndex(discover_u_aps_func_name, discover_u_aps_arg_names[0], pKernel->memReport);
		static cl_uint* pMasks	= (cl_uint*)pKernel->dataPool[buff0Idx].first;
		
		if(u >= pKernel->u_symbols)
			throw std::runtime_error(
				std::string("pFacesOmega::L_u: invalid symbolic state u=") +
				std::to_string(u)
			);
		
		return (symbolic_t)pMasks[u];
	}
#endif
    /* to init the function */
    void pFacesOmega::init_construct_pgame(){

        // create the specs wrapper
        std::string ltl_formula = m_spCfg->readConfigValueString("specifications.ltl_formula");
		std::string dpa_file = m_spCfg->readConfigValueString("specifications.dpa_file");
		bool write_dpa = m_spCfg->readConfigValueBool("specifications.write_dpa");

		if(!dpa_file.empty() && !ltl_formula.empty())
			throw std::runtime_error("pFacesOmega::init_construct_pgame: you have to provide either dpa_file or ltl_formula as specification but not both at the same time.");

#ifdef TEST_FUNCTION
		pfacesTimer tmr_dpa;
		tmr_dpa.tic();
#endif


		if(!ltl_formula.empty())
        	pSymSpec = std::make_shared<SymSpec<L_x_func_t, L_u_func_t>>(x_aps, u_aps, ltl_formula, L_x, L_u);
		else if(!dpa_file.empty())
			pSymSpec = std::make_shared<SymSpec<L_x_func_t, L_u_func_t>>(dpa_file, L_x, L_u);			
		else
			throw std::runtime_error("pFacesOmega::init_construct_pgame: no valid specification is provided in the config file.");

#ifdef TEST_FUNCTION
		auto time_dpa = tmr_dpa.toc();
#endif			

		if(write_dpa){
			std::string dpa_file = 
				pfacesFileIO::getFileDirectoryPath(m_spCfg->getConfigFilePath()) + 
				m_spCfg->readConfigValueString("project_name") + 
				std::string(".dpa");

			pSymSpec->dpa.writeToFile(dpa_file);
		}			

#ifdef TEST_FUNCTION		
        pfacesTerminal::showInfoMessage(
            std::string("The DPA is constructed in ") +
			std::to_string(time_dpa.count()) + 
			std::string(" seconds and it has ") + 
			std::to_string(pSymSpec->count_DPA_states()) +
            std::string(" states. ")
        );
#endif

#ifndef PARALLEL_IMPLEMENTATION
		// create the sym-model wrapper
        pSymModel = std::make_shared<SymModel<post_func_t>>(x_symbols, u_symbols, initial_states, get_sym_posts);

		// create the parity game
		pParityGame = std::make_shared<PGame<post_func_t, L_x_func_t, L_u_func_t>>(*pSymSpec, *pSymModel);
#endif
    }

#ifndef PARALLEL_IMPLEMENTATION
	size_t construct_pgame(void* pPackedKernel, void* pPackedParallelProgram){
		
		(void)pPackedParallelProgram;
		static pFacesOmega* pKernel = (pFacesOmega*)pPackedKernel;
		pKernel->pParityGame->constructArena();

#ifdef TEST_FUNCTION		
        pfacesTerminal::showInfoMessage(
            std::string("construct_pgame: The PGame has ") + 
			std::to_string(pKernel->pParityGame->get_n_env_nodes()) + std::string(" model nodes, ") +
			std::to_string(pKernel->pParityGame->get_n_env_edges()) + std::string(" model edges, ") +
			std::to_string(pKernel->pParityGame->get_n_sys_nodes()) + std::string(" controller nodes, and ") +
			std::to_string(pKernel->pParityGame->get_n_sys_edges()) + std::string(" controller edges.")
        );
#endif

		return 0;
	}
#endif

    /* add the function to the instruction list */
    void pFacesOmega::add_func_construct_pgame(std::vector<std::shared_ptr<pfacesInstruction>>& instrList, const cl::Device& targetDevice){

        (void)targetDevice;

		/* a message to declare start of execution */
		std::shared_ptr<pfacesInstruction> instrMsg_start_construct_pgame = std::make_shared<pfacesInstruction>();
		instrMsg_start_construct_pgame->setAsMessage("Constructing the parity game ... ");
		instrList.push_back(instrMsg_start_construct_pgame);	

#ifndef PARALLEL_IMPLEMENTATION
		/* a sync point */
		std::shared_ptr<pfacesInstruction> instr_BlockingSyncPoint = std::make_shared<pfacesInstruction>();
		instr_BlockingSyncPoint->setAsBlockingSyncPoint();
		instrList.push_back(instr_BlockingSyncPoint);

		/* a host side function to construct the PGame */
		/* TODO: replace with parallel implementation */
		std::shared_ptr<pfacesInstruction> instr_hostConstructPGame = std::make_shared<pfacesInstruction>();
		instr_hostConstructPGame->setAsHostFunction(construct_pgame, "construct_pgame");
		instrList.push_back(instr_hostConstructPGame);	
#endif	

    }    
}