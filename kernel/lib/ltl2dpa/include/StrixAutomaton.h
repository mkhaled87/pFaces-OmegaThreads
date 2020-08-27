/**
 * @file StrixAutomaton.h
 *
 * @brief I adapteed this form STRIX/aut (https://strix.model.in.tum.de).
 * I utilize its construction of the structure which interfaces OWL in a
 * good way. I made the following major modifications:
 *  1- Combined all the header files here and used those necessary items from Definitios.h
 *  2- Changed the name of class "ParityAutomatonTreeItem" to "ParityAutomatonTreeItem" which
 *     i though would make more sense.
 *  3- corrected some minor things mainly warnings (e.g., unnecessary copy constructor that is defaulted)
 * @author M. Khaled
 *
 */
#pragma once

#include <queue>
#include <atomic>
#include <mutex>
#include <thread>
#include <cstdint>
#include <vector>
#include <limits>
#include <iostream>
#include <tuple>
#include <set>
#include <condition_variable>

#include "cuddObj.hh"
#include "StrixOwl.h"


namespace strix_aut {

    // some types/constants
    typedef uint32_t node_id_t;
    typedef uint32_t edge_id_t;
    typedef uint32_t color_t;
    typedef uint64_t letter_t;
    constexpr letter_t MIN_LETTER = std::numeric_limits<letter_t>::min();
    constexpr letter_t MAX_LETTER = std::numeric_limits<letter_t>::max();
    constexpr node_id_t NODE_BOTTOM = std::numeric_limits<node_id_t>::max() - 1;
    constexpr node_id_t NODE_TOP = std::numeric_limits<node_id_t>::max() - 2;
    constexpr node_id_t NODE_NONE = std::numeric_limits<node_id_t>::max() - 3;
    constexpr node_id_t NODE_NONE_BOTTOM = std::numeric_limits<node_id_t>::max() - 4;
    constexpr node_id_t NODE_NONE_TOP = std::numeric_limits<node_id_t>::max() - 5;
    constexpr edge_id_t EDGE_BOTTOM = std::numeric_limits<edge_id_t>::max();    

    // Parity and Player definitions
    enum Parity { 
        EVEN = 0, 
        ODD = 1 
    };
    enum class ParityType : uint8_t {
        MAX = 0,
        MIN = 1,
    };    
    enum Player : int8_t {
        SYS_PLAYER = 1,
        ENV_PLAYER = -1,
        UNKNOWN_PLAYER = 0,
    };

    // the product state represents a combined state over all automaton in
    // the leaves of the tree structure
    typedef std::vector<node_id_t> product_state_t;
    typedef product_state_t::const_iterator state_iter;


