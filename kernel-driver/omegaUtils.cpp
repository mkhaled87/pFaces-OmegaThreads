#include "omega.h"

namespace pFacesOmegaKernels{

    std::vector<hyperrect> OmegaUtils::extractHyperrects(const std::string& hyperrects_line, size_t dim){
        
        std::vector<hyperrect> ret;

        // split based on union
        auto h_rects = pfacesUtils::strSplit(hyperrects_line, "U", false);

        // now we have a valid subset as intervals, let's add it/them
        for (auto h_rect : h_rects){

            auto str_intervals = pfacesUtils::strSplit(h_rect, "x", false);
            if(str_intervals.size() != dim)
                throw std::runtime_error(
                    std::string("pFacesOmega::init_discover_x_aps: Invalid number of intervals for hyperrect: ") + h_rect);
            
            std::vector<concrete_t> lb,ub;
            for(size_t i=0; i<dim; i++){
                str_intervals[i] = pfacesUtils::strReplaceAll(str_intervals[i], " ", "");
                str_intervals[i] = pfacesUtils::strReplaceAll(str_intervals[i], "[", "");
                str_intervals[i] = pfacesUtils::strReplaceAll(str_intervals[i], "]", "");
                auto interval = pfacesUtils::sStr2Vector<concrete_t>(str_intervals[i]);
                
                if(interval.size() != 2)
                    throw std::runtime_error(
                        std::string("pFacesOmega::init_discover_x_aps: Each interval in any hyperrect should have only to elements: ") + h_rect);

                lb.push_back(interval[0]);
                ub.push_back(interval[1]);
            }

            hyperrect hr = std::make_pair(lb,ub);
            ret.push_back(hr);
        }
        return ret;
    }

    std::vector<symbolic_t>  OmegaUtils::getWidths(
        const std::vector<concrete_t>& lb,
        const std::vector<concrete_t>& eta,
        const std::vector<concrete_t>& ub){

        std::vector<symbolic_t> ret;

        for(unsigned int i=0; i<lb.size(); i++){
            symbolic_t tmp = std::floor((ub[i]-lb[i])/eta[i])+1;
		    ret.push_back(tmp);
        }

        return ret;
    }

    void OmegaUtils::Flat2Conc(
        std::vector<concrete_t>& ret, 
        const symbolic_t& flat_val, 
        const std::vector<symbolic_t>& dim_width,
        const std::vector<concrete_t>& lb,
        const std::vector<concrete_t>& eta) {

        symbolic_t fltCurrent;
        symbolic_t fltIntial;
        symbolic_t fltVolume;
        symbolic_t fltTmp;


        fltIntial = flat_val;
        for (int i = ((int)dim_width.size()) - 1; i >= 0; i--) {
            fltCurrent = fltIntial;

            fltVolume = 1;
            for (int k = 0; k<i; k++) {
                fltTmp = dim_width[k];
                fltVolume = fltVolume*fltTmp;
            }

            fltCurrent = fltCurrent / fltVolume;
            fltTmp = dim_width[i];
            fltCurrent = fltCurrent%fltTmp;

            ret[i] = lb[i] + eta[i]*fltCurrent;

            fltCurrent = fltCurrent * fltVolume;
            fltIntial = fltIntial - fltCurrent;
        }
    }

    std::vector<symbolic_t> OmegaUtils::Conc2Symbolic(
        const std::vector<concrete_t>& x_conc, 
        const std::vector<concrete_t>& x_eta, 
        const std::vector<concrete_t>& x_lb
    ){
        std::vector<symbolic_t> x_sym(x_conc.size());
        for (size_t i = 0; i<x_conc.size(); i++) {
            x_sym[i] = (x_conc[i] - x_lb[i] + x_eta[i]/2.0)/x_eta[i];
        }
        return x_sym;
    }

    symbolic_t OmegaUtils::Conc2Flat(
        const std::vector<concrete_t>& x_conc, 
        const std::vector<concrete_t>& x_eta, 
        const std::vector<concrete_t>& x_lb, 
        const std::vector<symbolic_t>& x_widths) {
        

        std::vector<symbolic_t> x_sym(x_eta.size());
        for (size_t i = 0; i<x_eta.size(); i++) {
            x_sym[i] = (x_conc[i] - x_lb[i] + x_eta[i]/2.0)/x_eta[i];
        }
        
        symbolic_t fltTmpVolume;
        symbolic_t fltTmp;
        symbolic_t x_flat = 0 ;
        for (size_t i = 0; i<x_eta.size(); i++) {
            fltTmpVolume = 1;
            for (size_t j = 0; j<i; j++) {
                fltTmpVolume *= x_widths[j];
            }
            fltTmp = x_sym[i];
            fltTmp *= fltTmpVolume;
            x_flat += fltTmp;
        }

        return x_flat;
    }

