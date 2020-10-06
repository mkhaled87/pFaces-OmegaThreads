#pragma once

/*
* omegaControlProblem.h
*
*  date    : 10.09.2020
*  author  : M. Khaled
*  details : a class for symbolic control problems.
* 
*/

#include <iostream>
#include <string>
#include <vector>
#include <map>

#include <pfaces-sdk.h>
#include <Ltl2Dpa.h>
#include "omegaDpa.h"

namespace pFacesOmegaKernels{

// function types to by used for tempated classes
typedef std::vector<symbolic_t> (post_func_t)(symbolic_t, symbolic_t);
typedef symbolic_t (L_x_func_t)(symbolic_t);
typedef symbolic_t (L_u_func_t)(symbolic_t);

// a class to represent the state of the symbolic model
class SymState{
public:

    // enum to represent the type of the state
    enum SYM_STATE_TYPE{
        DUMMY_STATE,
        NORMAL_STATE,
        OVERFLOW_STATE
    };

    SYM_STATE_TYPE type;
    symbolic_t value;
    
    // constructors
    SymState();
    SymState(SYM_STATE_TYPE _type);
    SymState(SYM_STATE_TYPE _type, symbolic_t _value);

    // a comparator
    bool operator==(const SymState& anotherSymState) const;
};


// a class to represent the symbolic model
// F: the type of the function used for posts on the template:
//
//      std::vector<symbolic_t> posts(symbolic_t x, symbolic_t u);
//
// you may change the template in top of this header.
//
template<class F>
class SymModel {

    // sizes of X and U spaces are enough to identify the states
    size_t n_sym_states;
    size_t n_sym_controls;

    // an overflow state
    SymState overflow_state;

    // a dummy state
    SymState dummy_state;

    // a function to represent the transition map
    F& get_sym_posts;

    // the set of initial states
    std::vector<SymState> initial_states;    
 
public:

    // a constructor
    SymModel(const size_t _n_sym_states, const size_t _n_sym_controls, const std::vector<symbolic_t>& sym_inital_states, F& post_func);

    // checks for states and controls
    bool is_valid_sym_state(symbolic_t state) const;
    bool is_valid_sym_control(symbolic_t control) const;    
    bool is_overflow_state(const SymState& state) const;
    bool is_dummy_state(const SymState& state) const;     

    // getters
    std::vector<SymState> get_initial_states() const;
    SymState get_dummy_state() const;
    SymState get_overflow_state() const;
    size_t get_n_states();
    size_t get_n_controls();
    std::vector<SymState> get_posts(const SymState& state, const symbolic_t control) const;

    // constructs a state in a smart way using only its symbolic value
    // if value < num_state => normal
    // if value = num_state => dummy
    // if value = num_state+1 => overflow
    SymState construct_state(const symbolic_t val);

    // print info
    void print_info();
};


// a class to represent the symbolic specifications
// L1: the type of the function used to map X -> X_APs (i.e., L_x)
// L2: the type of the function used to map U -> U_APs (i.e., L_u)
//
// L1 or L2 should take the state/control as symbolic_t and return
// a vector of bits representing indicies to APs the state/control 
// belong to:
//
//      symbolic_t L(symbolic_t in);
//
// you may change the template in top of this header.
//
template<class L1, class L2>
class SymSpec {
public: 
    // enum to represent the type of AP
    enum AP_TYPE{
        STATE_AP,
        CONTROL_AP
    };

private:
    const size_t max_possible_APs = std::numeric_limits<strix_aut::letter_t>::digits;
    const size_t max_possible_state_APs = max_possible_APs/2;
    const size_t max_possible_control_APs = max_possible_APs/2;    
    const std::string valid_AP_descr = std::string("one lower alpha letters");

    // the map functions
    L1& L_x;
    L2& L_u;

    // storage for APs and formula
    std::vector<std::string> state_APs;
    std::vector<std::string> control_APs;
    std::string ltl_spec;

    // add an LTL spec
    void add_ltl_formula(std::string _ltl_spec);
    
    // add the state atomic propositions
    void add_state_APs(const std::vector<std::string>& _state_APs);

    // add the control atomic propositions
    void add_control_APs(const std::vector<std::string>& _control_APs);    
    
public:
    // a total DPA to represent the low-level spec
    TotalDPA dpa;

    // constructors
    SymSpec(
        const std::vector<std::string>& _state_APs, 
        const std::vector<std::string>& _control_APs, 
        const std::string& _ltl_formula,
        L1& _L_x,
        L2& _L_u);
    SymSpec(
        const std::string& _dpa_file,
        L1& _L_x,
        L2& _L_u);

    // is a specific AP valid 
    bool is_valid_AP(std::string AP);

    // counters
    size_t count_all_APs() const;
    size_t count_state_APs() const;
    size_t count_control_APs() const;    
    size_t count_DPA_states() const;

    // getters
    std::vector<std::string> get_state_APs();
    std::vector<std::string> get_control_APs();
    std::string get_ltl_formula();

    // from a state and control, get a complete clause in AP space
    strix_aut::letter_t get_complete_clause(const SymState& symState, const symbolic_t control) const;

    // print info
    void print_info();
};

}