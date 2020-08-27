#pragma once

#include <iostream>
#include <iomanip>
#include <sstream>


#include "pgame.h"

class PGSolver {
private:

    void init_solver(){
        n_env_nodes = arena.get_n_env_nodes();
        n_env_edges = arena.get_n_env_edges();
        n_sys_edges = arena.get_n_sys_edges();
        n_sys_nodes = arena.get_n_sys_nodes();
    }

    void preprocess_and_solve_game(){
        reduce_colors();
        solve_game();
    }

    void reduce_colors(){
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

protected:
    PGame& arena;
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
    PGSolver(PGame& arena):arena(arena){
        init_solver();
    }

    strix_aut::Player getWinner() const{
        return winner;
    }

    virtual ~PGSolver(){};

    virtual void solve(size_t _verbosity=0) {
        verbosity = _verbosity;
        init_solver();
        preprocess_and_solve_game();
    }

    uint8_t get_sys_successor(strix_aut::edge_id_t i){
        return sys_successors[i];
    }

};

class PGSISolver : public PGSolver {

    typedef int32_t distance_t;
    typedef size_t dist_id_t;
    static constexpr distance_t DISTANCE_INFINITY = std::numeric_limits<distance_t>::max() - 1;
    static constexpr distance_t DISTANCE_MINUS_INFINITY = -DISTANCE_INFINITY;
    static_assert(DISTANCE_INFINITY > 0, "plus infinity not positive");
    static_assert(DISTANCE_INFINITY + 1 > 0, "plus infinity too large");
    static_assert(DISTANCE_MINUS_INFINITY < 0, "minus infinity not negative");
    static_assert(DISTANCE_MINUS_INFINITY - 1 < 0, "minus infinity too small");    

private:
    std::vector<distance_t> sys_distances;
    std::vector<distance_t> env_distances;

    distance_t color_distance_delta(const strix_aut::color_t& color){
        return 1 - (((arena.parity + color) & 1) << 1);
    }

    template <strix_aut::Player P>
    void update_nodes(){

        for (strix_aut::node_id_t i = 0; i < n_env_nodes; i++) {
            if (arena.getEnvWinner(i) == strix_aut::Player::UNKNOWN_PLAYER && env_distances[i * n_colors] == P*DISTANCE_INFINITY) {
                // node won by current player
                arena.setEnvWinner(i, P);
            }
        }

        for (strix_aut::node_id_t i = 0; i < n_sys_nodes; i++) {
            if (arena.getSysWinner(i) == strix_aut::Player::UNKNOWN_PLAYER && sys_distances[i * n_colors] == P*DISTANCE_INFINITY) {
                // node won by current player
                arena.setSysWinner(i, P);
                if constexpr(P == strix_aut::SYS_PLAYER) {
                    // need to deactivate non-winning edges for non-deterministic strategy
                    for (strix_aut::edge_id_t j = arena.getSysSuccsBegin(i); j != arena.getSysSuccsEnd(i); j++) {
                        if (sys_successors[j]) {
                            const Edge edge = arena.getSysEdge(j);
                            if (
                                    edge.successor < n_env_nodes &&
                                    arena.getEnvWinner(edge.successor) == strix_aut::Player::UNKNOWN_PLAYER &&
                                    env_distances[edge.successor * n_colors] < DISTANCE_INFINITY
                            ) {
                                sys_successors[j] = false;
                            }
                        }
                    }
                }
            }
        }
        winner = arena.getEnvWinner(arena.get_initial_node());
    }
    
    template <strix_aut::Player P>
    void bellman_ford(){
        print_debug("Executing Bellman-Ford algorithm…");
        bellman_ford_init<P>();
        print_values_debug();
        bool change = true;
        while (change) {
            print_debug("Executing Bellman-Ford iteration…");
            if constexpr(P == strix_aut::SYS_PLAYER) {
                bellman_ford_sys_iteration<P>();
            }
            else {
                bellman_ford_env_iteration<P>();
            }
            print_values_debug();
            print_debug("Executing Bellman-Ford iteration…");
            if constexpr(P == strix_aut::SYS_PLAYER) {
                change = bellman_ford_env_iteration<P>();
            }
            else {
                change = bellman_ford_sys_iteration<P>();
            }
            print_values_debug();
        }
    }
    
