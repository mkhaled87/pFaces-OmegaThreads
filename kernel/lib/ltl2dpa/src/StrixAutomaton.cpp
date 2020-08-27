#include <iostream>
#include <iomanip>
#include <algorithm>
#include <bitset>
#include <cmath>

#include "StrixAutomaton.h"

namespace strix_aut {

    bool cmp_trees(const std::unique_ptr<ParityAutomatonTreeItem>& t1, const std::unique_ptr<ParityAutomatonTreeItem>& t2) {
        if (t1->node_type < t2->node_type) {
            return true;
        }
        else if (t1->node_type > t2->node_type) {
            return false;
        }
        else {
            const letter_t size1 = t1->getMaximumAlphabetSize();
            const letter_t size2 = t2->getMaximumAlphabetSize();
            if (size1 < size2) {
                return true;
            }
            else if (size1 > size2) {
                return false;
            }
            else {
                const auto alphabet1 = t1->getAlphabet();
                const auto alphabet2 = t2->getAlphabet();
                if (alphabet1 < alphabet2) {
                    return true;
                }
                else if (alphabet1 > alphabet2) {
                    return false;
                }
                else {
                    return t1->getMinIndex() < t2->getMinIndex();
                }
            }
        }
    }

    // --------------------------------------------------
    // class: Automaton
    // --------------------------------------------------
    Automaton::Automaton(Isolate isolate, JVM owl, OwlAutomaton _automaton) :
        isolate(isolate),
        owl(owl),
        automaton(std::move(_automaton)),
        node_type(initNodeType()),
        max_color(initMaxColor()),
        default_color(initDefaultColor()),
        parity(initParity()),
        parity_type(initParityType()),
        has_safety_filter(initSafetyFilter()),
        alphabet_size(0),
        max_number_successors(0),
        has_lock(false),
        complete(false)
    {
        // reserve some space in the vector to avoid initial resizing, which needs locking for the parallel construction
        successors.reserve(4096);

        owl_nodes.elements = nullptr;
        owl_leaves.elements = nullptr;
        owl_scores.elements = nullptr;
    }

    Automaton::~Automaton() {
        if (automaton != nullptr) {
            destroy_object_handle(owl, automaton);
            automaton = nullptr;
        }
    }

    void Automaton::setAlphabetSize(const letter_t _alphabet_size) {
        alphabet_size = _alphabet_size;
        max_number_successors = ((letter_t)1 << alphabet_size);
    }

    letter_t Automaton::getAlphabetSize() const {
        return alphabet_size;
    }

    color_t Automaton::initMaxColor() const {
        switch (automaton_acceptance_condition(owl, automaton)) {
            case PARITY_MIN_EVEN:
            case PARITY_MIN_ODD:
                return automaton_acceptance_condition_sets(owl, automaton) - 1;
            case PARITY_MAX_EVEN:
            case PARITY_MAX_ODD: {
                    int c = automaton_acceptance_condition_sets(owl, automaton) - 1;
                    c += c % 2;
                    return c;
                }
            default:
                return 1;
        }
    }

    color_t Automaton::initDefaultColor() const {
        switch (automaton_acceptance_condition(owl, automaton)) {
            case CO_SAFETY:
                return 1;
            case SAFETY:
                return 0;
            default:
                return 0;
        }
    }

    NodeType Automaton::initNodeType() const {
        switch (automaton_acceptance_condition(owl, automaton)) {
            case PARITY_MIN_EVEN:
            case PARITY_MIN_ODD:
            case PARITY_MAX_EVEN:
            case PARITY_MAX_ODD:
                return NodeType::PARITY;
            case BUCHI:
                return NodeType::BUCHI;
            case CO_BUCHI:
                return NodeType::CO_BUCHI;
            case SAFETY:
                return NodeType::WEAK;
            case CO_SAFETY:
                return NodeType::WEAK;
            default:
                throw std::invalid_argument("unsupported acceptance");
        }
    }

    Parity Automaton::initParity() const {
        switch (automaton_acceptance_condition(owl, automaton)) {
            case PARITY_MIN_EVEN:
            case PARITY_MAX_EVEN:
            case BUCHI:
            case SAFETY:
            case CO_SAFETY:
                return Parity::EVEN;
            case PARITY_MIN_ODD:
            case PARITY_MAX_ODD:
            case CO_BUCHI:
                return Parity::ODD;
            default:
                throw std::invalid_argument("unsupported acceptance");
        }
    }

    bool Automaton::initSafetyFilter() const {
        return
            automaton_acceptance_condition(owl, automaton) == SAFETY &&
            automaton_is_singleton(owl, automaton);
    }

