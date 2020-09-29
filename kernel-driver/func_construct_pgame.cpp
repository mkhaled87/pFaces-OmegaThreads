/*
 * func_construct_symmodel.cpp
 *
 *  Created on: 10.09.2020
 *      Author: M. Khaled
 */

#include "omega.h"

namespace pFacesOmegaKernels {


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
		std::vector<symbolic_t> ret;
		for (size_t p = 0; p < n_rect_symbols; p++)
		{
			/* compute the symbolic index of the post from its flat index */
			size_t post_x_flat = 0;
			for (size_t i = 0; i<ssDim; i++)
				post_x_flat += (sym_lb[i] + postcell[i])* first_cell_in_dim[i];

			ret.push_back(post_x_flat);

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

    /* to init the function */
    void pFacesOmega::init_construct_pgame(){

        // create the specs wrapper
        std::string ltl_formula = m_spCfg->readConfigValueString("specifications.ltl_formula");
		bool write_dpa = m_spCfg->readConfigValueBool("specifications.write_dpa");
        pSymSpec = std::make_shared<SymSpec<L_x_func_t, L_u_func_t>>(x_aps, u_aps, ltl_formula, L_x, L_u);
		std::cout << "The DPA has " << pSymSpec->count_DPA_states() << " states" << std::endl;

        // create the sym-model wrapper
        pSymModel = std::make_shared<SymModel<post_func_t>>(x_symbols, u_symbols, x_initial, get_sym_posts);

		if(write_dpa){
			std::string dpa_file = 
				pfacesFileIO::getFileDirectoryPath(m_spCfg->getConfigFilePath()) + 
				m_spCfg->readConfigValueString("project_name") + 
				std::string(".dpa");

			pSymSpec->dpa.writeToFile(dpa_file);
		}

		// create the parity game
		pParityGame = std::make_shared<PGame<post_func_t, L_x_func_t, L_u_func_t>>(*pSymSpec, *pSymModel);
    }

	size_t construct_pgame(void* pPackedKernel, void* pPackedParallelProgram){
		
		(void)pPackedParallelProgram;
		static pFacesOmega* pKernel = (pFacesOmega*)pPackedKernel;
		pKernel->pParityGame->constructArena();
		return 0;
	}

    /* add the function to the instruction list */
    void pFacesOmega::add_func_construct_pgame(std::vector<std::shared_ptr<pfacesInstruction>>& instrList, const cl::Device& targetDevice){

        (void)targetDevice;

		/* a message to declare start of execution */
		std::shared_ptr<pfacesInstruction> instrMsg_start_construct_pgame = std::make_shared<pfacesInstruction>();
		instrMsg_start_construct_pgame->setAsMessage("Constructing the parity game ... ");
		instrList.push_back(instrMsg_start_construct_pgame);	

		/* a sync point */
		std::shared_ptr<pfacesInstruction> instr_BlockingSyncPoint = std::make_shared<pfacesInstruction>();
		instr_BlockingSyncPoint->setAsBlockingSyncPoint();
		instrList.push_back(instr_BlockingSyncPoint);

		/* a host side function to construct the PGame */
		/* TODO: replace with parallel implementation */
		std::shared_ptr<pfacesInstruction> instr_hostConstructPGame = std::make_shared<pfacesInstruction>();
		instr_hostConstructPGame->setAsHostFunction(construct_pgame, "construct_pgame");
		instrList.push_back(instr_hostConstructPGame);		

    }    
}