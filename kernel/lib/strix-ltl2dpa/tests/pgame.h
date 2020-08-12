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

// a suitable number for reserving initial items in the vectors
constexpr size_t RESERVE = 4096;


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
template <typename Container>
struct container_hash {
    std::size_t operator()(Container const& c) const {
        return boost::hash_range(c.cbegin(), c.cend());
    }
};
size_t hash_value(const Edge& edge) {
    return std::hash<Edge>()(edge);
}
size_t hash_value(const BDD& bdd) {
    return (size_t)bdd.getNode();
}




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


struct ScoredProductStateComparator {
    bool operator() (ScoredProductState& s1, ScoredProductState& s2) {
        return s1.score < s2.score;
    }
};


template <class T, class Container = std::vector<T>, class Compare = std::less<T> >
class PQ : public std::priority_queue<T, Container, Compare> {
public:
    Container& container() { return this->c; }
};
typedef PQ<ScoredProductState, std::deque<ScoredProductState>, ScoredProductStateComparator> state_queue;
    

template <typename I>
struct SpecSeq {
    //the i-th bit is 1 iff the i-th element is specified and 1, otherwise the bit is 0
    I number;

    //the i-th bit is 1 iff the i-th element is not specified
    I unspecifiedBits;

    SpecSeq() : number(0), unspecifiedBits(0) {}
    SpecSeq(I n) : number(n), unspecifiedBits(0) {}
    SpecSeq(I n, I u) : number(n), unspecifiedBits(u) {}

    //bits that are specified in both outputs must be the same
    bool isCompatible(const SpecSeq<I>& other) const {
        const I mask = unspecifiedBits | other.unspecifiedBits;
        return (number | mask) == (other.number | mask);
    }

    //bits are specified if they are specified in at least one of the sequences
    //assumes that sequences have the same length and that they are compatible
    SpecSeq<I> intersect(const SpecSeq<I>& other) const {
        return SpecSeq<I>(number | other.number, unspecifiedBits & other.unspecifiedBits);
    }

    //two sequences are disjoint if their intersection is empty
    bool isDisjoint(const SpecSeq<I>& other) const {
        const I mask = unspecifiedBits | other.unspecifiedBits;
        return (number | mask) != (other.number | mask);
    }

    //all words that are in this sequence, but not in the other
    //assumes that there is at least one such word
    std::vector<SpecSeq<I>> diff(const SpecSeq<I>& other, int numBits) const {
        std::vector<SpecSeq<I>> ret;

        SpecSeq<I> last = *this;
        for (int b = 0; b < numBits; b++) {
            I bit = ((I)1 << b);
            if ((unspecifiedBits & bit) != 0 && (other.unspecifiedBits & bit) == 0) {
                SpecSeq<I> newSeq = last;

                newSeq.unspecifiedBits &= ~bit;
                last.unspecifiedBits &= ~bit;

                newSeq.number |= ((~other.number) & bit);
                last.number |= (other.number & bit);

                ret.push_back(newSeq);
            }
        }

        return ret;
    }

    //if a bit of this seq is unspecified, the corresponding bit of the other seq must also be unspecified
    //all specified bits must be equal
    bool isSubset(const SpecSeq<I>& other) const {
        return ((unspecifiedBits | other.unspecifiedBits) == other.unspecifiedBits) && isCompatible(other);
    }

    bool isFullySpecified() const {
        return unspecifiedBits == 0;
    }

    bool operator<(const SpecSeq<I>& other) const {
        return number < other.number || (number == other.number && unspecifiedBits < other.unspecifiedBits);
    }

    bool operator==(const SpecSeq<I>& other) const {
        return number == other.number && unspecifiedBits == other.unspecifiedBits;
    }

    bool operator!=(const SpecSeq<I>& other) const {
        return !operator==(other);
    }

