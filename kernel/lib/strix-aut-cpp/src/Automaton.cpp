#include "StrixAutomaton.h"

#include <iostream>
#include <algorithm>

namespace strix_aut {

    Automaton::Automaton(owl::Automaton _automaton) :
        automaton(std::move(_automaton)),
        node_type(initNodeType()),
        max_color(initMaxColor()),
        default_color(initDefaultColor()),
        parity_type(initParityType()),
        alphabet_size(0),
        max_number_successors(0),
        has_lock(false),
        complete(false)
    {
        // reserve some space in the vector to avoid initial resizing, which needs locking for the parallel construction
        successors.reserve(4096);
    }

    void Automaton::setAlphabetSize(const letter_t _alphabet_size) {
        alphabet_size = _alphabet_size;
        max_number_successors = ((letter_t)1 << alphabet_size);
    }

    color_t Automaton::initMaxColor() const {
        switch (automaton.acceptance()) {
            case owl::PARITY_MIN_EVEN:
            case owl::PARITY_MIN_ODD:
                return automaton.acceptance_set_count() - 1;
            default:
                return 1;
        }
    }

    color_t Automaton::initDefaultColor() const {
        switch (automaton.acceptance()) {
            case owl::CO_SAFETY:
                return 1;
            case owl::SAFETY:
                return 0;
            default:
                return 0;
        }
    }

    NodeType Automaton::initNodeType() const {
        switch (automaton.acceptance()) {
            case owl::PARITY_MIN_EVEN:
            case owl::PARITY_MIN_ODD:
                return NodeType::PARITY;
            case owl::BUCHI:
                return NodeType::BUCHI;
            case owl::CO_BUCHI:
                return NodeType::CO_BUCHI;
            case owl::SAFETY:
                return NodeType::WEAK;
            case owl::CO_SAFETY:
                return NodeType::WEAK;
            default:
                throw std::invalid_argument("unsupported acceptance");
        }
    }

    Parity Automaton::initParityType() const {
        switch (automaton.acceptance()) {
            case owl::PARITY_MIN_EVEN:
            case owl::BUCHI:
            case owl::SAFETY:
            case owl::CO_SAFETY:
                return Parity::EVEN;
            case owl::PARITY_MIN_ODD:
            case owl::CO_BUCHI:
                return Parity::ODD;
            default:
                throw std::invalid_argument("unsupported acceptance");
        }
    }

    color_t Automaton::getMaxColor() const {
        return max_color;
    }

    NodeType Automaton::getNodeType() const {
        return node_type;
    }

    Parity Automaton::getParityType() const {
        return parity_type;
    }

    void Automaton::add_new_query(node_id_t query) {
        std::unique_lock<std::mutex> lock(query_mutex);
        queries.push(query);
        query_mutex.unlock();
        change.notify_all();
    }

    void Automaton::mark_complete() {
        complete = true;
        change.notify_all();
    }

    void Automaton::wait_for_query(node_id_t& query) {
        std::unique_lock<std::mutex> lock(query_mutex);
        while(!complete && queries.empty()) {
            change.wait(lock);
        }
        if (!queries.empty()) {
            query = queries.front();
            queries.pop();
        }
    }

    void Automaton::add_new_states() {
        while (!complete) {
            node_id_t query = NODE_BOTTOM;
            wait_for_query(query);
            if (query != NODE_BOTTOM) {
                add_successors(query);
            }
        }
    }

    void Automaton::add_successors(node_id_t local_state) {
        if (local_state >= successors.size()) {
            /*
            if (successors.capacity() >= local_state + 1) {
                successors_mutex.lock();
                has_lock = true;
            }
            */
            successors.resize(local_state + 1);
            /*
            if (has_lock) {
                successors_mutex.unlock();
                has_lock = false;
            }
            */
        }

        std::vector<int32_t>& tree = successors[local_state].tree;
        std::vector<ScoredEdge>& leaves = successors[local_state].leaves;
        if (leaves.empty()) {
            owl::EdgeTree edge_tree = automaton.edges(local_state);
            const size_t offset = edge_tree.tree[0];
            const size_t edge_tree_size = edge_tree.tree.size();
            const size_t tree_size = offset - 1;
            const size_t leaves_size = (edge_tree_size - offset) / 2;

            tree.resize(tree_size);
            for (size_t i = 0, j = 1; i < tree_size; i += 3, j += 3) {
                tree[i] = edge_tree.tree[j];
                tree[i + 1] = edge_tree.tree[j + 1];
                if (tree[i + 1] <= 0) {
                    tree[i + 1] /= 2;
                }
                else {
                    tree[i + 1] -= 1;
                }
                tree[i + 2] = edge_tree.tree[j + 2];
                if (tree[i + 2] <= 0) {
                    tree[i + 2] /= 2;
                }
                else {
                    tree[i + 2] -= 1;
                }
            }

            leaves.reserve(leaves_size);
            for (size_t j = offset; j < edge_tree_size; j += 2) {
                node_id_t successor_state;
                int32_t state = edge_tree.tree[j];
                int32_t color = edge_tree.tree[j + 1];
                double score;
                if (state == -1) {
                    successor_state = NODE_BOTTOM;
                    color = 1 - parity_type;
                    score = 0.0;
                }
                else if (state == -2) {
                    successor_state = NODE_TOP;
                    color = parity_type;
                    score = 1.0;
                }
                else {
                    successor_state = state;
                    score = automaton.quality_score(state, color);
                    if (node_type == NodeType::WEAK) {
                        color = default_color;
                    }
                    else {
                        // shift score so values 0.0 and 1.0 are excluded
                        score = 0.5*score + 0.25;
                        if (node_type == NodeType::BUCHI || node_type == NodeType::CO_BUCHI) {
                            if (color == -1) {
                                color = 1;
                            }
                            else {
                                color = 0;
                            }
                        }
                        else if (color == -1) {
                            color = max_color;
                        }
                    }
                }
                leaves.push_back(ScoredEdge(successor_state, color, score, 1.0));
            }

            // flatten tree for small alphabets
            if (alphabet_size <= 4) {
                successors[local_state].flatten_tree(max_number_successors);
            }

            //new_successors.notify_all();
        }
    }

    ScoredEdge Automaton::getSuccessor(node_id_t local_state, letter_t letter) {
        add_successors(local_state);
        /*
        std::unique_lock<std::mutex> lock(successors_mutex);
        while(local_state >= successors.size() || successors[local_state].empty()) {
            new_successors.wait(lock);
        }
        */
        return successors[local_state].lookup(letter);
    }

    void Automaton::print_type() const {
        switch (node_type) {
            case NodeType::WEAK:
                if (default_color == 0) {
                    std::cout << "Safety";
                }
                else {
                    std::cout << "co-Safety";
                }
                break;
            case NodeType::BUCHI:
                std::cout << "Büchi";
                break;
            case NodeType::CO_BUCHI:
                std::cout << "co-Büchi";
                break;
            case NodeType::PARITY:
                std::cout << "Parity (" << getMaxColor() << ")";
                break;
        }
    }

    void Automaton::print_memory_usage() const {
        size_t cache_size = 0;
        for (const auto& succ : successors) {
            cache_size += succ.tree.size() * sizeof(int32_t);
            cache_size += succ.leaves.size() * sizeof(ScoredEdge);
            cache_size += succ.direct.size() * sizeof(ScoredEdge);
        }
        std::cout << "Automaton successors: " << (cache_size / 1024) << std::endl;
    }

}
