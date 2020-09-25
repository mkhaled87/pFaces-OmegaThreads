#pragma once

/*
* omegaImplementation.h
*
*  date    : 16.09.2020
*  author  : M. Khaled
*  details : a class for synthesized controller implementation.
* 
*/

#include <iostream>
#include <iomanip>
#include <sstream>

#include <Ltl2Dpa.h>
#include <pfaces-sdk.h>

#include "omegaParityGames.h"

namespace pFacesOmegaKernels{

// some types
typedef uint32_t state_id_t;

// a machine transition
class MachineTransition {
public:
    state_id_t nextState;
    const symbolic_t input;
    const std::vector<symbolic_t> output;

    MachineTransition(const state_id_t nextState, const symbolic_t input, const std::vector<symbolic_t> output);

    static MachineTransition parse(std::istream& ist);
    static void print(const MachineTransition& machine, std::ostream& ost);
};

// type: the machine is a set of transition for each state
typedef std::vector<std::vector<MachineTransition>> machine_t;

// a machine can be mealy or moore
enum class MachineSemantic { MEALY, MOORE };

// a successor to hold the successor state and output
class MachineSuccessor {
public:
    state_id_t successor;
    std::vector<symbolic_t> output;

    MachineSuccessor();
    MachineSuccessor(const state_id_t successor, const std::vector<symbolic_t> output);

    bool operator==(const MachineSuccessor& other) const;
    bool operator<(const MachineSuccessor& other) const;
};

// the mealy/moore machine
template<class T, class L1, class L2>
class Machine {

    MachineSemantic semantic;
    machine_t machine;
    std::vector<symbolic_t> state_labels;
    std::vector<int> state_label_accumulated_bits;
    int state_label_bits;

    // constants
    const std::string err_invalid_input = "Invalid input file: ";

public:
    Machine(const MachineSemantic semantic);
    Machine(const std::string& filename);
    ~Machine();

    void setMachine(machine_t new_machine);
    void setStateLabels(std::vector<symbolic_t> labels, const int bits, std::vector<int> accumulated_bits);
    symbolic_t getStateLabel(const state_id_t s) const;
    state_id_t numberOfStates() const;
    int getStateLabelBits() const;
    bool hasLabels() const;
    const std::vector<std::vector<MachineTransition>>& getTransitions() const;
    void construct(PGame<T, L1, L2>& arena, PGSolver<T, L1, L2>& solver);

    void writeToFile(const std::string& filename) const;
    void loadFromFile(const std::string& filename);

private:
    void constructMealyMachine(PGame<T, L1, L2>& arena, PGSolver<T, L1, L2>& solver);
};

}