/*
* omegaDpa.cpp
*
*  date    : 10.09.2020
*  author  : M. Khaled
*  details : a class for DPAs.
* 
*/

#include "omegaDpa.h"
#include "omegaUtils.h"

namespace pFacesOmegaKernels{


// ------------------------------------------
// class TotalDpaEdge
// ------------------------------------------
TotalDpaEdge::TotalDpaEdge(const strix_aut::letter_t& letter, const strix_aut::product_state_t successor, const strix_aut::ColorScore& cs, const size_t succ_idx) 
    : letter(letter), successor(successor), cs(cs), successor_idx(succ_idx) {

}

TotalDpaEdge TotalDpaEdge::parse(const std::string& str, const std::vector<strix_aut::product_state_t>& states){

    if(str[0] != '{' || str[str.size()-1] != '}')
        throw std::runtime_error("TotalDpaEdge:parse: Input is not a valid TotalDpaEdge. passed: (" + str + ")");
    
    std::stringstream inner(str.substr(1,str.size()-2));
    std::string segment;
    size_t idx=0;
    strix_aut::letter_t letter;
    size_t successor_idx;
    strix_aut::ColorScore cs;
    while(std::getline(inner, segment, ',')){
        std::stringstream tmp_ss(segment);
        if(idx == 0)
            tmp_ss >> letter;
        else if (idx == 1)
            tmp_ss >> successor_idx;
        else if (idx == 2)
            tmp_ss >> cs.color;
        else if (idx == 3)
            tmp_ss >> cs.score;
        else
            throw std::runtime_error("TotalDpaEdge:parse: parse failed: found additional element.");

        idx++;
    }

    if(successor_idx >= states.size())
        throw std::runtime_error("TotalDpaEdge:parse: parse failed: index out of bound:" + std::to_string(successor_idx) + " inside: " + str);

    return TotalDpaEdge(letter, states[successor_idx], cs, successor_idx);
}



// ------------------------------------------
// class TotalDPA
// ------------------------------------------
inline std::pair<bool, size_t> TotalDPA::is_state_in_states(const strix_aut::product_state_t& state){

    size_t size = state.size();
    size_t found_idx = 0;
    for(auto to_compare : states){
        
        bool different = false;
        for(size_t i=0; i<size; i++){
            if(to_compare[i] != state[i]){
                different = true;
                break;
            }
        }
        
        if(!different)
            return std::make_pair(true, found_idx);
        
        found_idx++;
    }
    return std::make_pair(false, 0);
}

TotalDPA::TotalDPA(const std::string& filename){
    loadFromFile(filename);
}

TotalDPA::TotalDPA(const std::vector<std::string>& _inVars, const std::vector<std::string>& _outVars, const std::string& _ltl_formula, bool simplify_spec){

    // store locally
    inVars = _inVars;
    outVars = _outVars;
    ltl_formula = _ltl_formula;
    n_io_vars = inVars.size() + outVars.size();
    simplified_ltl = simplify_spec;
    
    // use ltl2dpa tp cpmstruct the tree automaton
    strix_ltl2dpa::Ltl2DpaConverter converter;
    strix_aut::AutomatonTreeStructure autoStruct = converter.toAutTreeStruct(ltl_formula, inVars, outVars, simplify_spec);    

    // collect meta-data
    parity =  autoStruct.getParity();
    max_color = autoStruct.getMaxColor();
    statuses.resize(n_io_vars);
    for(size_t i=0; i<n_io_vars; i++)
        statuses[i] = autoStruct.getVariableStatus(i);

    // a queue for indexes of newly discovered states to be explored
    std::queue<size_t> to_explore;
    
    // get initial state : put always at index 0
    strix_aut::product_state_t initial_state = autoStruct.getInitialState();
    product_state_size = initial_state.size();
    states.push_back(initial_state);

    // add the initial state to the next to explore + create an edge list for it
    to_explore.push(0);
    state_edges.push_back(std::vector<TotalDpaEdge>());
    states_is_top.push_back(autoStruct.isTopState(initial_state));
    states_is_bottom.push_back(autoStruct.isBottomState(initial_state));       

    // keep exploring until no more states in the queue
    while(!to_explore.empty()){

        // get the state idx in the front
        size_t state_idx = to_explore.front();
        to_explore.pop();

        // get the state itself
        strix_aut::product_state_t state = states[state_idx];

        // for all io possibilities
        // TODO: improve this as done in strix using the Seq iterators
        // to avoid irrelevant IOs
        size_t n_possibilities = std::pow(2,n_io_vars);
        for(strix_aut::letter_t io_letter=0; io_letter < n_possibilities; io_letter++){

            // get the successor
            strix_aut::product_state_t successor(product_state_size);
            strix_aut::ColorScore cs = autoStruct.getSuccessor(state, successor, io_letter);

            // is the successor a new state ? add it to explore list
            auto successor_info = is_state_in_states(successor);
            size_t successor_idx = successor_info.second;
            if(!successor_info.first){
                successor_idx = states.size();
                to_explore.push(states.size());
                states.push_back(successor);
                state_edges.push_back(std::vector<TotalDpaEdge>());
                states_is_top.push_back(autoStruct.isTopState(successor));
                states_is_bottom.push_back(autoStruct.isBottomState(successor));
            }

            // create the edge
            state_edges[state_idx].push_back(TotalDpaEdge(io_letter, successor, cs, successor_idx));                
        }
    }
}

strix_aut::product_state_t TotalDPA::getInitialState(){
    return states[0];
}

strix_aut::Parity TotalDPA::getParity(){
    return parity;
}
strix_aut::color_t TotalDPA::getMaxColor(){
    return max_color;
}
atomic_proposition_status_t TotalDPA::getVariableStatus(int variable){
    return statuses[variable];
}
size_t TotalDPA::getStatesCount() const {
    return states.size();
}

strix_aut::ColorScore TotalDPA::getSuccessor(const strix_aut::product_state_t& state, strix_aut::product_state_t& successor, const strix_aut::letter_t& io_letter){

    std::pair<bool, size_t> found_state = is_state_in_states(state);
    if(!found_state.first)
        throw std::runtime_error("getSuccessor: The state is not found in the list of states.");

    if(io_letter >= std::pow(2,n_io_vars))
        throw std::runtime_error("getSuccessor: Invalid IO letter supplied.");

    TotalDpaEdge edge = state_edges[found_state.second][io_letter];

    if(edge.letter != io_letter)
        throw std::runtime_error("getSuccessor: Ooops something went wrong here.");

    successor = edge.successor;
    return edge.cs;
}

bool TotalDPA::isTopState(const strix_aut::product_state_t& state){
    std::pair<bool, size_t> found_state = is_state_in_states(state);
    if(!found_state.first)
        throw std::runtime_error("isTopState: The state is not found in the list of states.");

    return states_is_top[found_state.second];
}

bool TotalDPA::isBottomState(const strix_aut::product_state_t& state){
    std::pair<bool, size_t> found_state = is_state_in_states(state);
    if(!found_state.first)
        throw std::runtime_error("isBottomState: The state is not found in the list of states.");

    return states_is_bottom[found_state.second];
}    

void TotalDPA::printInfo(){
    const size_t n_states = states.size();

    std::cout << "The DPA has " << n_states << " states." << std::endl;
    std::cout << "Parity: " << parity << std::endl;
    std::cout << "Max Color: " << max_color << std::endl;
    std::cout << "Statuses: "; OmegaUtils::print_vector(statuses); std::cout << std::endl;
    std::cout << "Transitions: " << std::endl;
    for (size_t i=0; i<n_states; i++){
        auto state = states[i];
        std::cout << "q_" << i << " ";
        OmegaUtils::print_vector(state);
        if(states_is_top[i]) std::cout << "[TOP]";
        if(states_is_bottom[i]) std::cout << "[BOTTOM]";
        std::cout << ":" << std::endl;
        for (auto edge : state_edges[i]){
            std::cout << "\t --(b_IO: ";
            std::bitset<8> io(edge.letter);
            std::cout << io << ", Clr:" << edge.cs.color << ", Scr: " << edge.cs.score;
            std::cout << ")--> q_" << edge.successor_idx;
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }
}

void TotalDPA::loadFromFile(const std::string& filename){
    std::ifstream ifs(filename);
    std::string scanned;

    ifs >> scanned;
    if(scanned != std::string("in_vars:"))
        throw std::runtime_error(err_invalid_input + "cant find in_vars.");
    else
        OmegaUtils::scan_vector(inVars, ifs);

    scanned = "";
    ifs >> scanned;
    if(scanned != std::string("out_vars:"))
        throw std::runtime_error(err_invalid_input + "cant find out_vars.");
    else
        OmegaUtils::scan_vector(outVars, ifs);

    scanned = "";
    ifs >> scanned;
    if(scanned != std::string("ltl_formula:"))
        throw std::runtime_error(err_invalid_input + "cant find ltl_formula.");
    else{
        std::getline(ifs, ltl_formula);
        ltl_formula = pfacesUtils::strTrim(ltl_formula);
    }

    scanned = "";
    ifs >> scanned;
    if(scanned != std::string("n_io_vars:"))
        throw std::runtime_error(err_invalid_input + "cant find n_io_vars.");
    else{
        ifs >> n_io_vars;
    }

    if(n_io_vars != (inVars.size() + outVars.size()))
        throw std::runtime_error(err_invalid_input + "invalid number of IO vars.");

    scanned = "";
    ifs >> scanned;
    if(scanned != std::string("product_state_size:"))
        throw std::runtime_error(err_invalid_input + "cant find product_state_size.");
    else{
        ifs >> product_state_size;
    }

    scanned = "";
    ifs >> scanned;
    if(scanned != std::string("simplified_ltl:"))
        throw std::runtime_error(err_invalid_input + "cant find simplified_ltl.");
    else{
        ifs >> simplified_ltl;
    }

    scanned = "";
    ifs >> scanned;
    if(scanned != std::string("parity:"))
        throw std::runtime_error(err_invalid_input + "cant find parity.");
    else{
        size_t p;
        ifs >> p;

        if(p == 0)
            parity = strix_aut::Parity::EVEN;
        else
            parity = strix_aut::Parity::ODD;
    }
    
    scanned = "";
    ifs >> scanned;
    if(scanned != std::string("max_color:"))
        throw std::runtime_error(err_invalid_input + "cant find max_color.");
    else{
        ifs >> max_color;
    }

    scanned = "";
    ifs >> scanned;
    if(scanned != std::string("statuses:"))
        throw std::runtime_error(err_invalid_input + "cant find statuses.");
    else
        OmegaUtils::scan_vector(statuses, ifs);
 
    size_t n_states;
    scanned = "";
    ifs >> scanned;
    if(scanned != std::string("n_states:"))
        throw std::runtime_error(err_invalid_input + "cant find n_states.");
    else{
        ifs >> n_states;
    }

    scanned = "";
    ifs >> scanned;
    if(scanned != std::string("states:"))
        throw std::runtime_error(err_invalid_input + "cant find states.");
    else{

        std::string states_line;
        std::getline(ifs, states_line);
        states_line = pfacesUtils::strTrim(states_line);

        std::stringstream ss_states(states_line);
        std::string str_state;
        while(std::getline(ss_states, str_state, ';')){
            std::stringstream ss_state(str_state);
            strix_aut::product_state_t state;
            OmegaUtils::scan_vector(state, ss_state);
            states.push_back(state);
        }
    }

    if(states.size() != n_states)
        throw std::runtime_error(err_invalid_input + "invalid number of states elements.");

    scanned = "";
    ifs >> scanned;
    if(scanned != std::string("states_is_top:"))
        throw std::runtime_error(err_invalid_input + "cant find states_is_top.");
    else
        OmegaUtils::scan_vector(states_is_top, ifs);       

    if(states_is_top.size() != n_states)
        throw std::runtime_error(err_invalid_input + "invalid number of states_is_top elements.");

    scanned = "";
    ifs >> scanned;
    if(scanned != std::string("states_is_bottom:"))
        throw std::runtime_error(err_invalid_input + "cant find states_is_bottom.");
    else
        OmegaUtils::scan_vector(states_is_bottom, ifs);

    if(states_is_bottom.size() != n_states)
        throw std::runtime_error(err_invalid_input + "invalid number of states_is_bottom elements.");

    for(size_t i=0; i<n_states; i++){
        std::string  state_label = std::string("state_") + std::to_string(i) + std::string("_edges:");

        scanned = "";
        ifs >> scanned;
        if(scanned != std::string(state_label))
            throw std::runtime_error(err_invalid_input + "cant find " + state_label);
        else{
            state_edges.push_back(std::vector<TotalDpaEdge>());

            std::string edges_line;
            std::getline(ifs, edges_line);
            edges_line = pfacesUtils::strTrim(edges_line);

            std::stringstream ss_edges(edges_line);
            std::string str_edge;
            while(std::getline(ss_edges, str_edge, ';')){
                state_edges[i].push_back(TotalDpaEdge::parse(str_edge, states));
            }
        }
    }
}

void TotalDPA::writeToFile(const std::string& filename){

    std::ofstream ofs(filename);

    ofs << "in_vars: ";
    OmegaUtils::print_vector(inVars, ofs);
    ofs << std::endl;

    ofs << "out_vars: ";
    OmegaUtils::print_vector(outVars, ofs);
    ofs << std::endl;

    ofs << "ltl_formula: " << ltl_formula << std::endl;
    ofs << "n_io_vars: " << n_io_vars << std::endl;
    ofs << "product_state_size: " << product_state_size << std::endl;
    ofs << "simplified_ltl: " << simplified_ltl << std::endl;
    ofs << "parity: " << parity << std::endl;
    ofs << "max_color: " << max_color << std::endl;

    ofs << "statuses: ";
    OmegaUtils::print_vector(statuses, ofs);
    ofs << std::endl;
    
    ofs << "n_states: " << states.size() << std::endl;
    ofs << "states: ";
    for (auto state : states){
        OmegaUtils::print_vector(state, ofs);
        ofs << ";";
    }
    ofs << std::endl;

    ofs << "states_is_top: ";
    OmegaUtils::print_vector(states_is_top, ofs);
    ofs << std::endl;

    ofs << "states_is_bottom: ";
    OmegaUtils::print_vector(states_is_bottom, ofs);
    ofs << std::endl;

    size_t idx = 0;
    for (auto edges : state_edges){
        ofs << "state_" << idx << "_edges: ";
        size_t edge_idx = 0;
        for(auto edge : edges){
            ofs << "{"
                << edge.letter << ","
                << edge.successor_idx << ","
                << edge.cs.color << ","
                << edge.cs.score
                << "}";
            
            edge_idx++;
            if(edge_idx != edges.size())
                ofs << ";";
        }
        ofs << std::endl;
        idx++;
    }        
    ofs << std::endl;

}

}