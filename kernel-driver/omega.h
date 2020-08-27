/*
 * omega.h
 *
 *  Created on: 27.08.2020
 *      Author: M. Khaled
 */
#pragma once
#include <pfaces-sdk.h>

namespace pFacesOmegaKernels{

	// class: pFacesOmega, a 2d-kernel
	class pFacesOmega : public pfaces2DKernel {
	private:
		const std::shared_ptr<pfacesConfigurationReader> m_spCfg;
	
	public:
		pFacesOmega(
			const std::shared_ptr<pfacesKernelLaunchState>& spLaunchState, 
			const std::shared_ptr<pfacesConfigurationReader>& spCfg);
			
		~pFacesOmega() = default;

		void configureParallelProgram(
			pfacesParallelProgram& parallelProgram);

		void configureTuneParallelProgram(
			pfacesParallelProgram& tuneParallelProgram, 
			size_t targetFunctionIdx);

	};

}