    BDD toBDD(const Cudd& manager, const int numBits) {
        constexpr int bitwidth = std::numeric_limits<I>::digits + std::numeric_limits<I>::is_signed;
        int array[bitwidth];
        for (int b = 0; b < numBits; b++) {
            const I bit = ((I)1 << b);
            if ((unspecifiedBits & bit) == 0) {
                array[b] = (number & bit) >> b;
            }
            else {
                array[b] = -1;
            }
        }
        return BDD(manager, Cudd_CubeArrayToBdd(manager.getManager(), array));
    }

    std::string toString(const int numBits) const {
        std::stringstream s;
        for (I val = 1; val < ((I)1 << numBits); val <<= 1) {
            if ((unspecifiedBits & val) == 0) {
                if ((number & val) == 0) {
                    s << "0";
                }
                else {
                    s << "1";
                }
            }
            else {
                s << "-";
            }
        }
        return s.str();
    }

    std::string toVectorString(const std::vector<int>& accumulatedBits) const {
        std::stringstream s;
        s << "(";
        bool empty = true;
        for (size_t i = 0; i + 1 < accumulatedBits.size(); i++) {
            int startBit = accumulatedBits[i];
            int endBit = accumulatedBits[i+1];
            int numBits = endBit - startBit;
            if (numBits > 0) {
                if (!empty) {
                    s << ",";
                }
                else {
                    empty = false;
                }

                I mask = (((I)1 << numBits) - 1);
                I num = (number >> startBit) & mask;
                I unspec = (unspecifiedBits >> startBit) & mask;
                if (unspec != mask) {
                    s << num;
                }
                else {
                    s << "-";
                }
            }
        }
        s << ")";
        return s.str();
    }

    std::string toStateLabelledString(const std::vector<int>& accumulatedBits) const {
        std::stringstream s;
        int totalBits = accumulatedBits.back();
        if (unspecifiedBits == (((I)1 << totalBits) - 1)) {
            // add top/true
            s << "&#8868;";
        }
        else {
            bool empty = true;
            for (size_t i = 0; i + 1 < accumulatedBits.size(); i++) {
                int startBit = accumulatedBits[i];
                int endBit = accumulatedBits[i+1];
                int numBits = endBit - startBit;
                if (numBits > 0) {
                    I mask = (((I)1 << numBits) - 1);
                    I num = (number >> startBit) & mask;
                    I unspec = (unspecifiedBits >> startBit) & mask;
                    if (unspec != mask) {
                        if (!empty) {
                            // add logical and
                            s << "&#8743;";
                        }
                        else {
                            empty = false;
                        }
                        s << "s";
                        // add index using Unicode subscript numbers
                        int index = i;
                        std::vector<int> digits;
                        if (index == 0) {
                            digits.push_back(0);
                        }
                        else {
                            while (index > 0) {
                                digits.push_back(index % 10);
                                index /= 10;
                            }
                        }
                        for (auto it = digits.rbegin(); it != digits.rend(); it++) {
                            s << "&#832" << *it << ";";
                        }
                        s << "=" << num;
                    }
                }
            }
        }
        return s.str();
    }

    std::string toLabelledString(const int numBits, const std::vector<std::string>& bitNames) const {
        std::stringstream s;
        if (unspecifiedBits == (((I)1 << numBits) - 1)) {
            // add top/true
            s << "&#8868;";
        }
        else {
            bool empty = true;
            for (int bit = 0; bit < numBits; bit++) {
                I val = ((I)1 << bit);

                if ((unspecifiedBits & val) == 0) {
                    if (!empty) {
                        // add logical and
                        s << "&#8743;";
                    }
                    else {
                        empty = false;
                    }
                    if ((number & val) == 0) {
                        // add logical negation
                        s << "&#172;";
                    }
                    s << bitNames[bit];
                }
            }
        }
        return s.str();
    }

    friend std::ostream &operator<<(std::ostream &out, const SpecSeq<I>& output) {
        std::vector<int> bits;
        constexpr int num_bits = 8;
        int count = 0;
        I n = output.number;
        I d = output.unspecifiedBits;

        while (n > 0 || count < num_bits) {
            if (d%2 == 0) {
                bits.push_back(n%2);
            }
            else {
                bits.push_back(2);
            }

            n >>= 1;
            d >>= 1;
            count++;
        }

        for (auto bit = bits.rbegin(); bit != bits.rend(); bit++) {
            if (*bit == 2) {
                out << "-";
            }
            else {
                out << *bit;
            }
        }
        return out;
    }
};