    // a node in the tree can represnet an Automaton which can
    // be of different types 
    enum class NodeType : uint8_t {
        WEAK = 0,
        BUCHI = 1,
        CO_BUCHI = 2,
        PARITY = 3
    };
    inline NodeType join_node_type(const NodeType a, const NodeType b) {
        return (NodeType)(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
    }
    inline NodeType join_node_type_biconditional(const NodeType a, const NodeType b) {
        if (join_node_type(a, b) == NodeType::WEAK) {
            return NodeType::WEAK;
        }
        else {
            return NodeType::PARITY;
        }
    }

    // Color/Score/Weigh to be used on edges
    struct ColorScore {
        color_t color;
        double score;
        double weight;

        ColorScore() :
            color(0), score(0.0), weight(1.0)
        {};
        ColorScore(const color_t color, const double score, const double weight) :
            color(color), score(score), weight(weight)
        {};
    };

    // an edge between two nodes with a ColorScore on it
    struct ScoredEdge {
        node_id_t successor;
        ColorScore cs;

        ScoredEdge() :
            successor(NODE_NONE), cs()
        {};
        ScoredEdge(const node_id_t successor, const color_t color, const double score, const double weight) :
            successor(successor), cs(ColorScore(color, score, weight))
        {};
    };

    // a global method to convert owl objects to string
    std::string owl_object_to_string(JVM owl, void* object);

    // An Automaton located in a leaf of the tree structure
    class Automaton {
        private:
            struct SuccessorCache {
                std::vector<int32_t> tree;
                std::vector<ScoredEdge> leaves;
                std::vector<ScoredEdge> direct;

                inline ScoredEdge tree_lookup(const letter_t letter) const {
                    int32_t i;
                    if (!tree.empty()) {
                        i = 0;
                        while (i >= 0) {
                            if ((letter & ((letter_t)1 << tree[i])) == 0) {
                                i = tree[i + 1];
                            } else {
                                i = tree[i + 2];
                            }
                        }
                    }
                    else {
                        i = -1;
                    }
                    return leaves[-i-1];
                }

                inline ScoredEdge direct_lookup(const letter_t letter) const {
                    return direct[letter];
                }

                inline ScoredEdge lookup(const letter_t letter) const {
                    if (direct.empty()) {
                        return tree_lookup(letter);
                    }
                    else {
                        return direct_lookup(letter);
                    }
                }

                void flatten_tree(const letter_t max_letter) {
                    direct.reserve(max_letter);
                    for (letter_t letter = 0; letter < max_letter; letter++) {
                        direct.push_back(tree_lookup(letter));
                    }
                }

                BDD toBDD(Cudd manager, std::vector<int>& bdd_mapping, int32_t pos) {
                    if (pos < 0) {
                        node_id_t succ = leaves[-pos-1].successor;
                        if (succ == NODE_BOTTOM) {
                            return manager.bddZero();
                        }
                        else {
                            return manager.bddOne();
                        }
                    }
                    else {
                        int32_t var = tree[pos];
                        int32_t falseChild = tree[pos + 1];
                        int32_t trueChild = tree[pos + 2];
                        BDD bdd_var = manager.bddVar(bdd_mapping[var]);
                        BDD falseBDD = toBDD(manager, bdd_mapping, falseChild);
                        BDD trueBDD = toBDD(manager, bdd_mapping, trueChild);
                        return bdd_var.Ite(trueBDD, falseBDD);
                    }
                }

                BDD toBDD(Cudd manager, std::vector<int>& bdd_mapping) {
                    if (tree.empty()) {
                        return toBDD(manager, bdd_mapping, -1);
                    }
                    else {
                        return toBDD(manager, bdd_mapping, 0);
                    }
                }
            };

            Automaton(const Automaton&) = delete;

            Isolate isolate;
            JVM owl;
            OwlAutomaton automaton;
            const NodeType node_type;
            const color_t max_color;
            const color_t default_color;
            const Parity parity;
            const ParityType parity_type;
            bool has_safety_filter;
            std::vector< SuccessorCache > successors;
            letter_t alphabet_size;
            letter_t max_number_successors;
            sized_int_array_t owl_nodes;
            sized_int_array_t owl_leaves;
            sized_double_array_t owl_scores;

            // queue for querying for successors
            std::queue<node_id_t> queries;

            // mutex for accessing the queue for queries
            std::mutex query_mutex;

            // mutex for accessing the successors
            std::mutex successors_mutex;

            // flag if the consumer thread currently has the lock on successors
            bool has_lock;

            // condition variable signalling new queries or complete construction
            std::condition_variable change;

            // condition variable signalling new successors
            std::condition_variable new_successors;

            // variable signalling that the automaton is completely constructed
            std::atomic<bool> complete;

            void add_new_query(node_id_t query);
            void wait_for_query(node_id_t& query);
            void add_new_states();
            void add_successors(node_id_t local_state);

            color_t initMaxColor() const;
            color_t initDefaultColor() const;
            NodeType initNodeType() const;
            Parity initParity() const;
            ParityType initParityType() const;
            bool initSafetyFilter() const;

        public:
            Automaton(Isolate isolate, JVM owl, OwlAutomaton automaton);
            ~Automaton();

            void setAlphabetSize(const letter_t alphabet_size);
            letter_t getAlphabetSize() const;

            ScoredEdge getSuccessor(node_id_t local_state, letter_t letter);

            bool hasSafetyFilter();
            BDD getSafetyFilter(Cudd manager, std::vector<int>& bdd_mapping);

            color_t getMaxColor() const;
            NodeType getNodeType() const;
            Parity getParity() const;

            void print_type() const;
            void print_memory_usage() const;
    };

    // An item in the tree can be (later) node or a leaf in the tree structure.
    // Nodes represnet operation of underlying specification formula
    // leafs hold the formulae.
    class ParityAutomatonTreeItem {
        protected:
            size_t state_index;
            size_t state_width;

            ParityAutomatonTreeItem(const NodeType node_type, const Parity parity, const color_t max_color) :
                state_index(0), node_type(node_type), parity(parity), max_color(max_color)
            {}

        public:
            virtual ~ParityAutomatonTreeItem() {}

            const NodeType node_type;
            const Parity parity;
            const color_t max_color;

            virtual void getInitialState(product_state_t& state) = 0;
            virtual ColorScore getSuccessor(const product_state_t& state, product_state_t& new_state, letter_t letter) = 0;

            virtual BDD computeSafetyFilter(Cudd manager, size_t var_begin, size_t var_end, int depth) = 0;

            virtual void setState(product_state_t& new_state, node_id_t state) = 0;
            virtual void setTopState(product_state_t& new_state) = 0;
            virtual void setBottomState(product_state_t& new_state) = 0;

            virtual bool isTopState(const product_state_t& state) const = 0;
            virtual bool isBottomState(const product_state_t& state) const = 0;

            inline size_t getStateIndex() const { return state_index; }

            virtual int getMinIndex() const = 0;
            virtual letter_t getMaximumAlphabetSize() const = 0;
            virtual std::set<letter_t> getAlphabet() const = 0;

            inline void print_type() const {
                switch(node_type) {
                    case NodeType::PARITY:
                        std::cout << "Parity " << ((parity == Parity::EVEN) ? "even" : "odd");
                        break;
                    case NodeType::BUCHI:    std::cout << "Büchi";    break;
                    case NodeType::CO_BUCHI: std::cout << "co-Büchi"; break;
                    case NodeType::WEAK:     std::cout << "Weak";     break;
                }
            }
            virtual void print(const int verbosity = 0, const int indent = 0) const = 0;
    };


    // A leaf in the tree structure contains an actual Automaton which
    // is obtained from OWL after feeding a formula (LTL) to it.
    class ParityAutomatonTreeLeaf : public ParityAutomatonTreeItem {
        friend class AutomatonTreeStructure;

        private:
            ParityAutomatonTreeLeaf(const ParityAutomatonTreeLeaf&) = delete;

            Automaton& automaton;
            const std::string name;
            const int reference;
            std::vector<std::pair<letter_t,letter_t>> mapping;

        protected:
            ParityAutomatonTreeLeaf(
                Automaton& automaton,
                const std::string name,
                const int reference,
                std::vector<std::pair<letter_t,letter_t>> mapping
            );

        public:
            virtual ~ParityAutomatonTreeLeaf();
            ParityAutomatonTreeLeaf(ParityAutomatonTreeLeaf&&) = default;

            void getInitialState(product_state_t& state) override;
            ColorScore getSuccessor(const product_state_t& state, product_state_t& new_state, letter_t letter) override;

            BDD computeSafetyFilter(Cudd manager, size_t var_begin, size_t var_end, int depth) override;

            void setState(product_state_t& new_state, node_id_t state) override;
            void setTopState(product_state_t& new_state) override;
            void setBottomState(product_state_t& new_state) override;

            bool isTopState(const product_state_t& state) const override;
            bool isBottomState(const product_state_t& state) const override;

            virtual int getMinIndex() const override;
            virtual letter_t getMaximumAlphabetSize() const override;
            virtual std::set<letter_t> getAlphabet() const override;

            void print(const int verbosity = 0, const int indent = 0) const override;
    };

    // A node will represent operation on the next leafs/nodes in the tree
    // the operatin is represented by a tag.
    class ParityAutomatonTreeNode : public ParityAutomatonTreeItem {
        friend class AutomatonTreeStructure;

        protected:
            const node_type_t tag;
            const std::vector<std::unique_ptr<ParityAutomatonTreeItem>> children;
            const bool parity_child;
            const color_t dp;
            const node_id_t round_robin_size;

            ParityAutomatonTreeNode(
                const node_type_t tag, const NodeType node_type, const Parity parity, const color_t max_color,
                const node_id_t round_robin_size, const bool parity_child,
                const color_t dp, std::vector<std::unique_ptr<ParityAutomatonTreeItem>> children
            );

        public:
            virtual ~ParityAutomatonTreeNode();

            virtual void getInitialState(product_state_t& state) override;
            virtual ColorScore getSuccessor(const product_state_t& state, product_state_t& new_state, letter_t letter) override;

            virtual BDD computeSafetyFilter(Cudd manager, size_t var_begin, size_t var_end, int depth) override;

            virtual void setState(product_state_t& new_state, node_id_t state) override;
            virtual void setTopState(product_state_t& new_state) override;
            virtual void setBottomState(product_state_t& new_state) override;

            virtual bool isTopState(const product_state_t& state) const override;
            virtual bool isBottomState(const product_state_t& state) const override;

            virtual int getMinIndex() const override;
            virtual letter_t getMaximumAlphabetSize() const override;
            virtual std::set<letter_t> getAlphabet() const override;

            void print(const int verbosity = 0, const int indent = 0) const override;
    };
    class ParityAutomatonTreeBiconditionalNode : public ParityAutomatonTreeNode {
        friend class AutomatonTreeStructure;

        private:
            const int parity_child_index;
            const color_t d1;
            const color_t d2;

        protected:
            ParityAutomatonTreeBiconditionalNode(
                const NodeType node_type, const Parity parity, const color_t max_color,
                const node_id_t round_robin_size, const bool parity_child, const color_t dp,
                const int parity_child_index,
                std::vector<std::unique_ptr<ParityAutomatonTreeItem>> children
            );

        public:
            virtual ~ParityAutomatonTreeBiconditionalNode();

            void getInitialState(product_state_t& state) override;
            ColorScore getSuccessor(const product_state_t& state, product_state_t& new_state, letter_t letter) override;

            BDD computeSafetyFilter(Cudd manager, size_t var_begin, size_t var_end, int depth) override;

            void setState(product_state_t& new_state, node_id_t state) override;
            void setTopState(product_state_t& new_state) override;
            void setBottomState(product_state_t& new_state) override;

            bool isTopState(const product_state_t& state) const override;
            bool isBottomState(const product_state_t& state) const override;
    };

    // The tree structure to maintain the leaves and provide interface to
    // access the product state and navigate through the product Automaton.
    class AutomatonTreeStructure {
        private:
            AutomatonTreeStructure(const AutomatonTreeStructure&) = delete;
            AutomatonTreeStructure(JVM owl, size_t alphabet_size);

            JVM owl;
            OwlDecomposedDPA owl_automaton;
            size_t alphabet_size;
            std::deque<Automaton> automata;
            std::unique_ptr<ParityAutomatonTreeItem> tree; //(head of the tree ?)
            std::vector<atomic_proposition_status_t> statuses;

            std::unique_ptr<ParityAutomatonTreeItem> constructTree(const OwlStructure tree, std::vector<ParityAutomatonTreeLeaf*>& leaves);
            std::vector<ParityAutomatonTreeLeaf*> leaves;

            std::vector<size_t> leaf_state_indices;
            product_state_t initial_state;

        public:
            static AutomatonTreeStructure fromFormula(Isolate isolate, JVM owl, OwlFormula formula, size_t num_inputs, size_t num_outputs, bool simplify);
            static AutomatonTreeStructure fromAutomaton(Isolate isolate, JVM owl, OwlDecomposedDPA automaton, size_t num_inputs, size_t num_outputs);

            AutomatonTreeStructure(AutomatonTreeStructure&&);
            ~AutomatonTreeStructure();

            Parity getParity() const;
            color_t getMaxColor() const;

            product_state_t getInitialState() const;
            ColorScore getSuccessor(const product_state_t& state, product_state_t& new_state, letter_t letter);

            BDD computeSafetyFilter(Cudd manager, size_t var_begin, size_t var_end);

            std::vector<int> getAutomatonStates(const product_state_t& state) const;
            bool declareWinning(const product_state_t& state, const Player winner);
            Player queryWinner(const product_state_t& state);

            bool isTopState(const product_state_t& state) const;
            bool isBottomState(const product_state_t& state) const;

            std::set<letter_t> getAlphabet() const;
            atomic_proposition_status_t getVariableStatus(int variable) const;

            void print(const int verbosity = 0) const;
            void print_memory_usage() const;
    };
}
