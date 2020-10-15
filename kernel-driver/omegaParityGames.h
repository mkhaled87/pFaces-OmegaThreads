#pragma once

/*
* omegaParityGames.h
*
*  date    : 15.09.2020
*  author  : M. Khaled
*  details : classes for Parity games and their solvers.
* 
*/

#include <iostream>
#include <string>
#include <vector>

#include <Ltl2Dpa.h>
#include <pfaces-sdk.h>
#include "omegaControlProblem.h"

namespace pFacesOmegaKernels{

// a class representing the edge in a parity game structure
class GameEdge {
public:
    strix_aut::node_id_t successor;
    strix_aut::color_t color;

    GameEdge();
    GameEdge(const strix_aut::node_id_t successor, const strix_aut::color_t color);

    bool operator==(const GameEdge& other) const;
    bool operator!=(const GameEdge& other) const;
    bool operator<(const GameEdge& other) const;
};


// a class combining a product-state-reference and a score
class ScoredProductState {
public:
    double score;
    strix_aut::node_id_t ref_id;

    ScoredProductState();
    ScoredProductState(double score, strix_aut::node_id_t ref_id);
};


// a glass for parity games
template<class T, class L1, class L2>
class PGame{

    // a suitable number for reserving initial items in the vectors
    static constexpr size_t RESERVE = 4194304;    

    // a link to the DPA-tree-struct
    SymSpec<L1,L2>& sym_spec;
    SymModel<T>& sym_model;

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
    std::vector<GameEdge> sys_succs;
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

    PGame(SymSpec<L1, L2>& _sym_spec, SymModel<T>& _sym_model);
    ~PGame();

    void constructArena();
    void print_info() const;

    strix_aut::node_id_t get_initial_node();
    strix_aut::node_id_t get_n_env_nodes();
    strix_aut::node_id_t get_n_env_edges();
    strix_aut::node_id_t get_n_sys_edges();
    strix_aut::node_id_t get_n_sys_nodes();

    strix_aut::Player getSysWinner(strix_aut::node_id_t sys_node) const;
    strix_aut::Player getEnvWinner(strix_aut::node_id_t env_node) const;
    void setSysWinner(strix_aut::node_id_t sys_node, strix_aut::Player winner);
    void setEnvWinner(strix_aut::node_id_t env_node, strix_aut::Player winner);

    strix_aut::edge_id_t getSysSuccsBegin(strix_aut::node_id_t sys_node) const;
    strix_aut::edge_id_t getSysSuccsEnd(strix_aut::node_id_t sys_node) const;
    GameEdge getSysEdge(strix_aut::edge_id_t sys_edge) const;
    std::vector<symbolic_t> getSysOutput(strix_aut::edge_id_t sys_edge) const;
    strix_aut::edge_id_t getEnvSuccsBegin(strix_aut::node_id_t env_node) const;
    strix_aut::edge_id_t getEnvSuccsEnd(strix_aut::node_id_t env_node) const;
    strix_aut::node_id_t getEnvEdge(strix_aut::edge_id_t env_edge) const;
    std::vector<symbolic_t> getEnvInput(strix_aut::edge_id_t env_edge) const;

};



// base class for Parity-game solvers
template<class T, class L1, class L2>
class PGSolver {
private:

    void init_solver();
    void preprocess_and_solve_game();
    void reduce_colors();

protected:
    PGame<T, L1, L2>& arena;
    size_t verbosity;

    std::vector<uint8_t> sys_successors;
    std::vector<strix_aut::edge_id_t> env_successors;

    strix_aut::Player winner;

    strix_aut::node_id_t n_env_nodes;
    strix_aut::node_id_t n_env_edges;
    strix_aut::node_id_t n_sys_nodes;
    strix_aut::node_id_t n_sys_edges;

    strix_aut::color_t n_colors;
    std::vector<strix_aut::color_t> color_map;

    virtual void solve_game() = 0;

public:
    PGSolver(PGame<T, L1, L2>& arena);
    strix_aut::Player getWinner() const;
    
    virtual ~PGSolver(){};
    virtual void solve(size_t _verbosity=0) {
        verbosity = _verbosity;
        init_solver();
        preprocess_and_solve_game();
    }    

    uint8_t get_sys_successor(strix_aut::edge_id_t i);

};

// some types
typedef int32_t distance_t;
typedef size_t dist_id_t;

// some constants
const distance_t DISTANCE_INFINITY = std::numeric_limits<distance_t>::max() - 1;
const distance_t DISTANCE_MINUS_INFINITY = -DISTANCE_INFINITY;
static_assert(DISTANCE_INFINITY > 0, "plus infinity not positive");
static_assert(DISTANCE_INFINITY + 1 > 0, "plus infinity too large");
static_assert(DISTANCE_MINUS_INFINITY < 0, "minus infinity not negative");
static_assert(DISTANCE_MINUS_INFINITY - 1 < 0, "minus infinity too small");    

// a Parity-game solver based on strategy improvement (SI)
template<class T, class L1, class L2>
class PGSISolver : public PGSolver<T, L1, L2> {

    std::vector<distance_t> sys_distances;
    std::vector<distance_t> env_distances;

    void print_debug(const std::string& str) const;
    void print_values_debug();
    distance_t color_distance_delta(const strix_aut::color_t& color);

    template <strix_aut::Player P>
    void update_nodes();
    
    template <strix_aut::Player P>
    void bellman_ford_init();

    template <strix_aut::Player P>
    void bellman_ford();
    
    template <strix_aut::Player P>
    bool bellman_ford_sys_iteration();
    
    template <strix_aut::Player P>
    bool bellman_ford_env_iteration();

    bool strategy_improvement_SYS_PLAYER();
    bool strategy_improvement_ENV_PLAYER();

    template <strix_aut::Player P>
    void strategy_iteration();    


protected:
    void solve_game();

public:
    PGSISolver(PGame<T, L1, L2>& arena);
    ~PGSISolver();
};

}