template <typename I>
class SpecSeqIterator {
private:
    I bits_mask;
    I dont_care_mask;

    std::vector<unsigned int> bit_positions;

    I num_elements;
    I current_val;

public:
    SpecSeqIterator() :
        bits_mask(0),
        dont_care_mask(0),
        num_elements(0),
        current_val(0)
    {}

    SpecSeqIterator(I specifiedBits, I unspecifiedBits, I dontCareBits, const unsigned int numBits) :
        bits_mask(specifiedBits),
        dont_care_mask(dontCareBits)
    {
        bit_positions.reserve(numBits);
        for (unsigned int b = 0; b < numBits; b++) {
            I bit = ((I)1 << b);
            if ((unspecifiedBits & bit) != 0 && (dont_care_mask & bit) == 0) {
                bit_positions.push_back(b);
            }
        }
        num_elements = ((I)1 << bit_positions.size());
        current_val = 0;
    }

    SpecSeqIterator& operator=(const SpecSeqIterator<I>& other) {
        bits_mask = other.bits_mask;
        dont_care_mask = other.dont_care_mask;
        bit_positions = other.bit_positions;
        num_elements = other.num_elements;
        current_val = other.current_val;
        return *this;
    }

    ~SpecSeqIterator() {}

    inline void operator++() {
        current_val++;
    }
    inline void operator++(int) {
        operator++();
    }

    inline const SpecSeq<I> operator*() const {
        I value = 0;
        for (unsigned int b = 0; b < bit_positions.size(); b++) {
            value |= ((current_val & ((I)1 << b)) >> b) << bit_positions[b];
        }
        return SpecSeq<I>(value | bits_mask, dont_care_mask);
    }

    inline bool done() const {
        return current_val == num_elements;
    }

    inline void reset() {
        current_val = 0;
    }
};

template <typename I>
class BDDtoSpecSeqIterator {
private:
    BDD bdd;
    const int numBits;
    bool done_;

    // current cube
    DdGen* gen;
    int* cube;
    CUDD_VALUE_TYPE value;
    SpecSeq<I> current;

    void update() {
        if (Cudd_IsGenEmpty(gen)) {
            done_ = true;
        }
        else {
            // create SpecSeq
            current = SpecSeq<I>();

            for (int i = 0; i < numBits; i++) {
                const I b = ((I)1 << i);
                switch (cube[i]) {
                    case 0:
                        // bit not set
                        break;
                    case 1:
                        // bit set
                        current.number |= b;
                        break;
                    case 2:
                        // unspecified bit
                        current.unspecifiedBits |= b;
                        break;
                }
            }
        }
    }

public:
    BDDtoSpecSeqIterator(BDD bdd, const int numBits) :
        bdd(bdd), numBits(numBits), done_(false)
    {
        // initialize gen and grab first cube
        gen = Cudd_FirstCube(bdd.manager(), bdd.getNode(), &cube, &value);
        update();
    }

    ~BDDtoSpecSeqIterator() {
        Cudd_GenFree(gen);
    }

    inline void operator++() {
        // grab next cube
        Cudd_NextCube(gen, &cube, &value);
        update();
    }
    inline void operator++(int) {
        operator++();
    }

    inline const SpecSeq<I> operator*() const {
        return current;
    }

    inline bool done() const {
        return done_;
    }
};

class PGame{

    // a link to the DPA-tree-struct
    strix_aut::AutomatonTreeStructure& structure;

    // input/output iterators
    SpecSeqIterator<strix_aut::letter_t> input_iterator;
    std::vector<SpecSeqIterator<strix_aut::letter_t>> output_iterators;
    strix_aut::letter_t true_inputs_mask;
    strix_aut::letter_t true_outputs_mask;
    strix_aut::letter_t false_inputs_mask;
    strix_aut::letter_t false_outputs_mask;

