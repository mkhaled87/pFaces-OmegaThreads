#pragma once

#include <iostream>
#include <iomanip>
#include <sstream>

#include "pgame.h"

// some types
typedef uint32_t state_id_t;
constexpr state_id_t TOP_STATE = std::numeric_limits<state_id_t>::max();
constexpr state_id_t NONE_STATE = std::numeric_limits<state_id_t>::max() - 1;

//array index
template <typename I>
inline I ai(I x, I y, I ySize) {
    return x*ySize + y;
}

// all bits set
template <typename I>
SpecSeq<I> true_clause(const int numBits) {
    return SpecSeq<I>(0, ((I)1 << numBits) - 1);
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
    const SpecSeq<strix_aut::letter_t> input;
    const std::vector<SpecSeq<strix_aut::letter_t>> output;

    Transition(
            const state_id_t nextState,
            const SpecSeq<strix_aut::letter_t> input,
            const std::vector<SpecSeq<strix_aut::letter_t>> output
    ) : nextState(nextState), input(input), output(std::move(output)) {}
};

// the machine is a set of transition for each state
typedef std::vector<std::vector<Transition>> machine_t;

// a machine can be mealy or moore
enum class Semantic { MEALY, MOORE };

// a successor to hold the successor state and output
struct Successor {
    state_id_t successor;
    std::vector<SpecSeq<strix_aut::letter_t>> output;

    Successor() : successor(0), output(0) {}
    Successor(const state_id_t successor, const std::vector<SpecSeq<strix_aut::letter_t>> output) :
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

    std::vector<SpecSeq<strix_aut::node_id_t>> state_labels;
    std::vector<int> state_label_accumulated_bits;
    int state_label_bits;

    //std::vector<state_id_t> min_map;

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

    void setStateLabels(std::vector<SpecSeq<strix_aut::node_id_t>> labels, const int bits, std::vector<int> accumulated_bits){
        state_label_bits = bits;
        state_labels = std::move(labels);
        state_label_accumulated_bits = std::move(accumulated_bits);
    }