    template <strix_aut::Player P>
    void bellman_ford_init(){
        for (strix_aut::node_id_t i = 0; i < n_sys_nodes; i++) {
            if (arena.getSysWinner(i) == P || (P == strix_aut::ENV_PLAYER && arena.getSysWinner(i) == strix_aut::Player::UNKNOWN_PLAYER)) {
                sys_distances[i * n_colors] = P*DISTANCE_INFINITY;
            }
            else {
                const dist_id_t k = i * n_colors;
                for (dist_id_t l = k; l < k + n_colors; l++) {
                    sys_distances[l] = 0;
                }
            }
        }
        for (strix_aut::node_id_t i = 0; i < n_env_nodes; i++) {
            if (arena.getEnvWinner(i) == P || (P == strix_aut::SYS_PLAYER && arena.getEnvWinner(i) == strix_aut::Player::UNKNOWN_PLAYER)) {
                env_distances[i * n_colors] = P*DISTANCE_INFINITY;
            }
            else {
                const dist_id_t k = i * n_colors;
                for (dist_id_t l = k; l < k + n_colors; l++) {
                    env_distances[l] = 0;
                }
            }
        }
    }

    
    template <strix_aut::Player P>
    bool bellman_ford_sys_iteration(){
        bool change = false;
        
        for (strix_aut::node_id_t i = 0; i < n_sys_nodes; i++) {
            if (arena.getSysWinner(i) == strix_aut::Player::UNKNOWN_PLAYER) {
                const dist_id_t k = i * n_colors;
                if constexpr(P == strix_aut::SYS_PLAYER) {
                    // need to compare against 0 for non-deterministic strategies
                    for (dist_id_t l = k; l < k + n_colors; l++) {
                        sys_distances[l] = 0;
                    }
                }

                for (strix_aut::edge_id_t j = arena.getSysSuccsBegin(i); j != arena.getSysSuccsEnd(i); j++) {
                    if (P == strix_aut::ENV_PLAYER || sys_successors[j]) {
                        const Edge edge = arena.getSysEdge(j);
                        dist_id_t m = edge.successor * n_colors;

                        if (edge.successor == strix_aut::NODE_BOTTOM) {
                            continue;
                        }
                        else if (edge.successor == strix_aut::NODE_TOP) {
                            if (sys_distances[k] != DISTANCE_INFINITY) {
                                change = true;
                                sys_distances[k] = DISTANCE_INFINITY;
                            }
                            break;
                        }
                        else if (edge.successor < n_env_nodes) {
                            if (env_distances[m] == DISTANCE_INFINITY) {
                                if (sys_distances[k] != DISTANCE_INFINITY) {
                                    change = true;
                                    sys_distances[k] = DISTANCE_INFINITY;
                                }
                                break;
                            }
                            else if (env_distances[m] == DISTANCE_MINUS_INFINITY) {
                                // skip successor
                                continue;
                            }
                        }
                        // successor distance is finite, may not yet be explored
                        bool local_change = false;

                        const strix_aut::color_t cur_color = color_map[edge.color];
                        const distance_t cur_color_change = color_distance_delta(cur_color);
                        sys_distances[k + cur_color] -= cur_color_change;

                        for (dist_id_t l = k; l < k + n_colors; l++, m++) {
                            const distance_t d = sys_distances[l];
                            distance_t d_succ;
                            if (edge.successor < n_env_nodes) {
                                d_succ = env_distances[m];
                            }
                            else {
                                d_succ = 0;
                            }
                            if (local_change || d_succ > d) {
                                sys_distances[l] = d_succ;
                                local_change = true;
                            }
                            else if (d_succ != d) {
                                break;
                            }
                        }
                        sys_distances[k + cur_color] += cur_color_change;

                        if (local_change) {
                            change = true;
                        }
                    }
                }
            }
        }
        return change;
    }
    
