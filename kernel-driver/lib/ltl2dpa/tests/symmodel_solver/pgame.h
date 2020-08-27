#pragma once

#include <iostream>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <cstring>
#include <limits>
#include <algorithm>
#include <cstdint>
#include <bitset>
#include <map>
#include <queue>
#include <memory>
#include <unordered_map>
#include <unordered_set>

// boost is only used to hash a range (e.g., a vector)
// we may replace it with another simpler function or extract their
// hash range
#include <boost/functional/hash.hpp>
#include "StrixLtl2Dpa.h"

struct Edge {
    strix_aut::node_id_t successor;
    strix_aut::color_t color;

    Edge() : successor(strix_aut::NODE_NONE), color(0) {};
    Edge(const strix_aut::node_id_t successor, const strix_aut::color_t color) : successor(successor), color(color) {};

    bool operator==(const Edge& other) const {
        return successor == other.successor && color == other.color;
    }
    bool operator!=(const Edge& other) const {
        return !(operator==(other));
    }
    bool operator<(const Edge& other) const {
        return std::tie(successor, color) < std::tie(other.successor, other.color);
    }
};
template <>
struct std::hash<Edge> {
    std::size_t operator()(const Edge& edge) const {
        std::size_t seed = 0;
        boost::hash_combine(seed, edge.successor);
        boost::hash_combine(seed, edge.color);
        return seed;
    }
};

// some functions for hashing different types of data
size_t hash_value(const Edge& edge) {
    return std::hash<Edge>()(edge);
}
template <typename Container>
struct container_hash {
    std::size_t operator()(Container const& c) const {
        auto seed = boost::hash_range(c.first.cbegin(), c.first.cend());
        boost::hash_combine(seed, c.second.value);
        return seed;
    }
};




struct ScoredProductState {
    double score;
    strix_aut::node_id_t ref_id;

    ScoredProductState() :
        score(0.0),
        ref_id(strix_aut::NODE_NONE)
    {}
    ScoredProductState(double score, strix_aut::node_id_t ref_id) :
        score(score),
        ref_id(ref_id)
    {}
};
struct ScoredProductStateComparator {
    bool operator() (ScoredProductState& s1, ScoredProductState& s2) {
        return s1.score < s2.score;
    }
};

struct MinMaxState {
    double min_score;
    double max_score;
    strix_aut::node_id_t ref_id;

    MinMaxState(const ScoredProductState& product_state) :
        min_score(product_state.score),
        max_score(product_state.score),
        ref_id(product_state.ref_id)
    {}
    MinMaxState(double score, strix_aut::node_id_t ref_id) :
        min_score(score),
        max_score(score),
        ref_id(ref_id)
    {}
    MinMaxState(double min_score, double max_score, strix_aut::node_id_t ref_id) :
        min_score(min_score),
        max_score(max_score),
        ref_id(ref_id)
    {}
};



template <class T, class Container = std::vector<T>, class Compare = std::less<T> >
class PQ : public std::priority_queue<T, Container, Compare> {
public:
    Container& container() { return this->c; }
};
typedef PQ<ScoredProductState, std::deque<ScoredProductState>, ScoredProductStateComparator> state_queue;
    

class PGame{

    // a suitable number for reserving initial items in the vectors
    static constexpr size_t RESERVE = 4096;    

    // a link to the DPA-tree-struct
    strix_aut::AutomatonTreeStructure& structure;

    // input/output iterators
    strix_aut::letter_t true_inputs_mask;
    strix_aut::letter_t true_outputs_mask;
    strix_aut::letter_t false_inputs_mask;
    strix_aut::letter_t false_outputs_mask;


    // node/edge management
    size_t product_state_size;
    strix_aut::node_id_t initial_node;
    strix_aut::node_id_t initial_node_ref;
    strix_aut::node_id_t n_env_nodes;
    strix_aut::node_id_t n_sys_nodes;
    strix_aut::edge_id_t n_sys_edges;
    strix_aut::edge_id_t n_env_edges;    
    std::vector<strix_aut::edge_id_t> sys_succs_begin;
    std::vector<strix_aut::edge_id_t> env_succs_begin;
    std::vector<Edge> sys_succs;
    std::vector<strix_aut::node_id_t> env_succs;
    std::vector<std::vector<symbolic_t>> sys_output;
    std::vector<std::vector<symbolic_t>> env_input;
    std::vector<strix_aut::node_id_t> env_node_map;
    std::vector<bool> env_node_reachable;
    