    // Manager for BDDs of inputs and outputs
    Cudd manager_input_bdds;
    Cudd manager_output_bdds;

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
    std::vector<BDD> sys_output;
    std::vector<BDD> env_input;
    std::vector<strix_aut::node_id_t> env_node_map;
    std::vector<bool> env_node_reachable;
    std::vector<strix_aut::product_state_t> product_states;


    // winning things
    std::vector<strix_aut::Player> sys_winner;
    std::vector<strix_aut::Player> env_winner;


public:
    size_t n_inputs;             // number of input vars
    size_t n_outputs;            // number of output vars
    bool complete;               // signalling that the arena is completely constructed
    bool solved;                 // variable signalling that the arena is already solved
    strix_aut::Parity parity;    // parity of the game
    strix_aut::color_t n_colors; // number of colors


    PGame(const size_t _n_inputs, const size_t _n_outputs, strix_aut::AutomatonTreeStructure& _structure):
        structure(_structure), 
        n_inputs(_n_inputs), n_outputs(_n_outputs),
        complete(false), solved(false), 
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
        //winning_queue.reserve(RESERVE);
        //unreachable_queue.reserve(RESERVE);

        // initialize the nodes/edges
        sys_succs_begin.push_back(0);
        env_succs_begin.push_back(0);

        // initialize manager for BDDs
        manager_input_bdds = Cudd(n_inputs);
        manager_output_bdds = Cudd(n_outputs);
        manager_input_bdds.AutodynDisable();
        manager_output_bdds.AutodynDisable();

        // construct masks for unused and always true or false inputs and outputs
        true_inputs_mask = 0;
        true_outputs_mask = 0;
        false_inputs_mask = 0;
        false_outputs_mask = 0;     

        // check which inputs are UNUSED, CONS_TRUE or CONS_FALSE
        strix_aut::letter_t unused_inputs_mask = 0;
        strix_aut::letter_t irrelevant_inputs_mask = 0;
        for (strix_aut::letter_t a = 0; a < n_inputs; a++) {
            const strix_aut::letter_t bit = ((strix_aut::letter_t)1 << a);
            atomic_proposition_status_t status = _structure.getVariableStatus(a);
            if (status != USED) {
                irrelevant_inputs_mask |= bit;
                if (status == UNUSED) {
                    unused_inputs_mask |= bit;
                }
                else if (status == CONSTANT_TRUE) {
                    true_inputs_mask |= bit;
                }
                else if (status == CONSTANT_FALSE) {
                    false_inputs_mask |= bit;
                }
                else {
                    std::cerr << "Invalid status for input var (idx=" << a << "): " << status << std::endl;
                    assert(false);
                }
            }
        }        

        // initialize the outputs iterator   
        strix_aut::letter_t unused_outputs_mask = 0;
        strix_aut::letter_t irrelevant_outputs_mask = 0;    
        for (strix_aut::letter_t a = 0; a < n_outputs; a++) {
            const strix_aut::letter_t bit = ((strix_aut::letter_t)1 << a);
            const strix_aut::letter_t b = a + n_inputs;
            atomic_proposition_status_t status = _structure.getVariableStatus(b);
            if (status != USED) {
                irrelevant_outputs_mask |= bit;
                if (status == UNUSED) {
                    unused_outputs_mask |= bit;
                }
                else if (status == CONSTANT_TRUE) {
                    true_outputs_mask |= bit;
                }
                else if (status == CONSTANT_FALSE) {
                    false_outputs_mask |= bit;
                }
                else {
                    std::cerr << "Invalid status for output var (idx=" << a << "): " << status << std::endl;
                    assert(false);
                }
            }
        }