    std::vector<symbolic_t> OmegaUtils::GetSymbolsInHyperrect(
        const hyperrect& hr,
        const std::vector<concrete_t>& x_eta, 
        const std::vector<concrete_t>& x_lb,
        const std::vector<symbolic_t>& x_widths){

        size_t ssDim = x_eta.size();
        std::vector<symbolic_t> sym_lb = OmegaUtils::Conc2Symbolic(hr.first, x_eta, x_lb);
		std::vector<symbolic_t> sym_ub = OmegaUtils::Conc2Symbolic(hr.second, x_eta, x_lb);
		symbolic_t n_rect_symbols = 1;
        std::vector<symbolic_t> dim_posts_count(ssDim);
        std::vector<symbolic_t> postcell(ssDim);
        std::vector<symbolic_t> first_cell_in_dim(ssDim);
		for (unsigned int j = 0; j<ssDim; j++) {
			/* compute width per dimension */
			dim_posts_count[j] = sym_ub[j] - sym_lb[j] + 1;

			/* update number of posts */
			n_rect_symbols *= dim_posts_count[j];

			/* initialize postcell */
			postcell[j] = 0;

            /* first cell in dim */
            if (j == 0)
                first_cell_in_dim[j] = 1;
            else {
                first_cell_in_dim[j] = first_cell_in_dim[j - 1] * x_widths[j - 1];
            }            
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


    // override istream to use if fot reading an OWL enum type for statuses from files
    std::istream& operator >> (std::istream& i, atomic_proposition_status_t& status){
        int value;
        i >> value;
        status = (atomic_proposition_status_t)value;
        return i;
    }

    template<class T>
    void OmegaUtils::print_vector(const std::vector<T>& vec, std::ostream& ost){
        ost << "{";
        size_t idx = 0;
        for (auto str : vec){
            ost << str;
            idx++;
            if(vec.size() != idx)
                ost << ",";
        }
        ost << "}";
    }
    template void OmegaUtils::print_vector(const std::vector<std::string>& vec, std::ostream& ost);
    template void OmegaUtils::print_vector(const std::vector<bool>& vec, std::ostream& ost);
    template void OmegaUtils::print_vector(const std::vector<uint32_t>& vec, std::ostream& ost);
    template void OmegaUtils::print_vector(const std::vector<int32_t>& vec, std::ostream& ost);
    template void OmegaUtils::print_vector(const std::vector<uint64_t>& vec, std::ostream& ost);
    template void OmegaUtils::print_vector(const std::vector<int64_t>& vec, std::ostream& ost);
    template void OmegaUtils::print_vector(const std::vector<atomic_proposition_status_t>& vec, std::ostream& ost);
    


    template<class T>
    void OmegaUtils::scan_vector(std::vector<T>& vec, std::istream& ist){

        std::string scanned_line;
        std::getline(ist, scanned_line);

        scanned_line = pfacesUtils::strTrim(scanned_line);
        
        if(scanned_line[0] != '{' || scanned_line[scanned_line.size()-1] != '}')
            throw std::runtime_error("DPA:scan_vector: Input is not a vector. Found: (" + scanned_line + ")");
        
        std::stringstream inner(scanned_line.substr(1,scanned_line.size()-2));
        std::string segment;
        while(std::getline(inner, segment, ',')){
            T val;
            std::stringstream tmp_ss(segment);
            tmp_ss >> val;
            vec.push_back(val);
        }
    }    
    template void OmegaUtils::scan_vector(std::vector<std::string>& vec, std::istream& ist);
    template void OmegaUtils::scan_vector(std::vector<bool>& vec, std::istream& ist);
    template void OmegaUtils::scan_vector(std::vector<uint32_t>& vec, std::istream& ist);
    template void OmegaUtils::scan_vector(std::vector<int32_t>& vec, std::istream& ist);
    template void OmegaUtils::scan_vector(std::vector<uint64_t>& vec, std::istream& ist);
    template void OmegaUtils::scan_vector(std::vector<int64_t>& vec, std::istream& ist);
    template void OmegaUtils::scan_vector(std::vector<atomic_proposition_status_t>& vec, std::istream& ist);
}