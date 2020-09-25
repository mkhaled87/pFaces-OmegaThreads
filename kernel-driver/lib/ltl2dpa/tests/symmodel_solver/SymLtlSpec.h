#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <map>

#include <pfaces-sdk.h>
#include <Ltl2Dpa.h>

inline bool is_valid_sym_state(size_t n_sym_states, symbolic_t state){
    return (state < n_sym_states);
}
inline bool is_valid_sym_control(size_t n_sym_controls, symbolic_t control){
    return (control < n_sym_controls);
}

enum AP_TYPE{
    STATE_AP,
    CONTROL_AP
};

enum SYM_STATE_TYPE{
    DUMMY,
    NORMAL
};

class SymState{
public:
    SYM_STATE_TYPE type;
    symbolic_t value;
    SymState():type(SYM_STATE_TYPE::DUMMY),value(0){}
    SymState(SYM_STATE_TYPE _type):type(_type),value(0){}
    SymState(SYM_STATE_TYPE _type, symbolic_t _value):type(_type),value(_value){}

    bool operator==(const SymState& anotherSymState) const {
        return (type == anotherSymState.type)&&(value == anotherSymState.value);
    }
};

template<class F>
class SymModel {
    size_t n_sym_states;
    size_t n_sym_controls;

    SymState initial_state;

    F& get_sym_posts;

public:
    SymModel(const size_t _n_sym_states, const size_t _n_sym_controls, const symbolic_t sym_inital_state, F& post_func)
    :n_sym_states(_n_sym_states), n_sym_controls(_n_sym_controls), 
    initial_state(SymState(SYM_STATE_TYPE::NORMAL, sym_inital_state)),
    get_sym_posts(post_func){}

    SymState get_initial_state() const {
        return initial_state;
    }

    std::vector<SymState> get_posts(const SymState& state, const symbolic_t control) const {

        // is dymmy ? invalid ! maybe later we make it a sink state !
        if(state.type == SYM_STATE_TYPE::DUMMY)
            throw std::runtime_error("SymModel::get_posts: no post for DUMMY states.");

        if(is_valid_sym_state(n_sym_states, state.value) && is_valid_sym_control(n_sym_controls, control)){
            std::vector<symbolic_t> sym_post_states = get_sym_posts(state.value, control);
            std::vector<SymState> ret;
            
            for (symbolic_t sym_state : sym_post_states)
                ret.push_back(SymState(SYM_STATE_TYPE::NORMAL, sym_state));
            
            return ret;
        }
        else
            throw std::runtime_error("SymModel::get_posts: Invalid state.");
    }
        
};

class SymLtlSpec {

    const std::string AP_ALL_STATES = std::string("s");
    const std::string AP_ALL_CONTROLS = std::string("c");  
    const size_t max_possible_APs = std::numeric_limits<strix_aut::letter_t>::digits;
    const size_t max_possible_state_APs = max_possible_APs/2;
    const size_t max_possible_control_APs = max_possible_APs/2;

    size_t n_sym_states;
    size_t n_sym_controls;

    std::map<std::string, std::vector<symbolic_t>> state_APs_to_states_map;
    std::map<std::string, std::vector<symbolic_t>> control_APs_to_controls_map;

    std::vector<std::string> state_APs;
    std::vector<std::string> control_APs;
    std::string ltl_spec;

    const std::string valid_AP_descr = std::string("one lower alpha letter excluding: ") + AP_ALL_STATES + std::string(", ") + AP_ALL_CONTROLS + std::string(".");
    inline bool is_valid_AP(std::string AP){

        std::vector<std::string> banned_alpha = {AP_ALL_STATES, AP_ALL_CONTROLS};

        if(AP.size() > 1)
            return false;

        if(!std::isalpha(AP[0]))
            return false;

        if(!std::islower(AP[0]))
            return false;

        if(pfacesUtils::isVectorElement(banned_alpha,std::to_string(AP[0])))
            return false;

        return true;
    }

    inline size_t count_all_APs() const {
        // include additional 2 for AP_ALL_STATES and AP_ALL_CONTROLS
        return state_APs.size() + control_APs.size() + 2;
    }
    inline size_t count_state_APs() const {
        // include additional 1 for AP_ALL_STATES
        return state_APs.size() + 1;
    }
    inline size_t count_control_APs() const {
        // include additional 1 for AP_ALL_CONTROLS
        return control_APs.size() + 1;
    }    
public:
    SymLtlSpec(const size_t _n_sym_states, const size_t _n_sym_controls)
    :n_sym_states(_n_sym_states), n_sym_controls(_n_sym_controls){}