        // compute safety filter
        BDD filter = _structure.computeSafetyFilter(manager_output_bdds, n_inputs, n_inputs + n_outputs);
        BDDtoSpecSeqIterator<strix_aut::letter_t> it(filter, n_outputs);
        while (!it.done()) {
            SpecSeq<strix_aut::letter_t> cur = *it;
            it++;
            output_iterators.push_back(SpecSeqIterator<strix_aut::letter_t>(
                    cur.number & ~irrelevant_outputs_mask,
                    cur.unspecifiedBits & ~irrelevant_outputs_mask,
                    irrelevant_outputs_mask,
                    n_outputs
            ));
        }
        input_iterator = SpecSeqIterator<strix_aut::letter_t>(0, ~irrelevant_inputs_mask, irrelevant_inputs_mask, n_inputs);

    }
    ~PGame() {
        // clean up BDDs before manager is destroyed
        std::vector<BDD>().swap(env_input);
        std::vector<BDD>().swap(sys_output);    
    }    

    void constructArena(const int verbosity = 0){

        // get initial state of the Spec-DPA-tree
        const strix_aut::product_state_t initial_state = structure.getInitialState();

        product_state_size = initial_state.size();

        // the states
        std::vector<strix_aut::product_state_t> states;
        states.reserve(RESERVE);

        // map from product states in queue to scores and reference ids
        std::unordered_map<strix_aut::product_state_t, MinMaxState, container_hash<strix_aut::product_state_t> > state_map;  

        // queues for min/max states
        state_queue queue_max;
        state_queue queue_min;


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
        const strix_aut::node_id_t top_node_ref = env_node_map.size();
        env_node_map.push_back(strix_aut::NODE_TOP);
        env_node_reachable.push_back(true);
        states.push_back({});

        // add ref for initial node
        initial_node_ref = env_node_map.size();
        env_node_map.push_back(strix_aut::NODE_NONE);
        env_node_reachable.push_back(true);  

        // add the initial state
        ScoredProductState initial(1.0, initial_node_ref);
        state_map.insert({ initial_state, MinMaxState(initial) });
        queue_max.push(initial);
        states.push_back(std::move(initial_state));              

        // some vars to manage new nodes
        bool use_max_queue = true;
        std::unordered_set<strix_aut::node_id_t> already_queried;
        bool new_winning_nodes = false;
        bool new_declared_nodes = false;
        size_t winning_nodes_found = 0;
        size_t losing_nodes_found = 0;
        size_t unreachable_nodes_found = 0;
        size_t queried_nodes = 0;

        // the exploration loop : keep exploring till solved
        while (!solved && !(queue_max.empty() && queue_min.empty())) {

            // get next state
            ScoredProductState scored_state = std::move(queue_max.top());
            queue_max.pop();
            const strix_aut::node_id_t ref_id = scored_state.ref_id;
            
            // already explored ?
            if (env_node_map[ref_id] != strix_aut::NODE_NONE) {
                continue;
            }

            const strix_aut::node_id_t env_node = n_env_nodes;
            env_node_map[ref_id] = env_node;
            env_node_to_ref_id.insert({ env_node, ref_id });

            if (verbosity >= 1) {
                std::cout << " [" << std::setw(4) << env_node << "] Computing successors for " << std::setw(4) << ref_id;
                std::cout << " (" << std::fixed << std::setprecision(3) << std::setw(6) << std::abs(scored_state.score) << ") = (";
                for (const auto s : states[ref_id]) {
                    if (s == strix_aut::NODE_TOP) {
                        std::cout << "  ⊤";
                    }
                    else if (s == strix_aut::NODE_BOTTOM) {
                        std::cout << "  ⊥";
                    }
                    else if (s == strix_aut::NODE_NONE) {
                        std::cout << "  -";
                    }
                    else if (s == strix_aut::NODE_NONE_TOP) {
                        std::cout << " -⊤";
                    }
                    else if (s == strix_aut::NODE_NONE_BOTTOM) {
                        std::cout << " -⊥";
                    }
                    else {
                        std:: cout << " " << std::setw(2) << s;
                    }
                }
                std::cout << " )";
                if (verbosity >= 3) {
                    std::cout << " : [";
                }
                else {
                    std::cout << std::endl;
                }
            }            

            strix_aut::edge_id_t cur_env_node_n_sys_edges = 0;
            strix_aut::node_id_t cur_n_sys_nodes = 0;
            std::map<strix_aut::node_id_t, BDD> env_successors;


            // for all inputs
            input_iterator.reset();
            while (!input_iterator.done()) {
                
                // get input letter
                SpecSeq<strix_aut::letter_t> input_letter = *input_iterator;
                input_iterator++;

                // new sys node with successors
                strix_aut::node_id_t sys_node = n_sys_nodes + cur_n_sys_nodes;
                std::map<Edge, BDD> sys_successors;
                strix_aut::edge_id_t cur_sys_node_n_sys_edges = 0;


                // for all output iterators
                for (auto& output_iterator : output_iterators) {
                    output_iterator.reset();

                    // for all outputs
                    while (!output_iterator.done()) {

                        // get output letter
                        SpecSeq<strix_aut::letter_t> output_letter = *output_iterator;
                        output_iterator++;

                        // compute joint letter for automata lookup
                        strix_aut::letter_t letter = input_letter.number + (output_letter.number << n_inputs);

                        // a var for the new state (yes it is only one as this is a *D*PA )
                        strix_aut::product_state_t new_state(product_state_size);

                        // get the next state (store in new_state)
                        const strix_aut::ColorScore cs = structure.getSuccessor(states[ref_id], new_state, letter);

                        // get the color of this transition
                        const strix_aut::color_t color = cs.color;

                        // score
                        strix_aut::node_id_t succ = env_node_map.size();
                        double score = -(double)succ;

                        // if not a buttom
                        if (!structure.isBottomState(new_state)) {

                            // is it top ?
                            if (structure.isTopState(new_state)) {
                                succ = top_node_ref;
                            }
                            // now: it is not top or buttom
                            else {
                                auto result = state_map.insert({ new_state, MinMaxState(score, succ) });

                                // is it new there ? (we use the result from the insert on the unordered_map state_map)
                                if (result.second){
                                    env_node_map.push_back(strix_aut::NODE_NONE);
                                    env_node_reachable.push_back(true);
                                    states.push_back(std::move(new_state));
                                    queue_max.push(ScoredProductState(score, succ));
                                }
                            }    

                            // now, add the edge
                            const strix_aut::node_id_t succ_node = env_node_map[succ];
                            if (succ_node == strix_aut::NODE_BOTTOM) {
                                // successor losing, do not add an edge
                            }
                            else {
                                Edge edge(succ, color);

                                // add output to bdd
                                BDD output_bdd = output_letter.toBDD(manager_output_bdds, n_outputs);

                                auto const result = sys_successors.insert({ edge, output_bdd });
                                if (!result.second) {
                                    result.first->second |= output_bdd;
                                }
                            }
                        }
                    }
                }

                if (verbosity >= 3) {
                    for (const auto& it : sys_successors) {
                        if (it.first.successor == top_node_ref) {
                            std::cout << "   ⊤   ";
                        }
                        else {
                            std::cout << " " << std::setw(2) << it.first.successor;
                            std::cout << ":" << std::setw(2) << it.first.color;
                            std::cout << " ";
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

                // add input to bdd
                BDD input_bdd = input_letter.toBDD(manager_input_bdds, n_inputs);
                auto const result2 = env_successors.insert({ sys_node, input_bdd });
                if (!result2.second) {
                    result2.first->second |= input_bdd;
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

            if (verbosity >= 3) {
                std::cout << "]" << std::endl;
            }

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

        // fill vector for product states
        product_states.resize(n_env_nodes);
        for (auto& entry : state_map) {
            const strix_aut::node_id_t node_id = env_node_map[entry.second.ref_id];
            if (node_id != strix_aut::NODE_NONE && node_id != strix_aut::NODE_BOTTOM && node_id != strix_aut::NODE_TOP) {
                product_states[node_id] = std::move(entry.first);
            }
        }        
    }

    void print_info() const {
        std::cout << "Number of env nodes  : " << n_env_nodes << std::endl;
        std::cout << "Number of sys nodes  : " << n_sys_nodes << std::endl;
        std::cout << "Number of env edges  : " << n_env_edges << std::endl;
        std::cout << "Number of sys edges  : " << n_sys_edges << std::endl;
        std::cout << "Number of colors     : " << n_colors << std::endl;
    }
};