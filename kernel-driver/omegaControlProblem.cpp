/*
* omegaControlProblem.cpp
*
*  date    : 10.09.2020
*  author  : M. Khaled
*  details : a class for symbolic control problems.
* 
*/

#include "omegaControlProblem.h"

namespace pFacesOmegaKernels{

// ------------------------------------------
// class SymState
// ------------------------------------------
SymState::SymState()
:type(SYM_STATE_TYPE::DUMMY_STATE),value(0){
}

SymState::SymState(SYM_STATE_TYPE _type)
:type(_type),value(0){
}

SymState::SymState(SYM_STATE_TYPE _type, symbolic_t _value)
:type(_type),value(_value){
}

bool SymState::operator==(const SymState& anotherSymState) const {
    return (type == anotherSymState.type)&&(value == anotherSymState.value);
}


// ------------------------------------------
// class SymModel
// ------------------------------------------
template<class F>
SymModel<F>::SymModel(const size_t _n_sym_states, const size_t _n_sym_controls, const symbolic_t sym_inital_state, F& post_func)
:n_sym_states(_n_sym_states), n_sym_controls(_n_sym_controls), 
initial_state(SymState(SymState::SYM_STATE_TYPE::NORMAL_STATE, sym_inital_state)),
overflow_state(SymState(SymState::SYM_STATE_TYPE::OVERFLOW_STATE, _n_sym_states)),
dummy_state(SymState(SymState::SYM_STATE_TYPE::DUMMY_STATE, _n_sym_states+1)),
get_sym_posts(post_func){
}

template<class F>
bool SymModel<F>::is_valid_sym_state(symbolic_t state) const {
    
    return (state < n_sym_states);
}

template<class F>
bool SymModel<F>::is_valid_sym_control(symbolic_t control) const {
    
    return (control < n_sym_controls);
}

template<class F>
bool SymModel<F>::is_overflow_state(const SymState& state) const {
    
    if(state.type == SymState::SYM_STATE_TYPE::OVERFLOW_STATE)
        return true;

    return false;
}

template<class F>
bool SymModel<F>::is_dummy_state(const SymState& state) const {
    
    if(state.type == SymState::SYM_STATE_TYPE::DUMMY_STATE)
        return true;

    return false;
}

template<class F>
SymState SymModel<F>::get_initial_state() const {
    return initial_state;
}

template<class F>
SymState SymModel<F>::get_dummy_state() const {
    return dummy_state;
}

template<class F>
SymState SymModel<F>::get_overflow_state() const {
    return overflow_state;
}

template<class F>
size_t SymModel<F>::get_n_states(){
    return n_sym_states;
}

template<class F>
size_t SymModel<F>::get_n_controls(){
    return n_sym_controls;
}

template<class F>
std::vector<SymState> SymModel<F>::get_posts(const SymState& state, const symbolic_t control) const {

    static std::vector<SymState> ret_overflow = {overflow_state};

    // is dummy ?
    if(is_dummy_state(state))
        throw std::runtime_error("SymModel::get_posts: no posts for DUMMY states.");

    // is overflow ?
    if(is_overflow_state(state))
        return ret_overflow;

    // it is a normal state
    if(is_valid_sym_state(state.value) && is_valid_sym_control(control)){
        std::vector<symbolic_t> sym_post_states = get_sym_posts(state.value, control);
        std::vector<SymState> ret;
        
        for (symbolic_t sym_state : sym_post_states){

            if(!is_valid_sym_state(sym_state))
                return ret_overflow;

            ret.push_back(SymState(SymState::SYM_STATE_TYPE::NORMAL_STATE, sym_state));
        }

        return ret;
    }
    else
        throw std::runtime_error("SymModel::get_posts: Invalid input state or control.");
}

template<class F>
SymState SymModel<F>::construct_state(const symbolic_t val){
    
    if(val == dummy_state.value)
        return get_dummy_state();

    if(val == overflow_state.value)
        return get_overflow_state();

    return SymState(SymState::SYM_STATE_TYPE::NORMAL_STATE, val);

}

template<class F>
void SymModel<F>::print_info(){
    std::cout << "Num states: " << n_sym_states << std::endl; 
    std::cout << "Num controls: " << n_sym_controls << std::endl; 
    std::cout << "Initial state: " << initial_state.value << std::endl; 
    std::cout << "Transitions: " << std::endl;

    for(size_t x=0; x<n_sym_states; x++){
        for(size_t u=0; u<n_sym_controls; u++){
            auto posts = get_sym_posts(x, u);
            for(size_t xx_i=0; xx_i<posts.size(); xx_i++){
                std::cout << "(" << x << "," << u << "," << posts[xx_i] << ") ";
            }
        }
    }
    std::cout << std::endl;
}

/* force generating the class implementation for function of type post_func_t */
template class SymModel<post_func_t>;


// ------------------------------------------
// class SymSpec
// ------------------------------------------
template<class L1, class L2>
bool SymSpec<L1, L2>::is_valid_AP(std::string AP){

    // first letter must be alpha    
    if(!std::isalpha(AP[0]))
        return false;

    // all letters must be lower alpha or numeric
    for(char letter : AP)
        if(!(
            (std::islower(letter) && std::isalpha(letter))
            ||
            std::isdigit(letter)
            ))
            return false;


    return true;
}

template<class L1, class L2>
size_t SymSpec<L1, L2>::count_all_APs() const {
    return state_APs.size() + control_APs.size();
}

template<class L1, class L2>
size_t SymSpec<L1, L2>::count_state_APs() const {
    return state_APs.size();
}

template<class L1, class L2>
size_t SymSpec<L1, L2>::count_control_APs() const {
    return control_APs.size();
}

template<class L1, class L2>
size_t SymSpec<L1, L2>::count_DPA_states() const {
    return dpa.getStatesCount();
}

template<class L1, class L2>
SymSpec<L1, L2>::SymSpec(
        const std::vector<std::string>& _state_APs, 
        const std::vector<std::string>& _control_APs, 
        const std::string& _ltl_formula,
        L1& _L_x,
        L2& _L_u):
    L_x(_L_x),
    L_u(_L_u),
    dpa(TotalDPA(_state_APs, _control_APs, _ltl_formula)){

    // fill local data
    add_ltl_formula(_ltl_formula);
    add_state_APs(_state_APs);
    add_control_APs(_control_APs);
}

template<class L1, class L2>
void SymSpec<L1, L2>::add_ltl_formula(std::string _ltl_spec){
    ltl_spec = _ltl_spec;
}

template<class L1, class L2>
void SymSpec<L1, L2>::add_state_APs(const std::vector<std::string>& _state_APs){

    if(_state_APs.size() >= max_possible_state_APs)
        throw std::runtime_error("SymSpec::add_state_APs: Maximum number of possible state APs reached !");

    for(auto state_AP : _state_APs){

        if(pfacesUtils::isVectorElement(control_APs, state_AP))
            throw std::runtime_error("SymSpec::add_state_AP: One provided state AP is already used as a control AP.");            

        if(!is_valid_AP(state_AP))
            throw std::runtime_error(
                std::string("SymSpec::add_state_AP: Invalid state AP: an AP must be ") + valid_AP_descr
                );
    }

    state_APs = _state_APs;
}

template<class L1, class L2>
void SymSpec<L1, L2>::add_control_APs(const std::vector<std::string>& _control_APs){

    if(_control_APs.size() >= max_possible_control_APs)
        throw std::runtime_error("SymSpec::add_control_AP: Maximum number of possible contorl APs reached !");

    for(auto control_AP : _control_APs){

        if(pfacesUtils::isVectorElement(state_APs, control_AP))
            throw std::runtime_error("SymSpec::add_control_AP: The provided control AP is already used as a state AP.");

        if(!is_valid_AP(control_AP))
            throw std::runtime_error(
                std::string("SymSpec::add_control_AP: Invalid control AP: an AP must be ") + valid_AP_descr
                );

    }

    control_APs = _control_APs;
}

template<class L1, class L2>
std::vector<std::string> SymSpec<L1, L2>::get_state_APs(){
    return state_APs;
}

template<class L1, class L2>
std::vector<std::string> SymSpec<L1, L2>::get_control_APs(){
    return control_APs;
}

template<class L1, class L2>
std::string SymSpec<L1, L2>::get_ltl_formula(){
    return ltl_spec;
}

template<class L1, class L2>
strix_aut::letter_t SymSpec<L1, L2>::get_complete_clause(const SymState& symState, const symbolic_t control) const {

    // bits  :                        ... 1 0           
    // vars  :    control_APs       state_APs     

    // start with empty clause
    strix_aut::letter_t clause = 0;

    // add state APs having 'symState'
    if(symState.type == SymState::SYM_STATE_TYPE::NORMAL_STATE && count_state_APs() > 0){
        symbolic_t state = symState.value;
        strix_aut::letter_t state_APs_clause = L_x(state);
        clause = clause | state_APs_clause;
    }

    // add control APs having 'control'
    if(count_control_APs() > 0){
        strix_aut::letter_t control_APs_clause = L_u(control);
        control_APs_clause = control_APs_clause << count_state_APs();
        clause = clause | control_APs_clause;
    }


    return clause;
}


template<class L1, class L2>
void SymSpec<L1, L2>::print_info(){
    dpa.printInfo();
}


/* force generating the class implementation for functions of types: L_x_func_t and L_u_func_t */
template class SymSpec<L_x_func_t, L_u_func_t>;

}