    // add an LTL spec
    void add_ltl_spec(std::string _ltl_spec){
        ltl_spec = _ltl_spec;
    }
    
    // add a new state atomic proposition
    void add_state_AP(const std::string state_AP, const std::vector<symbolic_t>& sym_states){

        if(count_all_APs() == max_possible_APs)
            throw std::runtime_error("SymLtlSpec::add_state_AP: Maximum number of possible APs (states + controls) reached !");

        if(count_state_APs() == max_possible_state_APs)
            throw std::runtime_error("SymLtlSpec::add_state_AP: Maximum number of possible state APs reached !");

        if(pfacesUtils::isVectorElement(state_APs, state_AP))
            throw std::runtime_error("SymLtlSpec::add_state_AP: The provided state AP is already added.");

        if(pfacesUtils::isVectorElement(control_APs, state_AP))
            throw std::runtime_error("SymLtlSpec::add_state_AP: The provided state AP is already used as a control AP.");            

        if(!is_valid_AP(state_AP))
            throw std::runtime_error(
                std::string("SymLtlSpec::add_state_AP: Invalid state AP: an AP must be ") + valid_AP_descr
                );

        state_APs.push_back(state_AP);
        state_APs_to_states_map[state_AP] = sym_states;
    }

    // add a new control atomic proposition
    void add_control_AP(const std::string control_AP, const std::vector<symbolic_t>& sym_controls){

        if(count_all_APs() == max_possible_APs)
            throw std::runtime_error("SymLtlSpec::add_control_AP: Maximum number of possible APs (states + controls) reached !");

        if(count_control_APs() == max_possible_control_APs)
            throw std::runtime_error("SymLtlSpec::add_control_AP: Maximum number of possible contorl APs reached !");

        if(pfacesUtils::isVectorElement(control_APs, control_AP))
            throw std::runtime_error("SymLtlSpec::add_control_AP: The provided control AP is already added.");

        if(pfacesUtils::isVectorElement(state_APs, control_AP))
            throw std::runtime_error("SymLtlSpec::add_control_AP: The provided control AP is already used as a state AP.");

        if(!is_valid_AP(control_AP))
            throw std::runtime_error(
                std::string("SymLtlSpec::add_control_AP: Invalid control AP: an AP must be ") + valid_AP_descr
                );

        control_APs.push_back(control_AP);
        control_APs_to_controls_map[control_AP] = sym_controls;        
    }    

    // get the set of input vars
    std::vector<std::string> get_ltl_spec_in_vars(){
        // get those added by the user + S representing those un-mapped states
        std::vector<std::string> ret = state_APs;
        ret.insert(ret.begin(), AP_ALL_STATES);
        return ret;
    }

    // construct the set of mapped input vars
    std::vector<std::string> get_mapped_ltl_spec_in_vars(bool print=false){
        
        size_t req_bits = (size_t)std::ceil(std::log2(n_sym_states));
        std::vector<std::string> ret;
        if(print)
            std::cout << "in_vars: ";
        for(size_t i=0; i<req_bits; i++){
            std::string to_add = std::string("x") + std::to_string(i);
            if(print)
                std::cout << to_add << " ";
            ret.push_back(to_add);
        }
        if(print)
            std::cout << std::endl;        
        return ret;
    }

    // get the set of input vars
    std::vector<std::string> get_ltl_spec_out_vars(){
        // get those added by the user + C representing those un-mapped controls
        std::vector<std::string> ret = control_APs;
        ret.insert(ret.begin(), AP_ALL_CONTROLS);
        return ret;
    }

    // construct the set of mapped output vars
    std::vector<std::string> get_mapped_ltl_spec_out_vars(bool print=false){

        size_t req_bits = (size_t)std::ceil(std::log2(n_sym_controls));
        std::vector<std::string> ret;
        if(print)
            std::cout << "out_vars: ";        
        for(size_t i=0; i<req_bits; i++){
            std::string to_add = std::string("u") + std::to_string(i);
            if(print)
                std::cout << to_add << " ";            
            ret.push_back(to_add);
        }
        if(print)
            std::cout << std::endl;            
        return ret;        
    }

