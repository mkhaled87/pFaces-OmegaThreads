#pragma once

/*
* omegaDpa.h
*
*  date    : 10.09.2020
*  author  : M. Khaled
*  details : a class for DPAs.
* 
*/

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <cmath>

#include <Ltl2Dpa.h>

namespace pFacesOmegaKernels{

// Edge of a total DPA
class TotalDpaEdge {
public:
    strix_aut::letter_t letter;
    strix_aut::product_state_t successor;
    strix_aut::ColorScore cs;
    size_t successor_idx;

    // constructor
    TotalDpaEdge(const strix_aut::letter_t& letter, const strix_aut::product_state_t successor, const strix_aut::ColorScore& cs, const size_t succ_idx);

    // parse edge from string to be used for loading the edge from files
    static TotalDpaEdge parse(const std::string& str, const std::vector<strix_aut::product_state_t>& states);
};


// the TotalDPA (transitions defined for all input-letters and all states)
// this is developed to ease the later parallel implementation
class TotalDPA{

    // the specs initials
    std::vector<std::string> inVars;
    std::vector<std::string> outVars;
    std::string ltl_formula;
    size_t n_io_vars;
    size_t product_state_size;

    // DPA metadata
    bool simplified_ltl;
    strix_aut::Parity parity;
    strix_aut::color_t max_color;
    std::vector<atomic_proposition_status_t> statuses;

    // the states + flags for top/button
    std::vector<strix_aut::product_state_t> states;
    std::vector<bool> states_is_top;
    std::vector<bool> states_is_bottom;
    
    // per state: a list of edges
    std::vector<std::vector<TotalDpaEdge>> state_edges;

    // a private function to check
    inline std::pair<bool, size_t> is_state_in_states(const strix_aut::product_state_t& state);

    // a map to cache the indeicies of DPA node for faster access
    std::map<strix_aut::product_state_t, size_t> dpa_state_idx_map;
    bool dpa_state_idx_map_ready = false;

    // constants
    const std::string err_invalid_input = "Invalid input file: ";

    // caching states/indicies
    void cache_states();

public:

    // constructors
    TotalDPA(const std::string& filename);
    TotalDPA(const std::vector<std::string>& _inVars, const std::vector<std::string>& _outVars, const std::string& _ltl_formula, bool simplify_spec = false);

    // getters
    strix_aut::product_state_t getInitialState();
    strix_aut::Parity getParity();
    strix_aut::color_t getMaxColor();
    atomic_proposition_status_t getVariableStatus(int variable);
    size_t getStatesCount() const;
    std::vector<std::string> getInVars();
    std::vector<std::string> getOutVars();
    std::string getLtlFormula();

    // get the successor DPA state (yes it is only one sucessor as it is a (D)PA)
    strix_aut::ColorScore getSuccessor(const strix_aut::product_state_t& state, strix_aut::product_state_t& successor, const strix_aut::letter_t& io_letter);

    // is a state TOP/BUTTOM ?
    bool isTopState(const strix_aut::product_state_t& state);
    bool isBottomState(const strix_aut::product_state_t& state);

    // print the DPA info
    void printInfo();

    // write/load the DPA to/from a file
    void loadFromFile(const std::string& filename);
    void writeToFile(const std::string& filename);
};

}