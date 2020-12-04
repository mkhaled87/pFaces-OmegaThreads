/*
* omegaImplementation.cpp
*
*  date    : 16.09.2020
*  author  : M. Khaled
*  details : a class for synthesized controller implementation.
* 
*/

#include "omegaImplementation.h"
#include "omegaUtils.h"


namespace pFacesOmegaKernels{

// local constants
const state_id_t TOP_STATE = std::numeric_limits<state_id_t>::max();
const state_id_t NONE_STATE = std::numeric_limits<state_id_t>::max() - 1;
const symbolic_t ANY_INPUT = std::numeric_limits<symbolic_t>::max();
const symbolic_t ANY_OUTPUT = std::numeric_limits<symbolic_t>::max();

// ------------------------------------------
// class GameEdge
// ------------------------------------------
MachineTransition::MachineTransition(
        const state_id_t nextState,
        const symbolic_t input,
        const std::vector<symbolic_t> output
) 
:nextState(nextState), input(input), output(std::move(output)) {
}

MachineTransition MachineTransition::parse(std::istream& ist){
    
    std::string trans_line;
    std::getline(ist, trans_line);
    trans_line = pfacesUtils::strTrim(trans_line);

    if(trans_line[0] != '{' || trans_line[trans_line.size()-1] != '}')
        throw std::runtime_error("MachineTransition::parse: Input is not a valid MachineTransition. passed: " + trans_line);

    
    std::string inner = trans_line.substr(1,trans_line.size()-2);
    std::vector<std::string> trans_elements = pfacesUtils::strSplit(inner, ";", false);

    if(trans_elements.size() != 3)
        throw std::runtime_error("MachineTransition::parse: A transition should have ;-separated 3 elements. passed: " + inner);


    state_id_t nextState;
    std::stringstream ssElement0(trans_elements[0]);
    ssElement0 >> nextState;

    symbolic_t input;
    std::stringstream ssElement1(trans_elements[1]);
    ssElement1 >> input;

    std::string str_vec_outs = trans_elements[2];
    if(str_vec_outs[0] != '[' || str_vec_outs[str_vec_outs.size()-1] != ']')
        throw std::runtime_error("MachineTransition::parse: Inner outputs vector is not a valid. passed: (" + str_vec_outs + ")");

    std::vector<symbolic_t> output;
    std::string output_inner = str_vec_outs.substr(1,str_vec_outs.size()-2);
    std::vector<std::string> output_elements = pfacesUtils::strSplit(output_inner, ",", false);
    for (auto outElm:output_elements){
        symbolic_t tmp;
        std::stringstream ssOut(outElm);
        ssOut >> tmp;
        output.push_back(tmp);
    }
    
    return MachineTransition(nextState, input, output);
}

void MachineTransition::print(const MachineTransition& trans, std::ostream& ost){

    ost << "{";
    ost << trans.nextState << ";";
    ost << trans.input << ";[";
    for (size_t i=0; i<trans.output.size(); i++){
        ost << trans.output[i];
        if(i < trans.output.size()-1)
            ost << ",";
    }
    ost << "]}";
}

// ------------------------------------------
// class MachineSuccessor
// ------------------------------------------
MachineSuccessor::MachineSuccessor() 
: successor(0), output(0) {
}

MachineSuccessor::MachineSuccessor(const state_id_t successor, const std::vector<symbolic_t> output) 
:successor(successor), output(std::move(output)) {
}

bool MachineSuccessor::operator==(const MachineSuccessor& other) const {
    return (successor == other.successor && output == other.output);
}

bool MachineSuccessor::operator<(const MachineSuccessor& other) const {
    return std::tie(successor, output) < std::tie(other.successor, other.output);
}


// ------------------------------------------
// class Machine
// ------------------------------------------
template<class T, class L1, class L2>
Machine<T, L1, L2>::Machine(const MachineSemantic semantic) 
:semantic(semantic), state_label_bits(-1){
}

template<class T, class L1, class L2>
Machine<T, L1, L2>::Machine(const std::string& filename){
    loadFromFile(filename);
}

template<class T, class L1, class L2>
Machine<T, L1, L2>::~Machine() { 
}

template<class T, class L1, class L2>
void Machine<T, L1, L2>::setMachine(machine_t new_machine) {
    machine = std::move(new_machine);
}

template<class T, class L1, class L2>
void Machine<T, L1, L2>::setStateLabels(std::vector<symbolic_t> labels, const int bits, std::vector<int> accumulated_bits){
    state_label_bits = bits;
    state_labels = std::move(labels);
    state_label_accumulated_bits = std::move(accumulated_bits);
}

template<class T, class L1, class L2>
symbolic_t Machine<T, L1, L2>::getStateLabel(const state_id_t s) const{
    return state_labels[s];
}

template<class T, class L1, class L2>
state_id_t Machine<T, L1, L2>::numberOfStates() const{
    return machine.size();
}

template<class T, class L1, class L2>
int Machine<T, L1, L2>::getStateLabelBits() const{
    return state_label_bits;
}

template<class T, class L1, class L2>
bool Machine<T, L1, L2>::hasLabels() const{
    return state_label_bits >= 0;
}

template<class T, class L1, class L2>
const std::vector<std::vector<MachineTransition>>& Machine<T, L1, L2>::getTransitions() const{
    return machine;
}

template<class T, class L1, class L2>
void Machine<T, L1, L2>::construct(PGame<T, L1, L2>& arena, PGSolver<T, L1, L2>& solver){
    if(semantic == MachineSemantic::MEALY)
        constructMealyMachine(arena, solver);
    else
        std::runtime_error("Not yet implemented !");
}

template<class T, class L1, class L2>
void Machine<T, L1, L2>::writeToFile(const std::string& filename) const {

    std::ofstream ofs(filename);

    // semantic
    ofs << "semantic: ";
    switch(semantic){
        case MachineSemantic::MEALY:
            ofs << "mealy";
        break;

        case MachineSemantic::MOORE:
            ofs << "moore";
        break;
    }
    ofs << std::endl;

    // states
    ofs << "states: ";
    ofs << machine.size();
    ofs << std::endl;

    // per_state_transitions
    ofs << "state_transitions: ";
    std::vector<uint32_t> state_transitions;
    for (auto trans : machine)
        state_transitions.push_back(trans.size());
    OmegaUtils::print_vector(state_transitions, ofs);
    ofs << std::endl;


    // transitions in machine
    for (size_t s=0; s<machine.size(); s++){
        for (size_t t=0; t<machine[s].size(); t++){
            ofs << "trans_" << s << "_" << t << ": ";
            MachineTransition::print(machine[s][t], ofs);
            ofs << std::endl; 
        }
    }

    // state_labels
    ofs << "state_labels: ";
    OmegaUtils::print_vector(state_labels, ofs);
    ofs << std::endl;

    // state_label_accumulated_bits
    ofs << "state_label_accumulated_bits: ";
    OmegaUtils::print_vector(state_label_accumulated_bits, ofs);
    ofs << std::endl;

    // state_label_bits
    ofs << "state_label_bits: ";
    ofs << state_label_bits;
    ofs << std::endl;    
}

template<class T, class L1, class L2>
void Machine<T, L1, L2>::loadFromFile(const std::string& filename) {

    std::ifstream ifs(filename);
    std::string scanned = "";

    // semantic
    ifs >> scanned;
    if(scanned != std::string("semantic:"))
        throw std::runtime_error(err_invalid_input + "cant find semantic.");
    else{
        std::string str_semantic;
        ifs >> str_semantic;
        str_semantic = pfacesUtils::strTrim(str_semantic);
        if(str_semantic == std::string("mealy"))
            semantic = MachineSemantic::MEALY;
        else if(str_semantic == std::string("moore"))
            semantic = MachineSemantic::MOORE;
        else
            throw std::runtime_error("error !");
    }


    // states
    scanned = "";
    size_t n_states;
    ifs >> scanned;
    if(scanned != std::string("states:"))
        throw std::runtime_error(err_invalid_input + "cant find states.");
    else{
        ifs >> n_states;
    }

    // per_state_transitions
    scanned = "";
    std::vector<uint32_t> state_transitions;
    ifs >> scanned;
    if(scanned != std::string("state_transitions:"))
        throw std::runtime_error(err_invalid_input + "cant find state_transitions.");
    else{
        OmegaUtils::scan_vector(state_transitions, ifs);
    }

    // transitions in machine
    for (size_t s=0; s<n_states; s++){
        std::vector<MachineTransition> vecTrans;
        for (size_t t=0; t<state_transitions[s]; t++){
            std::string to_find = std::string("trans_") + std::to_string(s) + std::string("_") + std::to_string(t);
            scanned = "";
            ifs >> scanned;
            if(scanned != (to_find + std::string(":")))
                throw std::runtime_error(err_invalid_input + std::string("cant find ") + to_find + std::string("."));
            else{
                MachineTransition trans = MachineTransition::parse(ifs);
                vecTrans.push_back(trans);
            }
        }
        machine.push_back(vecTrans);
    }

    // state_labels
    scanned = "";
    ifs >> scanned;
    if(scanned != std::string("state_labels:"))
        throw std::runtime_error(err_invalid_input + "cant find state_labels.");
    else{
        OmegaUtils::scan_vector(state_labels, ifs);
    }

    // state_label_accumulated_bits
    scanned = "";
    ifs >> scanned;
    if(scanned != std::string("state_label_accumulated_bits:"))
        throw std::runtime_error(err_invalid_input + "cant find state_label_accumulated_bits.");
    else{
        OmegaUtils::scan_vector(state_label_accumulated_bits, ifs);
    }

    // state_label_bits
    scanned = "";
    ifs >> scanned;
    if(scanned != std::string("state_label_bits:"))
        throw std::runtime_error(err_invalid_input + "cant find state_label_bits.");
    else{
        ifs >> state_label_bits;
    }   
}

template<class T, class L1, class L2>
void Machine<T, L1, L2>::constructMealyMachine(PGame<T, L1, L2>& arena, PGSolver<T, L1, L2>& solver){
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

        std::map< MachineSuccessor, std::vector<symbolic_t>> input_list;

        for (strix_aut::edge_id_t env_edge = arena.getEnvSuccsBegin(env_node); env_edge != arena.getEnvSuccsEnd(env_node); env_edge++) {
            const strix_aut::node_id_t sys_node = arena.getEnvEdge(env_edge);
            std::map<strix_aut::node_id_t, std::vector<symbolic_t>> successor_list;

            for (strix_aut::edge_id_t sys_edge = arena.getSysSuccsBegin(sys_node); sys_edge != arena.getSysSuccsEnd(sys_node); sys_edge++) {
                if (solver.get_sys_successor(sys_edge)) {
                    auto edge = arena.getSysEdge(sys_edge);
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

                MachineSuccessor mealy_combined_successor(mealy_successor, sys_outputs);
                auto const result = input_list.insert({ mealy_combined_successor, env_inputs });
                if (!result.second) {
                    for(auto item : env_inputs)
                        result.first->second.push_back(item);    
                }
            }
        }

        for (const auto& entry : input_list) {
            for (auto inp : entry.second) {
                machine[state].push_back( MachineTransition(entry.first.successor, inp, entry.first.output) );
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
        machine.push_back({ MachineTransition(top_state, any_input, {any_output}) });
    else
        top_state = NONE_STATE;

    setMachine(std::move(machine));
}

/* force the compiler to implement the class for needed function types */
template class Machine<post_func_t, L_x_func_t, L_u_func_t>;


// ------------------------------------------
// class MachineCodeGenerator
// ------------------------------------------
std::string MachineCodeGenerator::transitions2pythonlist(const machine_t& machine_transitions, bool statePerLine){
    std::stringstream ss_transitions;

    size_t state_idx = 0;
    // for each state
    for(auto state_transitions : machine_transitions){
        ss_transitions << "[";

        // for each state-transition
        size_t trans_idx = 0;
        for(auto state_trans : state_transitions){
            ss_transitions << "[[" << state_trans.nextState << "], [";
            ss_transitions << state_trans.input << "], [";
            ss_transitions << pfacesUtils::vector2string(state_trans.output) << "]]";

            if(trans_idx != (state_transitions.size()-1)){
                ss_transitions << ",";
            }
            trans_idx++;
        }

        ss_transitions << "]";
        if(state_idx != (machine_transitions.size()-1)){
            ss_transitions << ",";
            if(statePerLine)
                ss_transitions << std::endl;
        }
        state_idx++;
    }

    return ss_transitions.str();
}
void MachineCodeGenerator::machine2rospython(const machine_t& machine_transitions, const std::string& node_name){

    // load the template
    std::string template_path = templates_path + std::string("ROS_PYTHON.py");
    std::string tmp_txt = pfacesFileIO::readTextFromFile(template_path);

    // machine data
    std::string machine_data = MachineCodeGenerator::transitions2pythonlist(machine_transitions, true);

    // replace some fields
    tmp_txt = pfacesUtils::strReplaceAll(tmp_txt, "##NODE_NAME##", node_name);
    tmp_txt = pfacesUtils::strReplaceAll(tmp_txt, "##MACHINE_DATA##", machine_data);

    // write
    std::string out_file = codeget_dir + std::string("ros_") + node_name + std::string(".py");
    pfacesFileIO::writeTextToFile(out_file, tmp_txt, false);
}
MachineCodeGenerator::MachineCodeGenerator(const std::string& krnl_path, const std::string name, const std::string& _codeget_dir){
    templates_path = krnl_path + std::string("templates") + std::string(PFACES_PATH_SPLITTER);
    machine_name = name;
    codeget_dir = _codeget_dir;
}
void MachineCodeGenerator::machine2code(const machine_t& machine_transitions, CodeTypes codeType){
    switch(codeType){
        case CodeTypes::ROS_PYTHON:
            machine2rospython(machine_transitions, machine_name);
            break;
        default:
            throw std::runtime_error("MachineCodeGenerator::machine2code: unsupported code type.");
    }
}
CodeTypes MachineCodeGenerator::parse_code_type(const std::string& strCodeType){
    if (strCodeType == std::string("ros-python"))
        return CodeTypes::ROS_PYTHON;

    throw std::runtime_error("MachineCodeGenerator::parse_code_type: unsupported code type.");
}

}