    SpecSeq<strix_aut::node_id_t> getStateLabel(const state_id_t s) const{
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

    void print_dot(std::ostream& out) const{
        const bool use_labels = hasLabels();

        const std::vector<std::vector<Transition>>& kiss_machine = getTransitions();

        out << "digraph \"\" {" << std::endl;
        out << "graph [rankdir=LR,ranksep=0.8,nodesep=0.2];" << std::endl;
        out << "node [shape=circle];" << std::endl;
        out << "edge [fontname=mono];" << std::endl;
        out << "init [shape=point,style=invis];" << std::endl;

        for (state_id_t s = 0; s < kiss_machine.size(); s++) {
            out << s << " [label=\"" << s << "\"";
            if (use_labels) {
                out << ",tooltip=\"" << getStateLabel(s).toStateLabelledString(state_label_accumulated_bits) << "\"";
            }
            out << "];" << std::endl;
        }

        out << "init -> 0 [penwidth=0,tooltip=\"initial state\"];" << std::endl;

        for (state_id_t s = 0; s < kiss_machine.size(); s++) {
            // first collect transitions to the same state
            std::map<state_id_t, std::vector<std::pair<SpecSeq<strix_aut::letter_t>, std::vector<SpecSeq<strix_aut::letter_t>> > > > stateTransitions;
            for (const auto& t : kiss_machine[s]) {
                stateTransitions[t.nextState].push_back({ t.input, t.output });
            }

            for (const auto& t : stateTransitions) {
                out << s << " -> " << t.first << " [label=\"";
                for (const auto& l : t.second) {
                    out << l.first.toString(n_inputs) << "/";
                    bool first = true;
                    for (const auto& output : l.second) {
                        if (!first) {
                            out << "+";
                        }
                        first = false;
                        out << output.toString(n_outputs);
                    }
                    out << "\\l";
                }
                out << "\",labeltooltip=\"";
                for (const auto& l : t.second) {
                    out << l.first.toLabelledString(n_inputs, inputs) << "/";
                    bool first = true;
                    for (const auto& output : l.second) {
                        if (!first) {
                            out << "&#8744;";
                        }
                        first = false;
                        out << output.toLabelledString(n_outputs, outputs);
                    }
                    out << "&#10;";
                }
                out << "\"];" << std::endl;
            }
        }
        out << "}" << std::endl;
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

        const SpecSeq<strix_aut::letter_t> any_input = true_clause<strix_aut::letter_t>(arena.n_inputs);
        const SpecSeq<strix_aut::letter_t> any_output = true_clause<strix_aut::letter_t>(arena.n_outputs);
        std::vector<strix_aut::node_id_t> state_map(arena.get_n_env_nodes(), strix_aut::NODE_NONE);

        std::deque<strix_aut::node_id_t> queue;
        queue.push_back(arena.get_initial_node());
        machine.push_back({});

        state_map[arena.get_initial_node()] = 0;

        while (!queue.empty()) {
            strix_aut::node_id_t env_node = queue.front();
            queue.pop_front();

            state_id_t state = state_map[env_node];

            std::map< Successor, BDD> input_list;

            for (strix_aut::edge_id_t env_edge = arena.getEnvSuccsBegin(env_node); env_edge != arena.getEnvSuccsEnd(env_node); env_edge++) {
                const strix_aut::node_id_t sys_node = arena.getEnvEdge(env_edge);
                std::map<strix_aut::node_id_t, BDD> successor_list;

                for (strix_aut::edge_id_t sys_edge = arena.getSysSuccsBegin(sys_node); sys_edge != arena.getSysSuccsEnd(sys_node); sys_edge++) {
                    if (solver.get_sys_successor(sys_edge)) {
                        const Edge edge = arena.getSysEdge(sys_edge);
                        BDD output_bdd = arena.getSysOutput(sys_edge);

                        auto const result = successor_list.insert({ edge.successor, output_bdd });
                        if (!result.second) {
                            result.first->second |= output_bdd;
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
                        size_t cur_succ_num_outputs = it->second.CountMinterm(arena.n_outputs);
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
                BDD output_bdd = succ_it->second;
                std::vector<SpecSeq<strix_aut::letter_t>> outputs;
                for (BDDtoSpecSeqIterator<strix_aut::letter_t> out_it(output_bdd, arena.n_outputs); !out_it.done(); out_it++) {
                    // add constant outputs
                    SpecSeq<strix_aut::letter_t> output_letter = arena.addRealizableOutputMask(*out_it);
                    outputs.push_back(output_letter);
                }
                // sort outputs by number of unspecified bits and number of zeros
                if (outputs.size() > 1) {
                    std::sort(std::begin(outputs), std::end(outputs),
                        [] (const auto& lhs, const auto& rhs) {
                            int lhs_c1 = popcount(lhs.number);
                            int rhs_c1 = popcount(rhs.number);
                            int lhs_c2 = popcount(lhs.unspecifiedBits);
                            int rhs_c2 = popcount(rhs.unspecifiedBits);
                            return lhs_c1 < rhs_c1 ||
                                (lhs_c1 == rhs_c1 && (lhs_c2 > rhs_c2 ||
                                (lhs_c2 == rhs_c2 && lhs.number < rhs.number)));
                    });
                }

                if (mealy_successor != TOP_STATE || outputs[0] != any_output) {
                    // add to bdd
                    BDD input_bdd = arena.getEnvInput(env_edge);

                    Successor mealy_combined_successor(mealy_successor, outputs);
                    auto const result = input_list.insert({ mealy_combined_successor, input_bdd });
                    if (!result.second) {
                        result.first->second |= input_bdd;
                    }
                }
            }

            for (const auto& entry : input_list) {
                for (BDDtoSpecSeqIterator<strix_aut::letter_t> in_it(entry.second, arena.n_inputs); !in_it.done(); in_it++) {
                    machine[state].push_back( Transition(entry.first.successor, *in_it, entry.first.output) );
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
        if (has_top_state) {
            SpecSeq<strix_aut::letter_t> top_output = arena.addRealizableOutputMask(any_output);
            machine.push_back({ Transition(top_state, any_input, {top_output}) });
        }
        else {
            top_state = NONE_STATE;
        }

        setMachine(std::move(machine));
    }    
};