#pragma once

#include <iostream>
#include <iomanip>
#include <sstream>

#include "pgame.h"

// some types
typedef uint32_t state_id_t;
constexpr state_id_t TOP_STATE = std::numeric_limits<state_id_t>::max();
constexpr state_id_t NONE_STATE = std::numeric_limits<state_id_t>::max() - 1;
constexpr symbolic_t ANY_INPUT = std::numeric_limits<symbolic_t>::max();
constexpr symbolic_t ANY_OUTPUT = std::numeric_limits<symbolic_t>::max();

//array index
template <typename I>
inline I ai(I x, I y, I ySize) {
    return x*ySize + y;
}


// count the number of set bits in a number
template <typename I>
inline
typename std::enable_if<std::is_integral<I>::value, unsigned >::type
popcount(I number) {
    static_assert(std::numeric_limits<I>::radix == 2, "non-binary type");

    constexpr int bitwidth = std::numeric_limits<I>::digits + std::numeric_limits<I>::is_signed;
    static_assert(bitwidth <= std::numeric_limits<unsigned long long>::digits, "arg too wide for std::bitset() constructor");

    typedef typename std::make_unsigned<I>::type UI;

    std::bitset<bitwidth> bs( static_cast<UI>(number) );
    return bs.count();
}

// a machine transition
struct Transition {
    state_id_t nextState;
    const symbolic_t input;
    const std::vector<symbolic_t> output;

    Transition(
            const state_id_t nextState,
            const symbolic_t input,
            const std::vector<symbolic_t> output
    ) : nextState(nextState), input(input), output(std::move(output)) {}
};

// the machine is a set of transition for each state
typedef std::vector<std::vector<Transition>> machine_t;

// a machine can be mealy or moore
enum class Semantic { MEALY, MOORE };

// a successor to hold the successor state and output
struct Successor {
    state_id_t successor;
    std::vector<symbolic_t> output;

    Successor() : successor(0), output(0) {}
    Successor(const state_id_t successor, const std::vector<symbolic_t> output) :
        successor(successor), output(std::move(output)) {}

    bool operator==(const Successor &other) const {
        return (successor == other.successor && output == other.output);
    }
    bool operator<(const Successor& other) const {
        return std::tie(successor, output) < std::tie(other.successor, other.output);
    }
};


// the mealy/moore machine
class MealyMachine {
public:
    const std::vector<std::string> inputs;
    const std::vector<std::string> outputs;
    const size_t n_inputs;
    const size_t n_outputs;
    const Semantic semantic;

private:
    machine_t machine;

    std::vector<symbolic_t> state_labels;
    std::vector<int> state_label_accumulated_bits;
    int state_label_bits;

public:
    MealyMachine(const std::vector<std::string> inputs_, const std::vector<std::string> outputs_, const Semantic semantic) :
        inputs(std::move(inputs_)),
        outputs(std::move(outputs_)),
        n_inputs(inputs.size()),
        n_outputs(outputs.size()),
        semantic(semantic),
        state_label_bits(-1)
    { }
    ~MealyMachine() { }

    void setMachine(machine_t new_machine) {
        machine = std::move(new_machine);
    }

    void setStateLabels(std::vector<symbolic_t> labels, const int bits, std::vector<int> accumulated_bits){
        state_label_bits = bits;
        state_labels = std::move(labels);
        state_label_accumulated_bits = std::move(accumulated_bits);
    }

    symbolic_t getStateLabel(const state_id_t s) const{
        return state_labels[s];
    }

    state_id_t numberOfStates() const{
        return machine.size();
    }

    int getStateLabelBits() const{
        return state_label_bits;
    }

    bool hasLabels() const{
        return state_label_bits >= 0;
    }

    const std::vector<std::vector<Transition>>& getTransitions() const{
        return machine;
    }