    // get all input vars that cover this state 
    std::vector<std::string> sym_state_to_in_vars(symbolic_t state){
        std::vector<std::string> ret;
        ret.push_back(AP_ALL_STATES);
        for(auto in_ap : state_APs){
            if(pfacesUtils::isVectorElement(state_APs_to_states_map[in_ap],state))
                ret.push_back(in_ap);
        }
        return ret;
    }

    // get out input vars that cover this control
    std::vector<std::string> sym_control_to_out_vars(symbolic_t control){
        std::vector<std::string> ret;
        ret.push_back(AP_ALL_CONTROLS);
        for(auto out_ap : control_APs){
            if(pfacesUtils::isVectorElement(control_APs_to_controls_map[out_ap],control))
                ret.push_back(out_ap);
        }
        return ret;
    }    

    // given a symbolic state (idx in the sym state space), construct the
    // corresponding a binary clause of the corresponding input variables (AP_in).
    std::string sym_state_to_mapped_clause(symbolic_t state){

        if(!is_valid_sym_state(n_sym_states, state))
            throw std::runtime_error("SymLtlSpec::sym_state_to_mapped_clause: Invalid state.");

        std::stringstream ret_ss;
        ret_ss << "(";
        size_t n_bits = (size_t)std::ceil(std::log2(n_sym_states));
        for(size_t i=0; i<n_bits; i++){
            symbolic_t mask = ((symbolic_t)1) << i;
            if(mask & state)
                ret_ss << "x" << i;
            else
                ret_ss << "!x" << i;

            if(i < (n_bits-1))
                ret_ss << "&";
        }
        ret_ss << ")";
        std::string ret = ret_ss.str();
        return ret;
    }

    bool is_sym_state_in_AP(const symbolic_t state, const std::string& ap){
        bool found = false;

        for(auto s : state_APs_to_states_map[ap])
            if(s == state)
                return true;
                
        return false;
    }

    // given a symbolic control (idx in the sym control space), construct the
    // corresponding a binary clause of the corresponding output variables (AP_out).
    std::string sym_control_to_mapped_clause(symbolic_t control){
        if(!is_valid_sym_control(n_sym_controls, control))
            throw std::runtime_error("SymLtlSpec::sym_control_to_mapped_clause: Invalid control.");

        std::stringstream ret_ss;
        ret_ss << "(";
        size_t n_bits = (size_t)std::ceil(std::log2(n_sym_controls));
        for(size_t i=0; i<n_bits; i++){
            symbolic_t mask = ((symbolic_t)1) << i;
            if(mask & control)
                ret_ss << "u" << i;
            else
                ret_ss << "!u" << i;

            if(i < (n_bits-1))
                ret_ss << "&";
        }
        ret_ss << ")";
        std::string ret = ret_ss.str();
        return ret;        
    }   

    std::string get_ltl_spec(){
        return ltl_spec;
    }

    std::string get_mapped_ltl_spec(){

        std::string ret = ltl_spec;

        // replace any state AP
        for(auto state_AP : state_APs){
            
            // is it in the ltl_spec ?
            std::size_t found = ret.find(state_AP);
            if (found != std::string::npos){
                
                // construct its ORed-clauses-expr and replace it
                std::stringstream to_replace_ss;
                to_replace_ss << "(";

                auto states_vector = state_APs_to_states_map[state_AP];
                for(size_t i=0; i<states_vector.size(); i++){
                    symbolic_t state = states_vector[i];
                    to_replace_ss << sym_state_to_mapped_clause(state);
                    if(i < states_vector.size()-1)
                        to_replace_ss << "|";
                }

                to_replace_ss << ")";
                std::string to_replace = to_replace_ss.str();
                ret = pfacesUtils::strReplaceAll(ret, state_AP, to_replace);
            }
        }

        // replace any control AP
        for(auto control_AP : control_APs){

            // is it in the ltl_spec ?
            std::size_t found = ret.find(control_AP);
            if (found != std::string::npos){

                // construct its ORed-clauses-expr and replace it
                std::stringstream to_replace_ss;
                to_replace_ss << "(";

                auto controls_vector = control_APs_to_controls_map[control_AP];
                for(size_t i=0; i<controls_vector.size(); i++){
                    symbolic_t control = controls_vector[i];
                    to_replace_ss << sym_control_to_mapped_clause(control);
                    if(i < controls_vector.size()-1)
                        to_replace_ss << "|";
                }
                
                to_replace_ss << ")";
                std::string to_replace = to_replace_ss.str();
                ret = pfacesUtils::strReplaceAll(ret, control_AP, to_replace);
            }
        }

        return ret;
    }