    ParityType Automaton::initParityType() const {
        switch (automaton_acceptance_condition(owl, automaton)) {
            case SAFETY:
            case CO_SAFETY:
            case BUCHI:
            case CO_BUCHI:
            case PARITY_MIN_EVEN:
            case PARITY_MIN_ODD:
                return ParityType::MIN;
            case PARITY_MAX_EVEN:
            case PARITY_MAX_ODD:
                return ParityType::MAX;
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

    Parity Automaton::getParity() const {
        return parity;
    }

    void Automaton::add_new_query(node_id_t query) {
        std::unique_lock<std::mutex> lock(query_mutex);
        queries.push(query);
        query_mutex.unlock();
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
            automaton_edge_tree(owl, automaton,
                    local_state,
                    &owl_nodes, &owl_leaves, &owl_scores);

            tree.reserve(owl_nodes.length);
            tree.insert(tree.end(), owl_nodes.elements, owl_nodes.elements + owl_nodes.length);

            leaves.reserve(owl_leaves.length);
            for (int i = 0; i < owl_leaves.length; i += 2) {
                node_id_t successor_state;
                int32_t state = owl_leaves.elements [i];
                int32_t color = owl_leaves.elements [i + 1];
                double score = owl_scores.elements [i/2];
                if (state == -1) {
                    successor_state = NODE_BOTTOM;
                    color = 1 - parity;
                    score = 0.0;
                }
                else if (state == -2) {
                    successor_state = NODE_TOP;
                    color = parity;
                    score = 1.0;
                }
                else {
                    successor_state = state;
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
                if (parity_type == ParityType::MAX) {
                    color = max_color - color;
                }
                leaves.push_back(ScoredEdge(successor_state, color, score, 1.0));
            }

            // free memory allocated by owl
            free_unmanaged_memory(owl, owl_nodes.elements);
            free_unmanaged_memory(owl, owl_leaves.elements);
            free_unmanaged_memory(owl, owl_scores.elements);
            owl_nodes.elements = nullptr;
            owl_leaves.elements = nullptr;
            owl_scores.elements = nullptr;

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

    bool Automaton::hasSafetyFilter() {
        return has_safety_filter;
    }

    BDD Automaton::getSafetyFilter(Cudd manager, std::vector<int>& bdd_mapping) {
        add_successors(0);
        return successors[0].toBDD(manager, bdd_mapping);
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


    // --------------------------------------------------
    // class: ParityAutomatonTreeLeaf
    // --------------------------------------------------
    ParityAutomatonTreeLeaf::ParityAutomatonTreeLeaf(
        Automaton& _automaton,
        const std::string _name,
        const int _reference,
        std::vector<std::pair<letter_t,letter_t>> _mapping
    ) :
        ParityAutomatonTreeItem(_automaton.getNodeType(), _automaton.getParity(), _automaton.getMaxColor()),
        automaton(_automaton),
        name(std::move(_name)),
        reference(std::move(_reference)),
        mapping(std::move(_mapping))
    {
        automaton.setAlphabetSize(mapping.size());
    }

    ParityAutomatonTreeLeaf::~ParityAutomatonTreeLeaf() {}

    void ParityAutomatonTreeLeaf::getInitialState(product_state_t& state) {
        // save index in product state
        state_index = state.size();
        // initial state of automaton
        state.push_back(0);
    }

    ColorScore ParityAutomatonTreeLeaf::getSuccessor(const product_state_t& state, product_state_t& new_state, letter_t letter) {
        if (isBottomState(state)) {
            setBottomState(new_state);
            return ColorScore(1 - parity, 0.0, 1.0);
        }
        else if (isTopState(state)) {
            setTopState(new_state);
            return ColorScore(parity, 1.0, 1.0);
        }

        const node_id_t local_state = state[state_index];

        letter_t new_letter = 0;

        for (const auto& map : mapping) {
            new_letter |= ((letter & ((letter_t)1 << map.first)) >> map.first) << map.second;
        }

        ScoredEdge edge = automaton.getSuccessor(local_state, new_letter);
        new_state[state_index] = edge.successor;

        return edge.cs;
    }

    BDD ParityAutomatonTreeLeaf::computeSafetyFilter(Cudd manager, size_t var_begin, size_t var_end, int depth) {
        // check if automaton only talks about specified variables
        bool correct_filter = true;
        std::vector<int> bdd_mapping(mapping.size());
        for (const auto& map : mapping) {
            if (var_begin <= map.first && map.first < var_end) {
                assert(map.second >= 0 && map.second < mapping.size());
                bdd_mapping[map.second] = map.first - var_begin;
            }
            else {
                correct_filter = false;
                break;
            }
        }
        if (correct_filter && automaton.hasSafetyFilter()) {
            return automaton.getSafetyFilter(manager, bdd_mapping);
        }
        else {
            return manager.bddOne();
        }
    }

    void ParityAutomatonTreeLeaf::setState(product_state_t& new_state, node_id_t state) {
        new_state[state_index] = state;
    }

    void ParityAutomatonTreeLeaf::setTopState(product_state_t& new_state) {
        new_state[state_index] = NODE_TOP;
    }

    void ParityAutomatonTreeLeaf::setBottomState(product_state_t& new_state) {
        new_state[state_index] = NODE_BOTTOM;
    }

    bool ParityAutomatonTreeLeaf::isTopState(const product_state_t& state) const {
        return state[state_index] == NODE_TOP;
    }
    bool ParityAutomatonTreeLeaf::isBottomState(const product_state_t& state) const {
        return state[state_index] == NODE_BOTTOM;
    }

    int ParityAutomatonTreeLeaf::getMinIndex() const {
        return reference;
    }

    letter_t ParityAutomatonTreeLeaf::getMaximumAlphabetSize() const {
        return mapping.size();
    }

    std::set<letter_t> ParityAutomatonTreeLeaf::getAlphabet() const {
        std::set<letter_t> alphabet;
        std::transform(
            mapping.begin(), 
            mapping.end(),
            std::inserter(alphabet, alphabet.end()), 
            [](const auto& map) -> letter_t { 
                return map.first; 
            }
        );
        return alphabet;
    }

    void ParityAutomatonTreeLeaf::print(const int verbosity, const int indent) const {
        std::cout << std::setfill(' ') << std::setw(2*indent) << "";

        std::cout << state_index << ")";
        std::cout << " A[" << reference << "]";
        std::cout << " (";
        automaton.print_type();
        std::cout << ")";
        if (verbosity >= 2) {
            std::cout << " " << name;
        }
        std::cout << std::endl;
        if (verbosity >= 4) {
            for (const auto& map : mapping) {
                std::cout << std::setfill(' ') << std::setw(2*(indent+2)) << "" <<
                            "p" << map.first << " -> " << map.second << std::endl;
            }
        }
    }



    // --------------------------------------------------
    // class: ParityAutomatonTreeNode
    // --------------------------------------------------
    ParityAutomatonTreeNode::ParityAutomatonTreeNode(
            const node_type_t tag, const NodeType node_type, const Parity parity, const color_t max_color,
            const node_id_t round_robin_size, const bool parity_child,
            const color_t dp, std::vector<std::unique_ptr<ParityAutomatonTreeItem>> _children
    ) :
        ParityAutomatonTreeItem(node_type, parity, max_color),
        tag(tag),
        children(std::move(_children)),
        parity_child(parity_child),
        dp(dp),
        round_robin_size(round_robin_size)
    { }

    ParityAutomatonTreeNode::~ParityAutomatonTreeNode() { }

    void ParityAutomatonTreeNode::getInitialState(product_state_t& state) {
        // save index in product state
        state_index = state.size();
        if (round_robin_size > 1) {
            // round-robin counter
            state.push_back(0);
        }
        if (round_robin_size > 0 && parity_child) {
            // inverse minimal parity seen
            state.push_back(0);
        }
        for (auto &child : children) {
            child->getInitialState(state);
        }
    }

    ColorScore ParityAutomatonTreeNode::getSuccessor(const product_state_t& state, product_state_t& new_state, letter_t letter) {
        if (isBottomState(state)) {
            setBottomState(new_state);
            return ColorScore(1 - parity, 0.0, 1.0);
        }
        else if (isTopState(state)) {
            setTopState(new_state);
            return ColorScore(parity, 1.0, 1.0);
        }

        size_t round_robin_index = state_index;
        size_t min_parity_index = state_index;
        node_id_t round_robin_counter = 0;
        if (round_robin_size > 1) {
            round_robin_counter = state[round_robin_index];
            min_parity_index += 1;
        }
        color_t min_parity = dp;
        if (round_robin_size > 0 && parity_child) {
            min_parity -= state[min_parity_index];
        }

        node_id_t buchi_index = 0;
        size_t active_children = 0;

        color_t max_weak_color = 0;
        color_t min_weak_color = 1;
        color_t min_buchi_color = 1;

        double score = 0.0;
        double weights = 0.0;

        for (auto &child : children) {
            const ColorScore cs = child->getSuccessor(state, new_state, letter);
            const color_t child_color = cs.color;
            double child_score = cs.score;
            double child_weight = cs.weight;

            if (child->isBottomState(new_state)) {
                if (tag == CONJUNCTION) {
                    setBottomState(new_state);
                    return ColorScore(1 - parity, 0.0, 1.0);
                }
            }
            else if (child->isTopState(new_state)) {
                if (tag == DISJUNCTION) {
                    setTopState(new_state);
                    return ColorScore(parity, 1.0, 1.0);
                }
            }
            else {
                active_children++;

                double log_one_half = log(0.5);
                if (tag == CONJUNCTION) {
                    child_weight *= log(child_score);
                }
                else {
                    child_weight *= log(1.0 - child_score);
                }
                child_weight /= log_one_half;
            }

            bool increase_score = false;
            bool decrease_score = false;
            switch (child->node_type) {
                case NodeType::WEAK:
                    max_weak_color = std::max(max_weak_color, child_color);
                    min_weak_color = std::min(min_weak_color, child_color);
                    break;
                case NodeType::BUCHI:
                case NodeType::CO_BUCHI:
                    if (
                            (tag == CONJUNCTION && child->node_type == NodeType::BUCHI) ||
                            (tag == DISJUNCTION && child->node_type == NodeType::CO_BUCHI)
                    ) {
                        if (child_color == 0 && round_robin_counter == buchi_index) {
                            if (child->node_type == NodeType::BUCHI) {
                                increase_score = true;
                            }
                            else if (child->node_type == NodeType::CO_BUCHI) {
                                decrease_score = true;
                            }
                            round_robin_counter++;
                        }
                        buchi_index++;
                    }
                    else {
                    min_buchi_color = std::min(min_buchi_color, child_color);
                    }
                    break;
                case NodeType::PARITY:
                    if (parity == child->parity) {
                        if (child_color < min_parity) {
                            min_parity = child_color;
                            if (min_parity % 2 == parity) {
                                increase_score = true;
                            }
                            else {
                                decrease_score = true;
                            }
                        }
                    }
                    else {
                        if (child_color + 1 < min_parity) {
                            min_parity = child_color + 1;
                            if (min_parity % 2 == parity) {
                                increase_score = true;
                            }
                            else {
                                decrease_score = true;
                            }
                        }
                    }
                    break;
            }

            if (increase_score) {
                child_score = (0.75 + 0.25*child_score);
                child_weight *= 2.0;
            }
            else if (decrease_score) {
                child_score = (0.25*child_score);
                child_weight *= 2.0;
            }
            score += child_score * child_weight;
            weights += child_weight;
        }

        if (active_children == 0) {
            // discard children
            if (tag == CONJUNCTION) {
                setTopState(new_state);
                return ColorScore(parity, 1.0, 1.0);
            }
            else {
                setBottomState(new_state);
                return ColorScore(1 - parity, 0.0, 1.0);
            }
        }

        score /= weights;

        color_t color;
        bool reset = false;

        if (tag == CONJUNCTION && max_weak_color != 0) {
            reset = true;
            color = 1 - parity;
        }
        else if (tag == DISJUNCTION && min_weak_color == 0) {
            reset = true;
            color = parity;
        }
        else if (min_buchi_color == 0) {
            reset = true;
            color = 0;
        }
        else if (round_robin_counter == round_robin_size) {
            reset = true;
            if (parity_child) {
                color = min_parity;
            }
            else {
                if (tag == CONJUNCTION) {
                    color = parity;
                }
                else {
                    color = 1 - parity;
                }
            }
        }
        else {
            // output neutral color, not accepting for conjunction and accepting for disjunction
            color = max_color;
        }

        if (reset) {
            round_robin_counter = 0;
            min_parity = dp;
        }
        if (round_robin_size > 1) {
            new_state[round_robin_index] = round_robin_counter;
        }
        if (round_robin_size > 0 && parity_child) {
            new_state[min_parity_index] = dp - min_parity;
        }
        return ColorScore(color, score, weights);
    }

    BDD ParityAutomatonTreeNode::computeSafetyFilter(Cudd manager, size_t var_begin, size_t var_end, int depth) {
        BDD filter = manager.bddOne();
        if (depth == 0 && tag == CONJUNCTION) {
            for (auto &child : children) {
                filter &= child->computeSafetyFilter(manager, var_begin, var_end, depth + 1);
            }
        }
        return filter;
    }

    void ParityAutomatonTreeNode::setState(product_state_t& new_state, node_id_t state) {
        size_t round_robin_index = state_index;
        size_t min_parity_index = state_index;
        if (round_robin_size > 1) {
            new_state[round_robin_index] = state;
            min_parity_index += 1;
        }
        if (round_robin_size > 0 && parity_child) {
            new_state[min_parity_index] = state;
        }
        for (auto &child : children) {
            child->setState(new_state, state);
        }
    }

    void ParityAutomatonTreeNode::setTopState(product_state_t& new_state) {
        if (tag == DISJUNCTION) {
            setState(new_state, NODE_NONE_TOP);
            new_state[state_index] = NODE_TOP;
        }
        else {
            size_t round_robin_index = state_index;
            size_t min_parity_index = state_index;
            if (round_robin_size > 1) {
                new_state[round_robin_index] = NODE_NONE;
                min_parity_index += 1;
            }
            if (round_robin_size > 0 && parity_child) {
                new_state[min_parity_index] = NODE_NONE;
            }
            for (auto &child : children) {
                child->setTopState(new_state);
            }
        }
    }

    void ParityAutomatonTreeNode::setBottomState(product_state_t& new_state) {
        if (tag == CONJUNCTION) {
            setState(new_state, NODE_NONE_BOTTOM);
            new_state[state_index] = NODE_BOTTOM;
        }
        else {
            size_t round_robin_index = state_index;
            size_t min_parity_index = state_index;
            if (round_robin_size > 1) {
                new_state[round_robin_index] = NODE_NONE;
                min_parity_index += 1;
            }
            if (round_robin_size > 0 && parity_child) {
                new_state[min_parity_index] = NODE_NONE;
            }
            for (auto &child : children) {
                child->setBottomState(new_state);
            }
        }
    }

    bool ParityAutomatonTreeNode::isTopState(const product_state_t& state) const {
        if (tag == DISJUNCTION) {
            // need to check both in case of nested disjunction/conjunction
            return state[state_index] == NODE_TOP && state[state_index + 1] == NODE_NONE_TOP;
        }
        else {
            for (auto &child : children) {
                if (!child->isTopState(state)) {
                    return false;
                }
            }
            return true;
        }
    }
    bool ParityAutomatonTreeNode::isBottomState(const product_state_t& state) const {
        if (tag == CONJUNCTION) {
            // need to check both in case of nested conjunction/disjunction
            return state[state_index] == NODE_BOTTOM && state[state_index + 1] == NODE_NONE_BOTTOM;
        }
        else {
            for (auto &child : children) {
                if (!child->isBottomState(state)) {
                    return false;
                }
            }
            return true;
        }
    }

    int ParityAutomatonTreeNode::getMinIndex() const {
        int min_index = std::numeric_limits<int>::max();
        for (const auto& child : children) {
            min_index = std::min(min_index, child->getMinIndex());
        }
        return min_index;
    }

    letter_t ParityAutomatonTreeNode::getMaximumAlphabetSize() const {
        letter_t max_size = MIN_LETTER;
        for (const auto& child : children) {
            max_size = std::max(max_size, child->getMaximumAlphabetSize());
        }
        return max_size;
    }

    std::set<letter_t> ParityAutomatonTreeNode::getAlphabet() const {
        std::set<letter_t> alphabet;
        for (const auto& child : children) {
            std::set<letter_t> child_alphabet = child->getAlphabet();
            alphabet.insert(child_alphabet.begin(), child_alphabet.end());
        }
        return alphabet;
    }

    void ParityAutomatonTreeNode::print(const int verbosity, const int indent) const {
        std::cout << std::setfill(' ') << std::setw(2*indent) << "";
        std::cout << "*) ";
        switch(tag) {
            case CONJUNCTION:
                std::cout << "Conjunction";
                break;
            case DISJUNCTION:
                std::cout << "Disjunction";
                break;
            case BICONDITIONAL:
                std::cout << "Biconditional";
                break;
            case AUTOMATON:
                throw std::invalid_argument("Automaton in non-leaf position");
        }
        std::cout << " (";
        print_type();
        std::cout << ")" << std::endl;

        size_t round_robin_index = state_index;
        size_t min_parity_index = state_index;
        if (round_robin_size > 1 && tag != BICONDITIONAL) {
            std::cout << std::setfill(' ') << std::setw(2*(indent+1)) << "";
            std::cout << round_robin_index << ") Round Robin LAR" << std::endl;;
            min_parity_index += 1;
        }
        if (round_robin_size > 0) {
            for (unsigned int i = 0;
                (tag == BICONDITIONAL && i < round_robin_size) ||
                (tag != BICONDITIONAL && i < 1 && parity_child);
            i++) {
                std::cout << std::setfill(' ') << std::setw(2*(indent+1)) << "";
                std::cout << min_parity_index + i << ") Parity LAR" << std::endl;;
            }
        }

        for (const auto& child : children) {
            child->print(verbosity, indent + 1);
        }
    }

    // --------------------------------------------------
    // class: ParityAutomatonTreeBiconditionalNode
    // --------------------------------------------------
    ParityAutomatonTreeBiconditionalNode::ParityAutomatonTreeBiconditionalNode(
            const NodeType node_type, const Parity parity, const color_t max_color,
            const node_id_t round_robin_size, const bool parity_child,
            const color_t dp, const int parity_child_index,
            std::vector<std::unique_ptr<ParityAutomatonTreeItem>> _children
    ) :
        ParityAutomatonTreeNode(BICONDITIONAL, node_type, parity, max_color, round_robin_size, parity_child, dp, std::move(_children)),
        parity_child_index(parity_child_index),
        d1(children[1 - parity_child_index]->max_color),
        d2(children[parity_child_index]->max_color)
    { }

    ParityAutomatonTreeBiconditionalNode::~ParityAutomatonTreeBiconditionalNode() { }

    void ParityAutomatonTreeBiconditionalNode::getInitialState(product_state_t& state) {
        // save index in product state
        state_index = state.size();
        for (unsigned int i = 0; i < round_robin_size; i++) {
            // inverse minimal parity seen
            state.push_back(0);
        }
        for (auto &child : children) {
            child->getInitialState(state);
        }
    }

    ColorScore ParityAutomatonTreeBiconditionalNode::getSuccessor(const product_state_t& state, product_state_t& new_state, letter_t letter) {
        if (isBottomState(state)) {
            setBottomState(new_state);
            return ColorScore(1 - parity, 0.0, 1.0);
        }
        else if (isTopState(state)) {
            setTopState(new_state);
            return ColorScore(parity, 1.0, 1.0);
        }

        color_t min_parity = dp;
        for (unsigned int i = 0; i < round_robin_size; i++) {
            min_parity = std::min(min_parity, dp - state[state_index + i]);
        }

        size_t active_children = 0;

        bool bottom = false;
        bool top = false;
        color_t child_colors[2];

        double min_score = 1.0;
        double max_score = 0.0;

        double score = 0.0;
        double weights = 0.0;

        int child_index = 0;
        for (auto &child : children) {
            const ColorScore cs = child->getSuccessor(state, new_state, letter);
            const color_t child_color = cs.color;

            double child_score = cs.score;
            double child_weight = cs.weight;
            bool increase_score = false;
            bool decrease_score = false;

            min_score = std::min(min_score, child_score);
            max_score = std::max(max_score, child_score);

            child_colors[child_index] = child_color;

            if (child->isBottomState(new_state)) {
                bottom = true;
            }
            else if(child->isTopState(new_state)) {
                top = true;
            }
            else {
                active_children++;

                double log_one_half = log(0.5);
                child_weight *= std::min(log(child_score), log(1.0 - child_score)) / log_one_half;
            }

            if (child_index == parity_child_index) {
                if (child_color < min_parity) {
                    min_parity = child_color;
                    if (min_parity % 2 == parity) {
                        increase_score = true;
                    }
                    else {
                        decrease_score = true;
                    }
                }
            }

            if (increase_score) {
                child_score = (0.75 + 0.25*child_score);
                child_weight *= 2.0;
            }
            if (decrease_score) {
                child_score = (0.25*child_score);
                child_weight *= 2.0;
            }
            score += child_score * child_weight;
            weights += child_weight;

            child_index++;
        }

        if (active_children == 0) {
            if (bottom && top) {
                setBottomState(new_state);
                return ColorScore(1 - parity, 0.0, 1.0);
            }
            else {
                setTopState(new_state);
                return ColorScore(parity, 1.0, 1.0);
            }
        }

        score /= weights;

        if (parity_child) {
            color_t color;
            const color_t c1 = child_colors[1 - parity_child_index];
            const color_t c2 = child_colors[parity_child_index];
            if (children[1 - parity_child_index]->node_type == NodeType::WEAK) {
                // one weak child
                color = c1 + c2;
            }
            else {
                // compute color
                if (c1 < d1) {
                    color = c1 + std::min(c2, d2 - state[state_index + c1]);
                }
                else {
                    color = c1 + c2;
                }

                // compute state update
                for (unsigned int i = 0; i < round_robin_size; i++) {
                    if (c1 <= i) {
                        new_state[state_index + i] = 0;
                    }
                    else {
                        new_state[state_index + i] = d2 - std::min(c2, d2 - state[state_index + i]);
                    }
                }
            }
            return ColorScore(color, score, weights);
        }
        else {
            // only weak children
            if (child_colors[0] == child_colors[1]) {
                return ColorScore(parity, score, weights);
            }
            else {
                return ColorScore(1 - parity, score, weights);
            }
        }
    }

    BDD ParityAutomatonTreeBiconditionalNode::computeSafetyFilter(Cudd manager, size_t var_begin, size_t var_end, int depth) {
        return manager.bddOne();
    }

    void ParityAutomatonTreeBiconditionalNode::setState(product_state_t& new_state, node_id_t state) {
        for (unsigned int i = 0; i < round_robin_size; i++) {
            new_state[state_index + i] = state;
        }
        children[0]->setState(new_state, state);
        children[1]->setState(new_state, state);
    }

    void ParityAutomatonTreeBiconditionalNode::setTopState(product_state_t& new_state) {
        if (round_robin_size > 0) {
            setState(new_state, NODE_NONE_TOP);
            new_state[state_index] = NODE_TOP;
        }
        else {
            // decide to set both to top
            children[0]->setTopState(new_state);
            children[1]->setTopState(new_state);
        }
    }

    void ParityAutomatonTreeBiconditionalNode::setBottomState(product_state_t& new_state) {
        if (round_robin_size > 0) {
            setState(new_state, NODE_NONE_BOTTOM);
            new_state[state_index] = NODE_BOTTOM;
        }
        else {
            // decide to set first to bottom and second to top
            children[0]->setBottomState(new_state);
            children[1]->setTopState(new_state);
        }
    }

    bool ParityAutomatonTreeBiconditionalNode::isTopState(const product_state_t& state) const {
        if (round_robin_size > 0) {
            return state[state_index] == NODE_TOP;
        }
        else {
            return children[0]->isTopState(state) && children[1]->isTopState(state);
        }
    }
    bool ParityAutomatonTreeBiconditionalNode::isBottomState(const product_state_t& state) const {
        if (round_robin_size > 0) {
            return state[state_index] == NODE_BOTTOM;
        }
        else {
            return children[0]->isBottomState(state) && children[1]->isTopState(state);
        }
    }

    // --------------------------------------------------
    // class: AutomatonTreeStructure
    // --------------------------------------------------   
    AutomatonTreeStructure::AutomatonTreeStructure(JVM owl, size_t alphabet_size) :
        owl(owl),
        alphabet_size(alphabet_size)
    {
        statuses.reserve(alphabet_size);
        for (size_t i = 0; i < alphabet_size; i++) {
            statuses.push_back(USED);
        }
    }

    AutomatonTreeStructure::AutomatonTreeStructure(AutomatonTreeStructure&& other) :
        owl(other.owl),
        owl_automaton(other.owl_automaton),
        alphabet_size(std::move(other.alphabet_size)),
        automata(std::move(other.automata)),
        tree(std::move(other.tree)),
        statuses(std::move(other.statuses)),
        leaves(std::move(other.leaves)),
        leaf_state_indices(std::move(other.leaf_state_indices)),
        initial_state(std::move(other.initial_state))
    {
        other.owl_automaton = nullptr;
    }

    AutomatonTreeStructure AutomatonTreeStructure::fromFormula(Isolate isolate, JVM owl, OwlFormula formula, size_t num_inputs, size_t num_outputs, bool simplify) {
        size_t alphabet_size = num_inputs + num_outputs;
        AutomatonTreeStructure structure(owl, alphabet_size);

        if (simplify) {
            int* statuses = new int[alphabet_size];
            formula = ltl_formula_simplify(owl, formula, num_inputs, statuses, alphabet_size);
            for (size_t i = 0; i < alphabet_size; i++) {
                structure.statuses[i] = (atomic_proposition_status_t)statuses[i];
            }
            delete[] statuses;
        }

        OwlDecomposedDPA decomposed_automaton = decomposed_dpa_of(owl, formula);
        structure.owl_automaton = decomposed_automaton;

        int num_automata = decomposed_dpa_automata_size(owl, decomposed_automaton);

        for (int i = 0; i < num_automata; i++) {
            OwlAutomaton automaton = decomposed_dpa_automata_get(owl, decomposed_automaton, i);
            structure.automata.emplace_back(isolate, owl, automaton);
        }

        OwlStructure owl_structure = decomposed_dpa_structure_get(owl, decomposed_automaton);

        std::vector<ParityAutomatonTreeLeaf*> leaves;
        structure.tree = structure.constructTree(owl_structure, leaves);
        structure.tree->getInitialState(structure.initial_state);
        for (const ParityAutomatonTreeLeaf* leaf : leaves) {
            structure.leaf_state_indices.push_back(leaf->getStateIndex());
        }

        return structure;
    }

    AutomatonTreeStructure AutomatonTreeStructure::fromAutomaton(Isolate isolate, JVM owl, OwlAutomaton automaton, size_t num_inputs, size_t num_outputs) {
        size_t alphabet_size = num_inputs + num_outputs;
        AutomatonTreeStructure structure(owl, alphabet_size);
        structure.owl_automaton = nullptr;

        structure.automata.emplace_back(isolate, owl, automaton);
        std::string name = "HOA";
        const int reference = 0;
        // create mapping
        std::vector<std::pair<letter_t, letter_t>> mapping;
        mapping.reserve(alphabet_size);
        for (letter_t i = 0; i < alphabet_size; i++) {
            mapping.push_back({i, i});
        }
        // make automaton into tree
        ParityAutomatonTreeLeaf* leaf = new ParityAutomatonTreeLeaf(
                structure.automata[reference],
                name,
                reference,
                mapping
        );
        structure.leaves.push_back(leaf);
        structure.tree = std::unique_ptr<ParityAutomatonTreeItem>(leaf);

        structure.tree->getInitialState(structure.initial_state);
        structure.leaf_state_indices.push_back(structure.tree->getStateIndex());

        return structure;
    }

    AutomatonTreeStructure::~AutomatonTreeStructure() {
        if (owl_automaton != nullptr) {
            destroy_object_handle(owl, owl_automaton);
        }
    }

    std::unique_ptr<ParityAutomatonTreeItem> AutomatonTreeStructure::constructTree(OwlStructure tree, std::vector<ParityAutomatonTreeLeaf*>& leaves) {
        const node_type_t tag = decomposed_dpa_structure_node_type(owl, tree);
        if (tag == AUTOMATON) {
            const int reference = decomposed_dpa_structure_referenced_automaton(owl, tree);
            Automaton& automaton = automata[reference];
            // create mapping
            std::vector<std::pair<letter_t, letter_t>> mapping;
            mapping.reserve(alphabet_size);
            for (letter_t i = 0; i < alphabet_size; i++) {
                int j;
                j = decomposed_dpa_structure_referenced_alphabet_mapping(owl, tree, i);
                if (j >= 0) {
                    mapping.push_back({i, (letter_t)j});
                }
            }
            // create name
            OwlFormula formula = decomposed_dpa_structure_referenced_formula(owl, tree);
            std::string name = strix_owl::owl_object_to_string(owl, formula);
            destroy_object_handle(owl, formula);
            // move tree into leaf
            ParityAutomatonTreeLeaf* leaf = new ParityAutomatonTreeLeaf(automaton, name, reference, mapping);
            leaves.push_back(leaf);
            return std::unique_ptr<ParityAutomatonTreeItem>(leaf);
        }
        else {
            // defaults for weak nodes
            NodeType node_type = NodeType::WEAK;
            Parity parity = Parity::EVEN;
            color_t max_color = 1;
            node_id_t round_robin_size = 0;

            int parity_child = false;
            color_t parity_child_max_color = 0;
            Parity parity_child_parity = Parity::EVEN;
            int parity_child_index = 0;

            std::vector<std::unique_ptr<ParityAutomatonTreeItem>> children;
            for (int i = 0; i < decomposed_dpa_structure_children(owl, tree); i++) {
                OwlStructure child = decomposed_dpa_structure_get_child(owl, tree, i);

                std::unique_ptr<ParityAutomatonTreeItem> pchild = constructTree(child, leaves);
                if (pchild->node_type == NodeType::PARITY) {
                    if (tag == BICONDITIONAL) {
                        parity_child = true;
                    }
                    else if (parity_child) {
                        throw std::invalid_argument("unsupported automaton tree");
                    }
                    else {
                        parity_child = true;
                        parity_child_parity = pchild->parity;
                        parity_child_max_color = pchild->max_color;
                    }
                }
                else if (pchild->node_type == NodeType::BUCHI) {
                    if (tag == CONJUNCTION) {
                        round_robin_size++;
                    }
                    else if (tag == BICONDITIONAL) {
                        parity_child = true;
                    }
                }
                else if (pchild->node_type == NodeType::CO_BUCHI) {
                    if (tag == DISJUNCTION) {
                        round_robin_size++;
                    }
                    else if (tag == BICONDITIONAL) {
                        parity_child = true;
                    }
                }

                node_type = join_node_type(node_type, pchild->node_type);

                children.push_back(std::move(pchild));
            }

            // sort children from "small,simple" to "large,complex" as a heuristic for more efficient product construction
            std::sort(children.begin(), children.end(), cmp_trees);

            if (tag == BICONDITIONAL) {
                const NodeType t1 = children[0]->node_type;
                const NodeType t2 = children[1]->node_type;
                node_type = join_node_type_biconditional(t1, t2);

                if (parity_child) {
                    // need to treat one child as a parity child and other as child updating the lar

                    if (t1 == NodeType::WEAK) {
                        parity_child_index = 1;
                    }
                    else if (t2 == NodeType::WEAK) {
                        parity_child_index = 0;
                    }
                    else if (children[0]->max_color < children[1]->max_color) {
                        parity_child_index = 1;
                    }
                    else {
                        parity_child_index = 0;
                    }
                    parity_child_parity = children[parity_child]->parity;
                    parity_child_max_color = children[parity_child]->max_color;
                }
            }

            if (node_type == NodeType::PARITY) {
                if (tag == CONJUNCTION || tag == DISJUNCTION) {
                    if (tag == CONJUNCTION) {
                        parity = Parity::ODD;
                    }
                    else {
                        parity = Parity::EVEN;
                    }
                    if (parity_child) {
                        if (parity != parity_child_parity) {
                            parity_child_max_color++;
                        }
                        max_color = parity_child_max_color;
                        if (round_robin_size > 0 && max_color % 2 != 0) {
                            max_color++;
                        }
                    }
                    else {
                        // needs to have co-buchi and buchi children
                        max_color = 2;
                    }
                }
                else {
                    if (parity_child) {
                        color_t d1 = children[1 - parity_child_index]->max_color;
                        color_t d2 = children[parity_child_index]->max_color;
                        Parity p1 = children[1 - parity_child_index]->parity;
                        Parity p2 = children[parity_child_index]->parity;

                        if (children[1 - parity_child_index]->node_type == NodeType::WEAK) {
                            // one weak child
                            max_color = d2 + 1;
                            parity = p2;
                            round_robin_size = 0;
                        }
                        else {
                            max_color = d1 + d2;
                            parity = (Parity)(((int)p1 + (int)p2) % 2);
                            round_robin_size = d1;
                        }
                        parity_child_max_color = d2;
                    }
                    // only weak children otherwise, leave defaults
                }
            }
            else if (node_type == NodeType::BUCHI) {
                parity = Parity::EVEN;
            }
            else if (node_type == NodeType::CO_BUCHI) {
                parity = Parity::ODD;
            }

            // intermediate pointer not needed any more
            destroy_object_handle(owl, tree);

            if (tag == CONJUNCTION || tag == DISJUNCTION) {
                return std::unique_ptr<ParityAutomatonTreeItem>(
                        new ParityAutomatonTreeNode(
                            tag, node_type, parity, max_color, round_robin_size, parity_child, parity_child_max_color, std::move(children)));
            }
            else {
                return std::unique_ptr<ParityAutomatonTreeItem>(
                        new ParityAutomatonTreeBiconditionalNode(
                            node_type, parity, max_color, round_robin_size, parity_child, parity_child_max_color,
                            parity_child_index, std::move(children)));
            }
        }
    }

    Parity AutomatonTreeStructure::getParity() const {
        return tree->parity;
    }

    color_t AutomatonTreeStructure::getMaxColor() const {
        return tree->max_color;
    }

    product_state_t AutomatonTreeStructure::getInitialState() const {
        return initial_state;
    }

    ColorScore AutomatonTreeStructure::getSuccessor(const product_state_t& state, product_state_t& new_state, letter_t letter) {
        return tree->getSuccessor(state, new_state, letter);
    }

    BDD AutomatonTreeStructure::computeSafetyFilter(Cudd manager, size_t var_begin, size_t var_end) {
        return tree->computeSafetyFilter(manager, var_begin, var_end, 0);
    }

    std::vector<int> AutomatonTreeStructure::getAutomatonStates(const product_state_t& state) const {
        std::vector<int> automaton_states;
        automaton_states.reserve(leaf_state_indices.size());
        for (size_t index : leaf_state_indices) {
            node_id_t local_state = state[index];
            int owl_state;
            if (local_state == NODE_NONE) {
                throw std::invalid_argument("local state should never be NONE");
            }
            else if (local_state == NODE_BOTTOM || local_state == NODE_NONE_BOTTOM) {
                owl_state = -1;
            }
            else if (local_state == NODE_TOP || local_state == NODE_NONE_TOP) {
                owl_state = -2;
            }
            else {
                owl_state = local_state;
            }

            automaton_states.push_back(owl_state);
        }

        return automaton_states;
    }

    bool AutomatonTreeStructure::declareWinning(const product_state_t& state, const Player winner) {
        std::vector<int> automaton_states = getAutomatonStates(state);
        switch (winner) {
            case Player::SYS_PLAYER:
                return decomposed_dpa_declare_realizability_status(owl, owl_automaton, REALIZABLE, automaton_states.data(), automaton_states.size());
            case Player::ENV_PLAYER:
                return decomposed_dpa_declare_realizability_status(owl, owl_automaton, UNREALIZABLE, automaton_states.data(), automaton_states.size());
            default:
                return false;
        }
    }

    Player AutomatonTreeStructure::queryWinner(const product_state_t& state) {
        std::vector<int> automaton_states = getAutomatonStates(state);
        realizability_status_t status = decomposed_dpa_query_realizability_status(owl, owl_automaton, automaton_states.data(), automaton_states.size());
        switch (status) {
            case REALIZABLE:
                return Player::SYS_PLAYER;
            case UNREALIZABLE:
                return Player::ENV_PLAYER;
            default:
                return Player::UNKNOWN_PLAYER;
        }
    }

    bool AutomatonTreeStructure::isTopState(const product_state_t& state) const {
        return tree->isTopState(state);
    }

    bool AutomatonTreeStructure::isBottomState(const product_state_t& state) const {
        return tree->isBottomState(state);
    }

    std::set<letter_t> AutomatonTreeStructure::getAlphabet() const {
        return tree->getAlphabet();
    }

    atomic_proposition_status_t AutomatonTreeStructure::getVariableStatus(int variable) const {
        return statuses[variable];
    }

    void AutomatonTreeStructure::print(const int verbosity) const {
        tree->print(verbosity);
    }

    void AutomatonTreeStructure::print_memory_usage() const {
        for (const auto& automaton : automata) {
            automaton.print_memory_usage();
        }
    }     
}
