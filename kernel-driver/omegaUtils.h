/*
 * omegaUtils.h
 *
 *  Created on: 27.08.2020
 *      Author: M. Khaled
 */
#pragma once
#include <pfaces-sdk.h>

namespace pFacesOmegaKernels {

    // special types
    typedef std::pair<std::vector<concrete_t>,std::vector<concrete_t>> hyperrect;

    // a class for utility functions
	class OmegaUtils{
	public:

		static
		std::vector<hyperrect> extractHyperrects(
			const std::string& hyperrects_line, 
			size_t dim);

		static
		std::vector<symbolic_t>  getWidths(
			const std::vector<concrete_t>& lb,
			const std::vector<concrete_t>& eta,
			const std::vector<concrete_t>& ub);

		static
		void Flat2Conc(
			std::vector<concrete_t>& ret, 
			const symbolic_t& flat_val, 
			const std::vector<symbolic_t>& dim_width,
			const std::vector<concrete_t>& lb,
			const std::vector<concrete_t>& eta);

		static
		std::vector<symbolic_t> Conc2Symbolic(
			const std::vector<concrete_t>& x_conc, 
			const std::vector<concrete_t>& x_eta, 
			const std::vector<concrete_t>& x_lb);

		static
		symbolic_t Conc2Flat(
			const std::vector<concrete_t>& conc, 
			const std::vector<concrete_t>& eta, 
			const std::vector<concrete_t>& lb, 
			const std::vector<symbolic_t>& widths);

		static 
		std::vector<symbolic_t> GetSymbolsInHyperrect(
			const hyperrect& hr,
			const std::vector<concrete_t>& x_eta, 
			const std::vector<concrete_t>& x_lb,
			const std::vector<symbolic_t>& x_widths);

        // print a vector of anything
        template<class T>
        static void print_vector(const std::vector<T>& vec, std::ostream& ost = std::cout);

        // scan any vector
        template<class T>
        static void scan_vector(std::vector<T>& vec, std::istream& ist);
	};
}