    void construct(PGame& arena, PGSolver& solver){
        if(semantic == Semantic::MEALY)
            constructMealyMachine(arena, solver);
        else
            std::runtime_error("Not yet implemented !");
    }

private:
    void constructMealyMachine(PGame& arena, PGSolver& solver){
        machine_t machine;

        const symbolic_t any_input = ANY_INPUT;
        const symbolic_t any_output = ANY_OUTPUT;
        std::vector<strix_aut::node_id_t> state_map(arena.get_n_env_nodes(), strix_aut::NODE_NONE);

        std::deque<strix_aut::node_id_t> queue;
        queue.push_back(arena.get_initial_node());
        machine.push_back({});

        state_map[arena.get_initial_node()] = 0;

        while (!queue.empty()) {
            strix_aut::node_id_t env_node = queue.front();
            queue.pop_front();

            state_id_t state = state_map[env_node];

            std::map< Successor, std::vector<symbolic_t>> input_list;

            for (strix_aut::edge_id_t env_edge = arena.getEnvSuccsBegin(env_node); env_edge != arena.getEnvSuccsEnd(env_node); env_edge++) {
                const strix_aut::node_id_t sys_node = arena.getEnvEdge(env_edge);
                std::map<strix_aut::node_id_t, std::vector<symbolic_t>> successor_list;

                for (strix_aut::edge_id_t sys_edge = arena.getSysSuccsBegin(sys_node); sys_edge != arena.getSysSuccsEnd(sys_node); sys_edge++) {
                    if (solver.get_sys_successor(sys_edge)) {
                        const Edge edge = arena.getSysEdge(sys_edge);
                        std::vector<symbolic_t> sys_outputs = arena.getSysOutput(sys_edge);

                        auto const result = successor_list.insert({ edge.successor, sys_outputs });
                        if (!result.second) {
                            for (auto item : sys_outputs)
                                result.first->second.push_back(item);    
                        }
                    }
                }

                // Heuristic: if possible, choose successor that was already explored,
                // and among those, choose successor with with maximum non-determinism

                auto succ_it = successor_list.begin();
                bool succ_explored = false;
                size_t succ_num_outputs = 0;

                for (auto it = successor_list.begin(); it != successor_list.end(); it++) {
                    const strix_aut::node_id_t cur_successor = it->first;
                    if (cur_successor == strix_aut::NODE_TOP) {
                        // always choose top node
                        succ_it = it;
                        break;
                    }
                    else {
                        bool cur_succ_explored = (state_map[cur_successor] != strix_aut::NODE_NONE);
                        size_t cur_succ_num_outputs = it->second.size();
                        if (
                                (cur_succ_explored && !succ_explored) ||
                                (cur_succ_explored == succ_explored && cur_succ_num_outputs > succ_num_outputs)
                        ) {
                            succ_it = it;
                            succ_num_outputs = cur_succ_num_outputs;
                            succ_explored = cur_succ_explored;
                        }
                    }
                }

                const strix_aut::node_id_t successor = succ_it->first;

                state_id_t mealy_successor;
                if (successor == strix_aut::NODE_TOP) {
                    mealy_successor = TOP_STATE;
                }
                else {
                    if (state_map[successor] == strix_aut::NODE_NONE) {
                        state_id_t successor_state = machine.size();
                        machine.push_back({});
                        state_map[successor] = successor_state;
                        queue.push_back(successor);
                    }
                    mealy_successor = state_map[successor];
                }

                // add outputs to state
                std::vector<symbolic_t> sys_outputs = succ_it->second;

                if (mealy_successor != TOP_STATE || sys_outputs[0] != any_output) {
                    std::vector<symbolic_t> env_inputs = arena.getEnvInput(env_edge);

                    Successor mealy_combined_successor(mealy_successor, sys_outputs);
                    auto const result = input_list.insert({ mealy_combined_successor, env_inputs });
                    if (!result.second) {
                        for(auto item : env_inputs)
                            result.first->second.push_back(item);    
                    }
                }
            }

            for (const auto& entry : input_list) {
                for (auto inp : entry.second) {
                    machine[state].push_back( Transition(entry.first.successor, inp, entry.first.output) );
                }
            }
        }

        bool has_top_state = false;
        strix_aut::node_id_t top_state = machine.size();
        for (state_id_t s = 0; s < machine.size(); s++) {
            for (auto& t : machine[s]) {
                if (t.nextState == TOP_STATE) {
                    has_top_state = true;
                    t.nextState = top_state;
                }
            }
        }
        if (has_top_state)
            machine.push_back({ Transition(top_state, any_input, {any_output}) });
        else
            top_state = NONE_STATE;

        setMachine(std::move(machine));
    }    
};