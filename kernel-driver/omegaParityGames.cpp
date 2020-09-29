/*
* omegaParityGames.cpp
*
*  date    : 15.09.2020
*  author  : M. Khaled
*  details : classes for Parity games and their solvers.
* 
*/

#include "omegaParityGames.h"

#include <map>
#include <queue>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <boost/functional/hash.hpp>


// this poor hasher must stay outside of namespace due to
// a known gcc bug !
template <>
struct std::hash<pFacesOmegaKernels::GameEdge> {
    std::size_t operator()(const pFacesOmegaKernels::GameEdge& edge) const {
        std::size_t seed = 0;
        boost::hash_combine(seed, edge.successor);
        boost::hash_combine(seed, edge.color);
        return seed;
    }
};

namespace pFacesOmegaKernels{

// boost-based container hash
template <typename Container>
struct container_hash {
    std::size_t operator()(Container const& c) const {
        auto seed = boost::hash_range(std::get<0>(c).cbegin(), std::get<0>(c).cend());
        boost::hash_combine(seed, std::get<1>(c).value);
        boost::hash_combine(seed, std::get<2>(c));
        return seed;
    }
};


// ------------------------------------------
// class GameEdge
// ------------------------------------------
GameEdge::GameEdge():
successor(strix_aut::NODE_NONE), color(0) {
}

GameEdge::GameEdge(const strix_aut::node_id_t successor, const strix_aut::color_t color):
successor(successor), color(color) {
}

bool GameEdge::operator==(const GameEdge& other) const {
    return successor == other.successor && color == other.color;
}

bool GameEdge::operator!=(const GameEdge& other) const {
    return !(operator==(other));
}

bool GameEdge::operator<(const GameEdge& other) const {
    return std::tie(successor, color) < std::tie(other.successor, other.color);
}

// a boost-based hasher for GameEdge
size_t hash_value(const GameEdge& edge) {
    return std::hash<GameEdge>()(edge);
}


// ------------------------------------------
// class ScoredProductState
// ------------------------------------------
ScoredProductState::ScoredProductState():
score(0.0), ref_id(strix_aut::NODE_NONE){
}

ScoredProductState::ScoredProductState(double score, strix_aut::node_id_t ref_id):
score(score), ref_id(ref_id){
}

// score-product state comparator
struct ScoredProductStateComparator {
    bool operator() (ScoredProductState& s1, ScoredProductState& s2) {
        return s1.score < s2.score;
    }
};

// type: score-product priority queue
template <class T, class Container = std::vector<T>, class Compare = std::less<T> >
class PQ : public std::priority_queue<T, Container, Compare> {
public:
    Container& container() { return this->c; }
};
typedef PQ<ScoredProductState, std::deque<ScoredProductState>, ScoredProductStateComparator> state_queue;


// a type for Environment state:  (DPA state, Model state, control input)
typedef std::tuple<strix_aut::product_state_t, SymState, symbolic_t> env_state_t;


// ------------------------------------------
// class PGame
// ------------------------------------------
template<class T, class L1, class L2>
PGame<T, L1, L2>::PGame(SymSpec<L1, L2>& _sym_spec, SymModel<T>& _sym_model):
sym_spec(_sym_spec), sym_model(_sym_model), initial_node(0),
n_env_nodes(0), n_sys_nodes(0), n_sys_edges(0), n_env_edges(0),
n_inputs(_sym_spec.count_state_APs()), n_outputs(_sym_spec.count_control_APs()),
complete(false), 
parity(_sym_spec.dpa.getParity()), n_colors(_sym_spec.dpa.getMaxColor() + 1){

    // reserve some space in the vector to avoid initial resizing, which needs locking for the parallel construction
    sys_succs_begin.reserve(RESERVE);
    sys_succs.reserve(RESERVE);
    sys_output.reserve(RESERVE);
    env_succs_begin.reserve(RESERVE);
    env_succs.reserve(RESERVE);
    env_input.reserve(RESERVE);
    env_node_map.reserve(RESERVE);
    env_node_reachable.reserve(RESERVE);
    sys_winner.reserve(RESERVE);
    env_winner.reserve(RESERVE);

    // initialize the nodes/edges
    sys_succs_begin.push_back(0);
    env_succs_begin.push_back(0);

    // construct masks for unused and always true or false inputs and outputs
    true_inputs_mask = 0;
    true_outputs_mask = 0;
    false_inputs_mask = 0;
    false_outputs_mask = 0;     

    // check which inputs are CONS_TRUE or CONS_FALSE
    for (strix_aut::letter_t a = 0; a < n_inputs; a++) {
        const strix_aut::letter_t bit = ((strix_aut::letter_t)1 << a);
        atomic_proposition_status_t status = _sym_spec.dpa.getVariableStatus(a);
        if (status != USED) {
            if (status == CONSTANT_TRUE) {
                true_inputs_mask |= bit;
            }
            else if (status == CONSTANT_FALSE) {
                false_inputs_mask |= bit;
            }
            else {
                if (status != UNUSED){
                    std::cerr << "Invalid status for input var (idx=" << a << "): " << status << std::endl;
                    assert(false);
                }
            }
        }
    }        

    // check which out vars are CONS_TRUE or CONS_FALSE
    for (strix_aut::letter_t a = 0; a < n_outputs; a++) {
        const strix_aut::letter_t bit = ((strix_aut::letter_t)1 << a);
        const strix_aut::letter_t b = a + n_inputs;
        atomic_proposition_status_t status = _sym_spec.dpa.getVariableStatus(b);
        if (status != USED) {
            if (status == CONSTANT_TRUE) {
                true_outputs_mask |= bit;
            }
            else if (status == CONSTANT_FALSE) {
                false_outputs_mask |= bit;
            }
            else {
                if (status != UNUSED){
                    std::cerr << "Invalid status for output var (idx=" << a << "): " << status << std::endl;
                    assert(false);
                }
            }
        }
    }

}

template<class T, class L1, class L2>
PGame<T, L1, L2>::~PGame() {
}

template<class T, class L1, class L2>
void PGame<T, L1, L2>::constructArena(){

    // get initial state of the Spec-DPA-tree
    const strix_aut::product_state_t dpa_initial_state = sym_spec.dpa.getInitialState();

    product_state_size = dpa_initial_state.size();

    // the states
    std::vector<env_state_t> states;
    states.reserve(RESERVE);

    // map from product states in queue to scores and reference ids
    std::unordered_map<env_state_t, ScoredProductState, container_hash<env_state_t> > state_map;  

    // queues for new states
    state_queue queue_new_states;

    // cache for system nodes
    auto sys_node_hash = [this](const strix_aut::node_id_t sys_node) {
        const size_t begin = sys_succs_begin[sys_node];
        const size_t end = sys_succs_begin[sys_node + 1];
        size_t seed = 0;
        boost::hash_range(seed, sys_succs.cbegin() + begin, sys_succs.cbegin() + end);
        boost::hash_range(seed, sys_output.cbegin() + begin, sys_output.cbegin() + end);
        return seed;
    };
    auto sys_node_equal = [this](const strix_aut::node_id_t sys_node_1, const strix_aut::node_id_t sys_node_2) {
        const size_t begin1 = sys_succs_begin[sys_node_1];
        const size_t length1 = sys_succs_begin[sys_node_1 + 1] - begin1;
        const size_t begin2 = sys_succs_begin[sys_node_2];
        const size_t length2 = sys_succs_begin[sys_node_2 + 1] - begin2;
        if (length1 != length2) {
            return false;
        }
        else {
            bool result = true;
            for (size_t j = 0; j < length1; j++) {
                if (
                        (sys_succs[begin1 + j] != sys_succs[begin2 + j]) ||
                        (sys_output[begin1 + j] != sys_output[begin2 + j])
                ) {
                    result = false;
                    break;
                }
            }
            return result;
        }
    };
    auto sys_node_map = std::unordered_set<strix_aut::node_id_t, decltype(sys_node_hash), decltype(sys_node_equal)>(RESERVE, sys_node_hash, sys_node_equal);  


    // a map from memory ids (for solver) to ref ids (for looking up states)
    std::unordered_map<strix_aut::node_id_t, strix_aut::node_id_t> env_node_to_ref_id;              

    // add ref for top node
    // DPA-state: TOP
    // SymState: DUMMY
    const strix_aut::node_id_t top_node_ref = env_node_map.size();
    env_node_map.push_back(strix_aut::NODE_TOP);
    env_node_reachable.push_back(true);
    env_state_t top_node_state = std::make_tuple(strix_aut::product_state_t(), sym_model.get_dummy_state(), 0);
    states.push_back(top_node_state);

    // add the initial state
    // DPA-state: initial product state
    // SymState: initial symbolic state
    initial_node_ref = env_node_map.size();
    env_node_map.push_back(strix_aut::NODE_NONE);
    env_node_reachable.push_back(true);  
    ScoredProductState initial(1.0, initial_node_ref);
    state_map.insert({ std::make_tuple(dpa_initial_state, sym_model.get_dummy_state(), 0), ScoredProductState(initial) });
    queue_new_states.push(initial);
    env_state_t initial_node_state = std::make_tuple(std::move(dpa_initial_state), sym_model.get_dummy_state(), 0);
    states.push_back(initial_node_state);              

    // the exploration loop : keep exploring till no more new states
    while (!queue_new_states.empty()) {

        // get next state
        ScoredProductState scored_state = std::move(queue_new_states.top());
        queue_new_states.pop();
        const strix_aut::node_id_t ref_id = scored_state.ref_id;
        
        // already explored ?
        if (env_node_map[ref_id] != strix_aut::NODE_NONE) {
            continue;
        }

        const strix_aut::node_id_t env_node = n_env_nodes;
        env_node_map[ref_id] = env_node;
        env_node_to_ref_id.insert({ env_node, ref_id });    

        strix_aut::edge_id_t cur_env_node_n_sys_edges = 0;
        strix_aut::node_id_t cur_n_sys_nodes = 0;
        std::map<strix_aut::node_id_t, std::vector<symbolic_t>> env_successors;


        // collect next model states to be used for making the env edges
        std::vector<symbolic_t> env_sym_states;
        if(sym_model.is_dummy_state(std::get<1>(states[ref_id]))){
            env_sym_states = {sym_model.get_initial_state().value};
        }
        else {
            symbolic_t sym_model_state = std::get<1>(states[ref_id]).value;
            symbolic_t sym_control_inp = std::get<2>(states[ref_id]);

            std::vector<SymState> mdl_posts = sym_model.get_posts(sym_model.construct_state(sym_model_state), sym_control_inp);
            for (SymState mdl_post : mdl_posts){
                env_sym_states.push_back(mdl_post.value);
            }
        }

        // for all inputs (env edges): symbolic states
        for (symbolic_t sym_state : env_sym_states){

            // the state of the sym model
            SymState current_sym_state = sym_model.construct_state(sym_state);
            
            // new sys node with successors
            strix_aut::node_id_t sys_node = n_sys_nodes + cur_n_sys_nodes;
            std::map<GameEdge, std::vector<symbolic_t>> sys_successors;
            strix_aut::edge_id_t cur_sys_node_n_sys_edges = 0;

            // for all outputs (system edges): the controls
            for (symbolic_t sym_control = 0; sym_control < sym_model.get_n_controls(); sym_control++){

                // now er can compute joint letter for automata lookup
                strix_aut::letter_t letter = sym_spec.get_complete_clause(current_sym_state, sym_control);

                // a var for the new state (yes it is only one as this is a *D*PA )
                strix_aut::product_state_t new_dpa_state(product_state_size);

                // get the next state (store in new_dpa_state)
                const strix_aut::ColorScore cs = sym_spec.dpa.getSuccessor(std::get<0>(states[ref_id]), new_dpa_state, letter);         

                // get the color of this transition
                const strix_aut::color_t color = cs.color;                               

                // score
                double score = -(double)(env_node_map.size());

                // score
                strix_aut::node_id_t succ = env_node_map.size();

                // skip buttom states
                if (!sym_spec.dpa.isBottomState(new_dpa_state)) {

                    // is it top ?
                    if (sym_spec.dpa.isTopState(new_dpa_state)) {
                        succ = top_node_ref;
                    }
                    // now: it is not top or buttom
                    else {
                        auto result = state_map.insert({ std::make_tuple(new_dpa_state,current_sym_state, sym_control), ScoredProductState(score, succ) });

                        // is it new there ? (we use the result from the insert on the unordered_map state_map)
                        if (result.second){
                            env_node_map.push_back(strix_aut::NODE_NONE);
                            env_node_reachable.push_back(true);

                            env_state_t new_node_state = std::make_tuple(std::move(new_dpa_state), current_sym_state, sym_control);
                            states.push_back(new_node_state);

                            queue_new_states.push(ScoredProductState(score, succ));
                        }
                        else {
                            // change ???
                            succ = result.first->second.ref_id;
                        }
                    }    

                    // now, add the edge if not loosing
                    const strix_aut::node_id_t succ_node = env_node_map[succ];
                    if (succ_node != strix_aut::NODE_BOTTOM) {
                        GameEdge edge(succ, color);
                        auto const result = sys_successors.insert({ edge, {sym_control} });
                        if (!result.second) {
                            result.first->second.push_back(sym_control);
                        }
                    }
                }


            }

            cur_sys_node_n_sys_edges = sys_successors.size();

            for (const auto& it : sys_successors) {
                sys_succs.push_back(it.first);
                sys_output.push_back(it.second);
            }
            sys_succs_begin.push_back(sys_succs.size());
            sys_winner.push_back(strix_aut::Player::UNKNOWN_PLAYER);                

            auto const result = sys_node_map.insert(sys_node);
            if (result.second) {
                // new sys node
                cur_n_sys_nodes++;
            }
            else {
                // sys node already present
                sys_succs.resize(sys_succs.size() - cur_sys_node_n_sys_edges);
                sys_output.resize(sys_output.size() - cur_sys_node_n_sys_edges);
                sys_succs_begin.resize(sys_succs_begin.size() - 1);
                sys_winner.resize(sys_winner.size() - 1);
                cur_sys_node_n_sys_edges = 0;
                sys_node = *result.first;
            }

            // add input
            auto const result2 = env_successors.insert({ sys_node, {sym_state} });
            if (!result2.second) {
                result2.first->second.push_back(sym_state);
            }

            cur_env_node_n_sys_edges += cur_sys_node_n_sys_edges;                
        }

        n_env_edges += env_successors.size();
        for (const auto& it : env_successors) {
            env_succs.push_back(it.first);
            env_input.push_back(it.second);
        }
        env_succs_begin.push_back(env_succs.size());
        env_winner.push_back(strix_aut::Player::UNKNOWN_PLAYER);

        n_sys_edges += cur_env_node_n_sys_edges;
        n_sys_nodes += cur_n_sys_nodes;
        n_env_nodes++;

        //print_info();
        //std::cout << "------------------------------\n";
    }

    // some tests
    assert(n_sys_nodes + 1 == sys_succs_begin.size());
    assert(n_env_nodes + 1 == env_succs_begin.size());
    assert(n_sys_nodes == sys_winner.size());
    assert(n_env_nodes == env_winner.size());
    assert(n_sys_edges == sys_succs.size());
    assert(n_env_edges == env_succs.size());  

    // mark as completed
    complete = true;
}

template<class T, class L1, class L2>
void PGame<T, L1, L2>::print_info() const {
    std::cout << "Number of env nodes  : " << n_env_nodes << std::endl;
    std::cout << "Number of sys nodes  : " << n_sys_nodes << std::endl;
    std::cout << "Number of env edges  : " << n_env_edges << std::endl;
    std::cout << "Number of sys edges  : " << n_sys_edges << std::endl;
    std::cout << "Number of colors     : " << n_colors << std::endl;
}

template<class T, class L1, class L2>
strix_aut::node_id_t PGame<T, L1, L2>::get_initial_node(){
    return initial_node;
}

template<class T, class L1, class L2>
strix_aut::node_id_t PGame<T, L1, L2>::get_n_env_nodes(){
    return n_env_nodes;
}

template<class T, class L1, class L2>
strix_aut::node_id_t PGame<T, L1, L2>::get_n_env_edges(){
    return n_env_edges;
}

template<class T, class L1, class L2>
strix_aut::node_id_t PGame<T, L1, L2>::get_n_sys_edges(){
    return n_sys_edges;
}

template<class T, class L1, class L2>
strix_aut::node_id_t PGame<T, L1, L2>::get_n_sys_nodes(){
    return n_sys_nodes;
}

template<class T, class L1, class L2>
strix_aut::Player PGame<T, L1, L2>::getSysWinner(strix_aut::node_id_t sys_node) const {
    return sys_winner[sys_node];
}

template<class T, class L1, class L2>
strix_aut::Player PGame<T, L1, L2>::getEnvWinner(strix_aut::node_id_t env_node) const {
    return env_winner[env_node];
}

template<class T, class L1, class L2>
void PGame<T, L1, L2>::setSysWinner(strix_aut::node_id_t sys_node, strix_aut::Player winner) {
    sys_winner[sys_node] = winner;
}

template<class T, class L1, class L2>
void PGame<T, L1, L2>::setEnvWinner(strix_aut::node_id_t env_node, strix_aut::Player winner) {
    env_winner[env_node] = winner;
}

template<class T, class L1, class L2>
strix_aut::edge_id_t PGame<T, L1, L2>::getSysSuccsBegin(strix_aut::node_id_t sys_node) const { 
    return sys_succs_begin[sys_node]; 
}

template<class T, class L1, class L2>
strix_aut::edge_id_t PGame<T, L1, L2>::getSysSuccsEnd(strix_aut::node_id_t sys_node) const { 
    return sys_succs_begin[sys_node + 1]; 
}

template<class T, class L1, class L2>
GameEdge PGame<T, L1, L2>::getSysEdge(strix_aut::edge_id_t sys_edge) const {
    return GameEdge(env_node_map[sys_succs[sys_edge].successor], sys_succs[sys_edge].color);
}

template<class T, class L1, class L2>
std::vector<symbolic_t> PGame<T, L1, L2>::getSysOutput(strix_aut::edge_id_t sys_edge) const { 
    return sys_output[sys_edge]; 
}

template<class T, class L1, class L2>
strix_aut::edge_id_t PGame<T, L1, L2>::getEnvSuccsBegin(strix_aut::node_id_t env_node) const { 
    return env_succs_begin[env_node]; 
}

template<class T, class L1, class L2>
strix_aut::edge_id_t PGame<T, L1, L2>::getEnvSuccsEnd(strix_aut::node_id_t env_node) const { 
    return env_succs_begin[env_node + 1]; 
}

template<class T, class L1, class L2>
strix_aut::node_id_t PGame<T, L1, L2>::getEnvEdge(strix_aut::edge_id_t env_edge) const { 
    return env_succs[env_edge]; 
}

template<class T, class L1, class L2>
std::vector<symbolic_t> PGame<T, L1, L2>::getEnvInput(strix_aut::edge_id_t env_edge) const {
    return env_input[env_edge]; 
}

/* force the compiler to implement the class for needed function types */
template class PGame<post_func_t, L_x_func_t, L_u_func_t>;


// ------------------------------------------
// class PGSolver
// ------------------------------------------
template<class T, class L1, class L2>
void PGSolver<T, L1, L2>::init_solver(){
    n_env_nodes = arena.get_n_env_nodes();
    n_env_edges = arena.get_n_env_edges();
    n_sys_edges = arena.get_n_sys_edges();
    n_sys_nodes = arena.get_n_sys_nodes();
    winner = strix_aut::Player::UNKNOWN_PLAYER;
}

template<class T, class L1, class L2>
void PGSolver<T, L1, L2>::preprocess_and_solve_game(){
    reduce_colors();
    solve_game();
}

template<class T, class L1, class L2>
void PGSolver<T, L1, L2>::reduce_colors(){
    n_colors = arena.n_colors;

    std::vector<strix_aut::edge_id_t> color_count(n_colors, 0);

    for (strix_aut::edge_id_t i = 0; i < n_sys_edges; i++) {
        const strix_aut::color_t c = arena.getSysEdge(i).color;
        color_count[c]++;
    }

    color_map.resize(n_colors);

    strix_aut::color_t cur_color = 0;
    for (strix_aut::color_t c = 0; c < n_colors; c++) {
        if (color_count[c] != 0) {
            if ((c % 2) != (cur_color % 2)) {
                cur_color++;
            }
            color_map[c] = cur_color;
        }
    }

    n_colors = cur_color + 1;
}

template<class T, class L1, class L2>
PGSolver<T, L1, L2>::PGSolver(PGame<T, L1, L2>& arena):arena(arena){
    init_solver();
}

template<class T, class L1, class L2>
strix_aut::Player PGSolver<T, L1, L2>::getWinner() const{
    return winner;
}

template<class T, class L1, class L2>
uint8_t PGSolver<T, L1, L2>::get_sys_successor(strix_aut::edge_id_t i){
    return sys_successors[i];
}

/* force the compiler to implement the class for needed function types */
template class PGSolver<post_func_t, L_x_func_t, L_u_func_t>;


// ------------------------------------------
// class PGSolver
// ------------------------------------------

template<class T, class L1, class L2>
void PGSISolver<T, L1, L2>::print_debug(const std::string& str) const {
    if (this->verbosity >= 5) {
        std::cout << str << std::endl;
    }
}

template<class T, class L1, class L2>
void PGSISolver<T, L1, L2>::print_values_debug(){
    if (this->verbosity >= 6) {
        std::cout << std::setfill('-') << std::setw(20) << "" << std::setfill(' ') << std::endl;
        std::cout << "---- sys nodes -----" << std::endl;
        std::cout << std::setfill('-') << std::setw(20) << "" << std::setfill(' ') << std::endl;
        std::cout << "        v     o(v)    d(v)" << std::endl;
        for (size_t i = 0; i < this->n_sys_nodes; i++) {
            if (i == this->arena.get_initial_node()) {
                std::cout << ">";
            }
            else {
                std::cout << " ";
            }
            std::cout << std::setw(8) << i;

            std::cout << " [";
            for (strix_aut::edge_id_t j = this->arena.getSysSuccsBegin(i); j != this->arena.getSysSuccsEnd(i); j++) {
                if (this->sys_successors[j]) {
                    strix_aut::node_id_t s = this->arena.getSysEdge(j).successor;
                    if (s == strix_aut::NODE_TOP) {
                        std::cout << "  ⊤";
                    }
                    else if (s == strix_aut::NODE_BOTTOM) {
                        std::cout << "  ⊥";
                    }
                    else if (s == strix_aut::NODE_NONE) {
                        std::cout << "  -";
                    }
                    else {
                        std:: cout << " " << std::setw(2) << s;
                    }
                }
            }
            std::cout << " ]";

            for (strix_aut::color_t c = 0; c < this->n_colors; c++) {
                distance_t distance = this->sys_distances[i*this->n_colors + c];
                if (distance == DISTANCE_INFINITY) {
                    std::cout << "       ∞";
                    break;
                }
                else if (distance == DISTANCE_MINUS_INFINITY) {
                    std::cout << "      -∞";
                    break;
                }
                else {
                    std::cout << std::setw(8) << distance;
                }
            }
            if (this->arena.getSysWinner(i) == strix_aut::SYS_PLAYER) {
                std::cout << "  won_sys";
            }
            else if (this->arena.getSysWinner(i) == strix_aut::ENV_PLAYER) {
                std::cout << "  won_env";
            }
            std::cout << std::endl;
        }
        std::cout << std::setfill('-') << std::setw(20) << "" << std::setfill(' ') << std::endl;
        std::cout << "---- env nodes -----" << std::endl;
        std::cout << std::setfill('-') << std::setw(20) << "" << std::setfill(' ') << std::endl;
        std::cout << "        v     d(v)" << std::endl;
        for (size_t i = 0; i < this->n_env_nodes; i++) {
            if (i == this->arena.get_initial_node()) {
                std::cout << ">";
            }
            else {
                std::cout << " ";
            }
            std::cout << std::setw(8) << i;

            std::cout << " [";
            if (this->env_successors[i] == strix_aut::EDGE_BOTTOM) {
                std::cout << "⊥";
            }
            else {
                std::cout << this->arena.getEnvEdge(this->env_successors[i]);
            }
            std::cout << "]";

            for (strix_aut::color_t c = 0; c < this->n_colors; c++) {
                distance_t distance = this->env_distances[i*this->n_colors + c];
                if (distance == DISTANCE_INFINITY) {
                    std::cout << "       ∞";
                    break;
                }
                else if (distance == DISTANCE_MINUS_INFINITY) {
                    std::cout << "      -∞";
                    break;
                }
                else {
                    std::cout << std::setw(8) << distance;
                }
            }
            if (this->arena.getEnvWinner(i) == strix_aut::SYS_PLAYER) {
                std::cout << "  won_sys";
            }
            else if (this->arena.getEnvWinner(i) == strix_aut::ENV_PLAYER) {
                std::cout << "  won_env";
            }
            std::cout << std::endl;
        }
        std::cout << std::setfill('-') << std::setw(20) << "" << std::setfill(' ') << std::endl;
    }
}

template<class T, class L1, class L2>
distance_t PGSISolver<T, L1, L2>::color_distance_delta(const strix_aut::color_t& color){
    return 1 - (((this->arena.parity + color) & 1) << 1);
}

template<class T, class L1, class L2>
template <strix_aut::Player P>
void PGSISolver<T, L1, L2>::update_nodes(){

    for (strix_aut::node_id_t i = 0; i < this->n_env_nodes; i++) {
        if (this->arena.getEnvWinner(i) == strix_aut::Player::UNKNOWN_PLAYER && env_distances[i * this->n_colors] == P*DISTANCE_INFINITY) {
            // node won by current player
            this->arena.setEnvWinner(i, P);
        }
    }

    for (strix_aut::node_id_t i = 0; i < this->n_sys_nodes; i++) {
        if (this->arena.getSysWinner(i) == strix_aut::Player::UNKNOWN_PLAYER && sys_distances[i * this->n_colors] == P*DISTANCE_INFINITY) {
            // node won by current player
            this->arena.setSysWinner(i, P);
            if (P == strix_aut::SYS_PLAYER) {
                // need to deactivate non-winning edges for non-deterministic strategy
                for (strix_aut::edge_id_t j = this->arena.getSysSuccsBegin(i); j != this->arena.getSysSuccsEnd(i); j++) {
                    if (this->sys_successors[j]) {
                        auto edge = this->arena.getSysEdge(j);
                        if (
                                edge.successor < this->n_env_nodes &&
                                this->arena.getEnvWinner(edge.successor) == strix_aut::Player::UNKNOWN_PLAYER &&
                                env_distances[edge.successor * this->n_colors] < DISTANCE_INFINITY
                        ) {
                            this->sys_successors[j] = false;
                        }
                    }
                }
            }
        }
    }
    this->winner = this->arena.getEnvWinner(this->arena.get_initial_node());
}

template<class T, class L1, class L2>
template <strix_aut::Player P>
void PGSISolver<T, L1, L2>::bellman_ford_init(){
    for (strix_aut::node_id_t i = 0; i < this->n_sys_nodes; i++) {
        if (this->arena.getSysWinner(i) == P || (P == strix_aut::ENV_PLAYER && this->arena.getSysWinner(i) == strix_aut::Player::UNKNOWN_PLAYER)) {
            this->sys_distances[i * this->n_colors] = P*DISTANCE_INFINITY;
        }
        else {
            const dist_id_t k = i * this->n_colors;
            for (dist_id_t l = k; l < k + this->n_colors; l++) {
                this->sys_distances[l] = 0;
            }
        }
    }
    for (strix_aut::node_id_t i = 0; i < this->n_env_nodes; i++) {
        if (this->arena.getEnvWinner(i) == P || (P == strix_aut::SYS_PLAYER && this->arena.getEnvWinner(i) == strix_aut::Player::UNKNOWN_PLAYER)) {
            this->env_distances[i * this->n_colors] = P*DISTANCE_INFINITY;
        }
        else {
            const dist_id_t k = i * this->n_colors;
            for (dist_id_t l = k; l < k + this->n_colors; l++) {
               this->env_distances[l] = 0;
            }
        }
    }
}

template<class T, class L1, class L2>
template <strix_aut::Player P>
void PGSISolver<T, L1, L2>::bellman_ford(){
    print_debug("Executing Bellman-Ford algorithm…");
    bellman_ford_init<P>();
    print_values_debug();
    bool change = true;
    while (change) {
        print_debug("Executing Bellman-Ford iteration…");
        if (P == strix_aut::SYS_PLAYER) {
            bellman_ford_sys_iteration<P>();
        }
        else {
            bellman_ford_env_iteration<P>();
        }
        print_values_debug();
        print_debug("Executing Bellman-Ford iteration…");
        if (P == strix_aut::SYS_PLAYER) {
            change = bellman_ford_env_iteration<P>();
        }
        else {
            change = bellman_ford_sys_iteration<P>();
        }
        print_values_debug();
    }
}

template<class T, class L1, class L2>
template <strix_aut::Player P>
bool PGSISolver<T, L1, L2>::bellman_ford_sys_iteration(){
    bool change = false;
    
    for (strix_aut::node_id_t i = 0; i < this->n_sys_nodes; i++) {
        if (this->arena.getSysWinner(i) == strix_aut::Player::UNKNOWN_PLAYER) {
            const dist_id_t k = i * this->n_colors;
            if (P == strix_aut::SYS_PLAYER) {
                // need to compare against 0 for non-deterministic strategies
                for (dist_id_t l = k; l < k + this->n_colors; l++) {
                    this->sys_distances[l] = 0;
                }
            }

            for (strix_aut::edge_id_t j = this->arena.getSysSuccsBegin(i); j != this->arena.getSysSuccsEnd(i); j++) {
                if (P == strix_aut::ENV_PLAYER || this->sys_successors[j]) {
                    auto edge = this->arena.getSysEdge(j);
                    dist_id_t m = edge.successor * this->n_colors;

                    if (edge.successor == strix_aut::NODE_BOTTOM) {
                        continue;
                    }
                    else if (edge.successor == strix_aut::NODE_TOP) {
                        if (this->sys_distances[k] != DISTANCE_INFINITY) {
                            change = true;
                            this->sys_distances[k] = DISTANCE_INFINITY;
                        }
                        break;
                    }
                    else if (edge.successor < this->n_env_nodes) {
                        if (this->env_distances[m] == DISTANCE_INFINITY) {
                            if (this->sys_distances[k] != DISTANCE_INFINITY) {
                                change = true;
                                this->sys_distances[k] = DISTANCE_INFINITY;
                            }
                            break;
                        }
                        else if (this->env_distances[m] == DISTANCE_MINUS_INFINITY) {
                            // skip successor
                            continue;
                        }
                    }
                    // successor distance is finite, may not yet be explored
                    bool local_change = false;

                    const strix_aut::color_t cur_color = this->color_map[edge.color];
                    const distance_t cur_color_change = color_distance_delta(cur_color);
                    this->sys_distances[k + cur_color] -= cur_color_change;

                    for (dist_id_t l = k; l < k + this->n_colors; l++, m++) {
                        const distance_t d = this->sys_distances[l];
                        distance_t d_succ;
                        if (edge.successor < this->n_env_nodes) {
                            d_succ = this->env_distances[m];
                        }
                        else {
                            d_succ = 0;
                        }
                        if (local_change || d_succ > d) {
                            this->sys_distances[l] = d_succ;
                            local_change = true;
                        }
                        else if (d_succ != d) {
                            break;
                        }
                    }
                    this->sys_distances[k + cur_color] += cur_color_change;

                    if (local_change) {
                        change = true;
                    }
                }
            }
        }
    }
    return change;
}

template<class T, class L1, class L2>
template <strix_aut::Player P>
bool PGSISolver<T, L1, L2>::bellman_ford_env_iteration(){
    bool change = false;
    
    for (strix_aut::node_id_t i = 0; i < this->n_env_nodes; i++) {
        if (this->arena.getEnvWinner(i) == strix_aut::Player::UNKNOWN_PLAYER) {
            if (P == strix_aut::SYS_PLAYER) {
                for (strix_aut::edge_id_t j = this->arena.getEnvSuccsBegin(i); j != this->arena.getEnvSuccsEnd(i); j++) {

                    const strix_aut::node_id_t successor = this->arena.getEnvEdge(j);
                    dist_id_t m = successor * this->n_colors;

                    if (this->sys_distances[m] < DISTANCE_INFINITY) {
                        bool local_change = false;

                        const dist_id_t k = i * this->n_colors;
                        for (dist_id_t l = k; l < k + this->n_colors; l++, m++) {
                            const distance_t d = this->env_distances[l];
                            const distance_t d_succ = this->sys_distances[m];
                            if (local_change || d_succ < d) {
                                this->env_distances[l] = d_succ;
                                local_change = true;
                            }
                            else if (d_succ != d) {
                                break;
                            }
                        }
                        if (local_change) {
                            change = true;
                        }
                    }
                }
            }
            else {
                const strix_aut::edge_id_t j = this->env_successors[i];
                if (j != strix_aut::EDGE_BOTTOM) {
                    const strix_aut::edge_id_t successor = this->arena.getEnvEdge(j);
                    dist_id_t m = successor * this->n_colors;
                    const dist_id_t k = i * this->n_colors;
                    for (dist_id_t l = k; l < k + this->n_colors; l++, m++) {
                        this->env_distances[l] = this->sys_distances[m];
                    }
                }
            }
        }
    }
    return change;
}

template<class T, class L1, class L2>
bool PGSISolver<T, L1, L2>::strategy_improvement_SYS_PLAYER(){
    bool change = false;

    for (strix_aut::node_id_t i = 0; i < this->n_sys_nodes; i++) {
        const dist_id_t k = i * this->n_colors;
        if (this->arena.getSysWinner(i) == strix_aut::Player::UNKNOWN_PLAYER && this->sys_distances[k] < DISTANCE_INFINITY) {
            for (strix_aut::edge_id_t j = this->arena.getSysSuccsBegin(i); j != this->arena.getSysSuccsEnd(i); j++) {
                this->sys_successors[j] = false;
                auto edge = this->arena.getSysEdge(j);

                if (edge.successor == strix_aut::NODE_TOP) {
                    this->sys_successors[j] = true;
                    change = true;
                }
                else if (edge.successor < this->n_env_nodes && this->arena.getEnvWinner(edge.successor) != strix_aut::ENV_PLAYER) {
                    bool improvement = true;
                    dist_id_t m = edge.successor * this->n_colors;

                    const strix_aut::color_t cur_color = this->color_map[edge.color];
                    const distance_t cur_color_change = color_distance_delta(cur_color);
                    this->sys_distances[k + cur_color] -= cur_color_change;

                    for (dist_id_t l = k; l < k + this->n_colors; l++, m++) {
                        const distance_t d = this->sys_distances[l];
                        const distance_t d_succ = this->env_distances[m];
                        if (d_succ > d) {
                            // strict improvement
                            change = true;
                            break;
                        }
                        else if (d_succ != d) {
                            improvement = false;
                            break;
                        }
                    }

                    this->sys_distances[k + cur_color] += cur_color_change;

                    if (improvement) {
                        this->sys_successors[j] = true;
                    }
                }
            }
        }
    }
    return change;
}

template<class T, class L1, class L2>
bool PGSISolver<T, L1, L2>::strategy_improvement_ENV_PLAYER(){
    bool change = false;

    for (strix_aut::node_id_t i = 0; i < this->n_env_nodes; i++) {
        const dist_id_t k = i * this->n_colors;
        if (this->arena.getEnvWinner(i) == strix_aut::Player::UNKNOWN_PLAYER && this->env_distances[k] > DISTANCE_MINUS_INFINITY) {
            for (strix_aut::edge_id_t j = this->arena.getEnvSuccsBegin(i); j != this->arena.getEnvSuccsEnd(i); j++) {
                const strix_aut::node_id_t successor = this->arena.getEnvEdge(j);
                if (this->arena.getSysWinner(successor) != strix_aut::SYS_PLAYER) {
                    bool improvement = false;
                    dist_id_t m = successor * this->n_colors;
                    if (this->sys_distances[m] == DISTANCE_MINUS_INFINITY) {
                        improvement = true;
                    }
                    else {
                        for (dist_id_t l = k; l < k + this->n_colors; l++, m++) {
                            const distance_t d = this->env_distances[l];
                            const distance_t d_succ = this->sys_distances[m];
                            if (d_succ < d) {
                                // strict improvement
                                improvement = true;
                                break;
                            }
                            else if (d_succ != d) {
                                break;
                            }
                        }
                    }

                    if (improvement) {
                        change = true;
                        this->env_successors[i] = j;
                        break;
                    }
                }
            }
        }
    }
    return change;
}

template<class T, class L1, class L2>
template <strix_aut::Player P>
void PGSISolver<T, L1, L2>::strategy_iteration(){
    print_values_debug();

    bool change = true;
    while (change && this->winner == strix_aut::Player::UNKNOWN_PLAYER) {
        
        bellman_ford<P>();

        print_debug("Executing strategy improvement…");
        if (P == strix_aut::SYS_PLAYER) {
            change = strategy_improvement_SYS_PLAYER();
        } else {
            change = strategy_improvement_ENV_PLAYER();
        }

        print_values_debug();

        print_debug("Marking solved nodes");
        update_nodes<P>();
        print_values_debug();
    }
}

template<class T, class L1, class L2>
void PGSISolver<T, L1, L2>::solve_game(){
    sys_distances = std::vector<distance_t>(this->n_sys_nodes * this->n_colors, 0);
    env_distances = std::vector<distance_t>(this->n_env_nodes * this->n_colors, 0);

    this->sys_successors.resize(this->n_sys_edges, false);
    this->env_successors.resize(this->n_env_nodes, strix_aut::EDGE_BOTTOM);

    print_debug("Starting strategy iteration for sys player…");
    strategy_iteration<strix_aut::SYS_PLAYER>();

    print_debug("Starting strategy iteration for env player…");
    strategy_iteration<strix_aut::ENV_PLAYER>();

    // clear memory
    std::vector<distance_t>().swap(this->sys_distances);
    std::vector<distance_t>().swap(this->env_distances);
}

template<class T, class L1, class L2>
PGSISolver<T, L1, L2>::PGSISolver(PGame<T, L1, L2>& arena)
:PGSolver<T, L1, L2>(arena){
}

template<class T, class L1, class L2>
PGSISolver<T, L1, L2>::~PGSISolver(){   
}

/* force the compiler to implement the class for needed function types */
template class PGSISolver<post_func_t, L_x_func_t, L_u_func_t>;


}