    template <strix_aut::Player P>
    bool bellman_ford_env_iteration(){
        bool change = false;
        
        for (strix_aut::node_id_t i = 0; i < n_env_nodes; i++) {
            if (arena.getEnvWinner(i) == strix_aut::Player::UNKNOWN_PLAYER) {
                if constexpr(P == strix_aut::SYS_PLAYER) {
                    for (strix_aut::edge_id_t j = arena.getEnvSuccsBegin(i); j != arena.getEnvSuccsEnd(i); j++) {

                        const strix_aut::node_id_t successor = arena.getEnvEdge(j);
                        dist_id_t m = successor * n_colors;

                        if (sys_distances[m] < DISTANCE_INFINITY) {
                            bool local_change = false;

                            const dist_id_t k = i * n_colors;
                            for (dist_id_t l = k; l < k + n_colors; l++, m++) {
                                const distance_t d = env_distances[l];
                                const distance_t d_succ = sys_distances[m];
                                if (local_change || d_succ < d) {
                                    env_distances[l] = d_succ;
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
                    const strix_aut::edge_id_t j = env_successors[i];
                    if (j != strix_aut::EDGE_BOTTOM) {
                        const strix_aut::edge_id_t successor = arena.getEnvEdge(j);
                        dist_id_t m = successor * n_colors;
                        const dist_id_t k = i * n_colors;
                        for (dist_id_t l = k; l < k + n_colors; l++, m++) {
                            env_distances[l] = sys_distances[m];
                        }
                    }
                }
            }
        }
        return change;
    }
    
    template <strix_aut::Player P>
    void strategy_iteration(){
        print_values_debug();

        bool change = true;
        while (change && winner == strix_aut::Player::UNKNOWN_PLAYER) {
            
            bellman_ford<P>();

            print_debug("Executing strategy improvement…");
            change = strategy_improvement<P>();
            print_values_debug();

            print_debug("Marking solved nodes");
            update_nodes<P>();
            print_values_debug();
        }
    }

    template <strix_aut::Player P> bool strategy_improvement();   
    template <> bool strategy_improvement<strix_aut::SYS_PLAYER>(){
        bool change = false;

        for (strix_aut::node_id_t i = 0; i < n_sys_nodes; i++) {
            const dist_id_t k = i * n_colors;
            if (arena.getSysWinner(i) == strix_aut::Player::UNKNOWN_PLAYER && sys_distances[k] < DISTANCE_INFINITY) {
                for (strix_aut::edge_id_t j = arena.getSysSuccsBegin(i); j != arena.getSysSuccsEnd(i); j++) {
                    sys_successors[j] = false;
                    const Edge edge = arena.getSysEdge(j);

                    if (edge.successor == strix_aut::NODE_TOP) {
                        sys_successors[j] = true;
                        change = true;
                    }
                    else if (edge.successor < n_env_nodes && arena.getEnvWinner(edge.successor) != strix_aut::ENV_PLAYER) {
                        bool improvement = true;
                        dist_id_t m = edge.successor * n_colors;

                        const strix_aut::color_t cur_color = color_map[edge.color];
                        const distance_t cur_color_change = color_distance_delta(cur_color);
                        sys_distances[k + cur_color] -= cur_color_change;

                        for (dist_id_t l = k; l < k + n_colors; l++, m++) {
                            const distance_t d = sys_distances[l];
                            const distance_t d_succ = env_distances[m];
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

                        sys_distances[k + cur_color] += cur_color_change;

                        if (improvement) {
                            sys_successors[j] = true;
                        }
                    }
                }
            }
        }
        return change;
    } 
    template <> bool strategy_improvement<strix_aut::ENV_PLAYER>(){
        bool change = false;

        for (strix_aut::node_id_t i = 0; i < n_env_nodes; i++) {
            const dist_id_t k = i * n_colors;
            if (arena.getEnvWinner(i) == strix_aut::Player::UNKNOWN_PLAYER && env_distances[k] > DISTANCE_MINUS_INFINITY) {
                for (strix_aut::edge_id_t j = arena.getEnvSuccsBegin(i); j != arena.getEnvSuccsEnd(i); j++) {
                    const strix_aut::node_id_t successor = arena.getEnvEdge(j);
                    if (arena.getSysWinner(successor) != strix_aut::SYS_PLAYER) {
                        bool improvement = false;
                        dist_id_t m = successor * n_colors;
                        if (sys_distances[m] == DISTANCE_MINUS_INFINITY) {
                            improvement = true;
                        }
                        else {
                            for (dist_id_t l = k; l < k + n_colors; l++, m++) {
                                const distance_t d = env_distances[l];
                                const distance_t d_succ = sys_distances[m];
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
                            env_successors[i] = j;
                            break;
                        }
                    }
                }
            }
        }
        return change;
    } 

    void print_debug(const std::string& str) const {
        if (verbosity >= 5) {
            std::cout << str << std::endl;
        }
    }
    void print_values_debug(){
        if (verbosity >= 6) {
            std::cout << std::setfill('-') << std::setw(20) << "" << std::setfill(' ') << std::endl;
            std::cout << "---- sys nodes -----" << std::endl;
            std::cout << std::setfill('-') << std::setw(20) << "" << std::setfill(' ') << std::endl;
            std::cout << "        v     o(v)    d(v)" << std::endl;
            for (size_t i = 0; i < n_sys_nodes; i++) {
                if (i == arena.get_initial_node()) {
                    std::cout << ">";
                }
                else {
                    std::cout << " ";
                }
                std::cout << std::setw(8) << i;

                std::cout << " [";
                for (strix_aut::edge_id_t j = arena.getSysSuccsBegin(i); j != arena.getSysSuccsEnd(i); j++) {
                    if (sys_successors[j]) {
                        strix_aut::node_id_t s = arena.getSysEdge(j).successor;
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

                for (strix_aut::color_t c = 0; c < n_colors; c++) {
                    distance_t distance = sys_distances[i*n_colors + c];
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
                if (arena.getSysWinner(i) == strix_aut::SYS_PLAYER) {
                    std::cout << "  won_sys";
                }
                else if (arena.getSysWinner(i) == strix_aut::ENV_PLAYER) {
                    std::cout << "  won_env";
                }
                std::cout << std::endl;
            }
            std::cout << std::setfill('-') << std::setw(20) << "" << std::setfill(' ') << std::endl;
            std::cout << "---- env nodes -----" << std::endl;
            std::cout << std::setfill('-') << std::setw(20) << "" << std::setfill(' ') << std::endl;
            std::cout << "        v     d(v)" << std::endl;
            for (size_t i = 0; i < n_env_nodes; i++) {
                if (i == arena.get_initial_node()) {
                    std::cout << ">";
                }
                else {
                    std::cout << " ";
                }
                std::cout << std::setw(8) << i;

                std::cout << " [";
                if (env_successors[i] == strix_aut::EDGE_BOTTOM) {
                    std::cout << "⊥";
                }
                else {
                    std::cout << arena.getEnvEdge(env_successors[i]);
                }
                std::cout << "]";

                for (strix_aut::color_t c = 0; c < n_colors; c++) {
                    distance_t distance = env_distances[i*n_colors + c];
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
                if (arena.getEnvWinner(i) == strix_aut::SYS_PLAYER) {
                    std::cout << "  won_sys";
                }
                else if (arena.getEnvWinner(i) == strix_aut::ENV_PLAYER) {
                    std::cout << "  won_env";
                }
                std::cout << std::endl;
            }
            std::cout << std::setfill('-') << std::setw(20) << "" << std::setfill(' ') << std::endl;
        }
    }


protected:
    void solve_game(){
        sys_distances = std::vector<distance_t>(n_sys_nodes * n_colors, 0);
        env_distances = std::vector<distance_t>(n_env_nodes * n_colors, 0);

        sys_successors.resize(n_sys_edges, false);
        env_successors.resize(n_env_nodes, strix_aut::EDGE_BOTTOM);

        print_debug("Starting strategy iteration for sys player…");
        strategy_iteration<strix_aut::SYS_PLAYER>();

        print_debug("Starting strategy iteration for env player…");
        strategy_iteration<strix_aut::ENV_PLAYER>();

        // clear memory
        std::vector<distance_t>().swap(sys_distances);
        std::vector<distance_t>().swap(env_distances);
    }

public:
    PGSISolver(PGame& arena):PGSolver(arena){}
    ~PGSISolver(){}
};