    // winning things
    std::vector<strix_aut::Player> sys_winner;
    std::vector<strix_aut::Player> env_winner;

public:
    size_t n_inputs;             // number of input vars
    size_t n_outputs;            // number of output vars
    bool complete;               // signalling that the arena is completely constructed
    strix_aut::Parity parity;    // parity of the game
    strix_aut::color_t n_colors; // number of colors

    PGame(const size_t _n_inputs, const size_t _n_outputs, strix_aut::AutomatonTreeStructure& _structure):
        structure(_structure), 
        n_inputs(_n_inputs), n_outputs(_n_outputs),
        complete(false), 
        parity(_structure.getParity()), n_colors(_structure.getMaxColor() + 1),
        initial_node(0), n_env_nodes(0), n_sys_nodes(0), n_sys_edges(0), n_env_edges(0){


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
            atomic_proposition_status_t status = _structure.getVariableStatus(a);
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
            atomic_proposition_status_t status = _structure.getVariableStatus(b);
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
    ~PGame() {}

    template<class F> 
    void constructArena(const SymControlProblem<F>& symControlProblem, const int verbosity = 0){

        // get initial state of the Spec-DPA-tree
        const strix_aut::product_state_t initial_state = structure.getInitialState();

        product_state_size = initial_state.size();

        // the states
        std::vector<std::pair<strix_aut::product_state_t, SymState>> states;
        states.reserve(RESERVE);

        // map from product states in queue to scores and reference ids
        std::unordered_map<std::pair<strix_aut::product_state_t, SymState>, MinMaxState, container_hash<std::pair<strix_aut::product_state_t, SymState>> > state_map;  

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
        std::pair<strix_aut::product_state_t,SymState> top_node_state({}, SymState(SYM_STATE_TYPE::DUMMY));
        states.push_back(top_node_state);

        // add the initial state
        // DPA-state: initial product state
        // SymState: initial symbolic state
        initial_node_ref = env_node_map.size();
        env_node_map.push_back(strix_aut::NODE_NONE);
        env_node_reachable.push_back(true);  
        ScoredProductState initial(1.0, initial_node_ref);
        state_map.insert({ std::pair<strix_aut::product_state_t,SymState>(initial_state, symControlProblem.get_initial_state()), MinMaxState(initial) });
        queue_new_states.push(initial);
        std::pair<strix_aut::product_state_t,SymState> initial_node_state(std::move(initial_state), symControlProblem.get_initial_state());
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


            // for all inputs : symbolic states
            std::vector<symbolic_t> possible_env_sym_states = {states[ref_id].second.value};
            for (symbolic_t sym_state : possible_env_sym_states){

                // new sys node with successors
                strix_aut::node_id_t sys_node = n_sys_nodes + cur_n_sys_nodes;
                std::map<Edge, std::vector<symbolic_t>> sys_successors;
                strix_aut::edge_id_t cur_sys_node_n_sys_edges = 0;

                // the state of the sym model
                SymState input_sym_state(SYM_STATE_TYPE::NORMAL, sym_state);


                // for all outputs : the controls
                for (symbolic_t sym_control = 0; sym_control < symControlProblem.get_n_controls(); sym_control++){

                    // compute joint letter for automata lookup
                    strix_aut::letter_t letter = symControlProblem.get_complete_clause(input_sym_state, sym_control);

                    // a var for the new state (yes it is only one as this is a *D*PA )
                    strix_aut::product_state_t new_dpa_state(product_state_size);

                    // get the next state (store in new_dpa_state)
                    const strix_aut::ColorScore cs = structure.getSuccessor(states[ref_id].first, new_dpa_state, letter);         

                    // get the color of this transition
                    const strix_aut::color_t color = cs.color;                               

                    // the new states of the sym model
                    std::vector<SymState> new_sym_states = symControlProblem.get_posts(input_sym_state, sym_control);

                    // score
                    double score = -(double)(env_node_map.size());


                    // if not a buttom
                    if (!structure.isBottomState(new_dpa_state)) {

                        // for all post states of the sym model
                        for(auto new_sym_state : new_sym_states){

                            // score
                            strix_aut::node_id_t succ = env_node_map.size();

                            // is it top ?
                            if (structure.isTopState(new_dpa_state)) {
                                succ = top_node_ref;
                            }
                            // now: it is not top or buttom
                            else {
                                auto result = state_map.insert({ std::pair<strix_aut::product_state_t,SymState>(new_dpa_state,new_sym_state), MinMaxState(score, succ) });

                                // is it new there ? (we use the result from the insert on the unordered_map state_map)
                                if (result.second){
                                    env_node_map.push_back(strix_aut::NODE_NONE);
                                    env_node_reachable.push_back(true);

                                    std::pair<strix_aut::product_state_t,SymState> new_node_state(std::move(new_dpa_state), new_sym_state);
                                    states.push_back(new_node_state);

                                    queue_new_states.push(ScoredProductState(score, succ));
                                }
                                else {
                                    succ = result.first->second.ref_id;
                                }
                            }    

                            // now, add the edge if not loosing
                            const strix_aut::node_id_t succ_node = env_node_map[succ];
                            if (succ_node != strix_aut::NODE_BOTTOM) {
                                Edge edge(succ, color);
                                auto const result = sys_successors.insert({ edge, {sym_control} });
                                if (!result.second) {
                                    result.first->second.push_back(sym_control);
                                }
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

    void print_info() const {
        std::cout << "Number of env nodes  : " << n_env_nodes << std::endl;
        std::cout << "Number of sys nodes  : " << n_sys_nodes << std::endl;
        std::cout << "Number of env edges  : " << n_env_edges << std::endl;
        std::cout << "Number of sys edges  : " << n_sys_edges << std::endl;
        std::cout << "Number of colors     : " << n_colors << std::endl;
    }

    strix_aut::node_id_t get_initial_node(){
        return initial_node;
    }

    strix_aut::node_id_t get_n_env_nodes(){
        return n_env_nodes;
    }
    strix_aut::node_id_t get_n_env_edges(){
        return n_env_edges;
    }
    strix_aut::node_id_t get_n_sys_edges(){
        return n_sys_edges;
    }
    strix_aut::node_id_t get_n_sys_nodes(){
        return n_sys_nodes;
    }


    strix_aut::Player getSysWinner(strix_aut::node_id_t sys_node) const {
        return sys_winner[sys_node];
    }

    strix_aut::Player getEnvWinner(strix_aut::node_id_t env_node) const {
        return env_winner[env_node];
    }

    void setSysWinner(strix_aut::node_id_t sys_node, strix_aut::Player winner) {
        sys_winner[sys_node] = winner;
    }

    inline void setEnvWinner(strix_aut::node_id_t env_node, strix_aut::Player winner) {
        env_winner[env_node] = winner;
    }

    strix_aut::edge_id_t getSysSuccsBegin(strix_aut::node_id_t sys_node) const { 
        return sys_succs_begin[sys_node]; 
    }

    strix_aut::edge_id_t getSysSuccsEnd(strix_aut::node_id_t sys_node) const { 
        return sys_succs_begin[sys_node + 1]; 
    }

    Edge getSysEdge(strix_aut::edge_id_t sys_edge) const {
        return Edge(env_node_map[sys_succs[sys_edge].successor], sys_succs[sys_edge].color);
    }

    std::vector<symbolic_t> getSysOutput(strix_aut::edge_id_t sys_edge) const { 
        return sys_output[sys_edge]; 
    }

    strix_aut::edge_id_t getEnvSuccsBegin(strix_aut::node_id_t env_node) const { 
        return env_succs_begin[env_node]; 
    }

    strix_aut::edge_id_t getEnvSuccsEnd(strix_aut::node_id_t env_node) const { 
        return env_succs_begin[env_node + 1]; 
    }

    strix_aut::node_id_t getEnvEdge(strix_aut::edge_id_t env_edge) const { 
        return env_succs[env_edge]; 
    }

    std::vector<symbolic_t> getEnvInput(strix_aut::edge_id_t env_edge) const {
        return env_input[env_edge]; 
    }

};