    #define PRINT_CLAUSE_DETAILS
    strix_aut::letter_t get_complete_clause(const SymState& symState, const symbolic_t control) const {

        // bits  :                                             ... 1 0           
        // vars  :    other_control_APs    c     other_state_APs     s

        const strix_aut::letter_t mask_all_states = 1;
        const strix_aut::letter_t mask_all_controls = ((strix_aut::letter_t)1 << count_state_APs());

        // start with all-false clause
        strix_aut::letter_t clause = 0;

        // only a normal state belongs to all states
        if(symState.type == SYM_STATE_TYPE::NORMAL)
            clause |= mask_all_states;
        
        // control always belongs to all controls
        clause |= mask_all_controls;

        // check state APs
        if(symState.type == SYM_STATE_TYPE::NORMAL){
            strix_aut::letter_t idx = 1;
            symbolic_t state = symState.value;
            for(std::string state_AP : state_APs){
                std::vector<symbolic_t> vec_states = state_APs_to_states_map.at(state_AP);
                auto vec_it = std::find(vec_states.begin(), vec_states.end(), state);
                if (vec_it != vec_states.end()){
                    clause |= ((strix_aut::letter_t)1 << idx);
                }
                idx++;
            }
        }

        // check control APs
        strix_aut::letter_t idx = count_state_APs() + 1;
        for(std::string control_AP : control_APs){
            std::vector<symbolic_t> vec_controls = control_APs_to_controls_map.at(control_AP);
            auto vec_it = std::find(vec_controls.begin(), vec_controls.end(), control);
            if (vec_it != vec_controls.end()){            
                clause |= ((strix_aut::letter_t)1 << idx);                                 
            }
            idx++;
        }

        return clause;
    }
};

template<class F>
class SymControlProblem {
    size_t n_sym_states;
    size_t n_sym_controls;    
    symbolic_t x_initial;
    
    SymModel<F> model;
    SymLtlSpec ltlSpec;
public:
    SymControlProblem(const size_t _n_sym_states, const size_t _n_sym_controls, const symbolic_t _x_initial, F& post_func, std::string _ltl_formula)
    :n_sym_states(_n_sym_states), 
    n_sym_controls(_n_sym_controls), 
    x_initial(_x_initial), 
    model(SymModel(_n_sym_states, _n_sym_controls, _x_initial, post_func)),
    ltlSpec(SymLtlSpec(_n_sym_states, _n_sym_controls)) {
        ltlSpec.add_ltl_spec(_ltl_formula);
    }

    size_t get_n_states() const {
        return n_sym_states;
    }

    size_t get_n_controls() const {
        return n_sym_controls;
    }

    void add_AP(const AP_TYPE type, const std::string& AP, const std::vector<symbolic_t>& elements){
        switch(type){
            case AP_TYPE::STATE_AP:
                ltlSpec.add_state_AP(AP, elements);
            break;

            case AP_TYPE::CONTROL_AP:
                ltlSpec.add_control_AP(AP, elements);
            break;

            default:
                throw std::runtime_error("SymControlProblem::addAP: Invalid AP type.");
            break;
        }
    }

    std::string get_ltl_formula(){
        return ltlSpec.get_ltl_spec();
    }
    std::string get_mapped_ltl_formula(){
        return ltlSpec.get_mapped_ltl_spec();
    }

    std::vector<std::string> get_in_vars(){
        return ltlSpec.get_ltl_spec_in_vars();
    }
    std::vector<std::string> get_out_vars(){
        return ltlSpec.get_ltl_spec_out_vars();
    }    
    std::vector<std::string> get_mapped_in_vars(){
        return ltlSpec.get_mapped_ltl_spec_in_vars();
    }
    std::vector<std::string> get_mapped_out_vars(){
        return ltlSpec.get_mapped_ltl_spec_out_vars();        
    }


    strix_aut::letter_t get_complete_clause(const SymState& symState, const symbolic_t control) const {
        return ltlSpec.get_complete_clause(symState, control);
    }

    std::vector<SymState> get_posts(const SymState& state, const symbolic_t control) const {
        return model.get_posts(state, control);
    }

    SymState get_initial_state() const {
        return model.get_initial_state();
    }

    bool is_sym_state_in_AP(const SymState& state, const std::string& ap){
        return ltlSpec.is_sym_state_in_AP(state.value, ap);
    }
};
