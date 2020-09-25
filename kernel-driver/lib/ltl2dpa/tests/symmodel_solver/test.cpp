#include <iostream>
#include <string>
#include <vector>

#include "SymLtlSpec.h"
#include "Ltl2Dpa.h"
#include "pgame.h"
#include "pgsolver.h"
#include "machine.h"

void PrintStringVector(std::vector<std::string> vec){
    for (auto str : vec) std::cout << str << " ";
    std::cout << std::endl;
}

int main(){

    size_t verbosity = 6;

    // the symbolic details
    const size_t n_states = 17;
    const size_t n_controls = 4;
    const size_t x_initial = 0;
    const size_t x_overflow = 16;
    auto model_posts = [](const symbolic_t state, const symbolic_t control) { 

        std::vector<symbolic_t> ret;

        //   states: i => x_i (for i \in [0;15])
        // controls: 0 => left, 1 => up, 2 => right, 3 => down
        const symbolic_t T[16][4] = {
                 /* u: 00, 01, 02, 03 */
        /* x_00 : */   16,  4,  1, 16,
        /* x_01 : */    0,  5,  2, 16,
        /* x_02 : */    1,  6,  3, 16,
        /* x_03 : */    2,  7, 16, 16,
        /* x_04 : */   16,  8,  5,  0,
        /* x_05 : */    4,  9,  6,  1,
        /* x_06 : */    5, 10,  7,  2,
        /* x_07 : */    6, 11, 16,  3,
        /* x_08 : */   16, 12,  9,  4,
        /* x_09 : */    8, 13, 10,  5,
        /* x_10 : */    9, 14, 11,  6,
        /* x_11 : */   10, 15, 16,  7,
        /* x_12 : */   16, 16, 13,  8,
        /* x_13 : */   12, 16, 14,  9,
        /* x_14 : */   13, 16, 15, 10,
        /* x_15 : */   14, 16, 16, 11
        };
        if(state >= 0 && state <= 15){
            ret.push_back(T[state][control]); 
            return ret;
        }
        else{
            ret.push_back(x_overflow); 
            return ret; 
        }
    };

    // the symbolic control problem
    SymControlProblem<decltype(model_posts)> symControlProblem(n_states, n_controls, x_initial, model_posts, "FG(t)&G(!(a))");
    symControlProblem.add_AP(AP_TYPE::STATE_AP, "t", {7,3});
    symControlProblem.add_AP(AP_TYPE::STATE_AP, "a", {1,5,9,2});

    std::vector<std::string> inVars = symControlProblem.get_in_vars();
    std::vector<std::string> outVars = symControlProblem.get_out_vars();
    std::string ltlSpec = symControlProblem.get_ltl_formula();

    //ltlSpec = std::string("G(s & c)&") + ltlSpec;

    std::cout << "Mapped In vars: "; PrintStringVector(inVars);
    std::cout << "Mapped Out vars: "; PrintStringVector(outVars);
    std::cout << "Mapped LTL formula: " << ltlSpec << std::endl;

    
    // construct the specs DPA
    std::cout << "Creating the DPA tree struct from the LTL formula ... " << std::flush;
    strix_ltl2dpa::Ltl2DpaConverter converter;
    strix_aut::AutomatonTreeStructure autoStruct = converter.toAutTreeStruct(ltlSpec, inVars, outVars, false);
    std::cout << "done!" << std::endl;


    std::cout << "Alphabet: ";
    auto alpha = autoStruct.getAlphabet();
    std::set<strix_aut::letter_t>::iterator alphaIterator = alpha.begin();
    while(alphaIterator != alpha.end()){
        auto alpha_idx = (*alphaIterator);
        std::cout << alpha_idx;
        if(alpha_idx < inVars.size())
            std::cout << "(in:" << inVars[alpha_idx] << "), ";
        else
            std::cout << "(out:" << outVars[alpha_idx-inVars.size()] << "), ";

        alphaIterator++;
    }
    std::cout << std::endl;    


    std::cout << "The DPA tree structure:" << std::endl;
    autoStruct.print(verbosity);   

    std::cout << "Initial state of the tree: (";
    auto initState = autoStruct.getInitialState();
    size_t stateSize = initState.size();
    for (size_t i = 0; i < stateSize; i++){
        std::cout << initState[i];
        if (i < stateSize-1)
            std::cout << ", ";
    }
    std::cout << ")" << std::endl; 


    // Construct a Parity Game
    std::cout << "Constructing a parity game from the DPA and symbolic problem ... " << std::endl << std::flush;
    PGame game = PGame(inVars.size(), outVars.size(), autoStruct);
    game.constructArena(symControlProblem, verbosity);
    game.print_info();

    //return 0;

    std::cout << "Solving the parity game ... " << std::endl << std::flush;
    PGSISolver solver = PGSISolver(game);
    solver.solve(verbosity);
    strix_aut::Player winner = solver.getWinner();
    switch (winner) {
        case strix_aut::Player::SYS_PLAYER:
            std::cout << "System wins" << std::endl;
            break;
        case strix_aut::Player::ENV_PLAYER:
            std::cout << "Environment wins" << std::endl;
            break;
        case strix_aut::Player::UNKNOWN_PLAYER:
            std::cout << "UNKNOWN wins" << std::endl;
            break;
    }


    // generate the implementation
    if (winner == strix_aut::Player::SYS_PLAYER){
        std::cout << "Extracting the system implementation ... " << std::flush;
        MealyMachine m(inVars, outVars, Semantic::MEALY);
        m.construct(game, solver);
        std::cout << "done!" << std::endl;

        auto cnt_state_trans = m.getTransitions();
        std::cout << "The contoroller has " << cnt_state_trans.size() << " states" << std::endl;
        size_t idx=0;
        for(auto stat_trans : cnt_state_trans){
            std::cout << "State " << idx << ":\t";
            for (auto one_trans : stat_trans){
                std::cout << "(->" << one_trans.nextState << ",i:x_" << one_trans.input << ",o:";
                for(auto out : one_trans.output){
                    std::cout << "u_" << out << " ";
                }
                std::cout << "\b) ";
            }
            std::cout << std::endl;
            idx++;
        }

        std::cout << "Simulating the closed-loop: " << std::endl;
        SymState mdl_state = symControlProblem.get_initial_state();
        state_id_t cnt_state = 0;
        // loop until a target is reached
        while(!symControlProblem.is_sym_state_in_AP(mdl_state, "t")){ 
            std::cout << "Symbolic model at state x_" << mdl_state.value << std::endl;

            // find a transition in the mealy machine that accepts the mdl state as input
            bool found = false;
            symbolic_t cnt_out;
            state_id_t cnt_next_state;
            std::cout << "current controller state has " << cnt_state_trans[cnt_state].size() << " transitins." << std::endl;
            for(auto one_trans : cnt_state_trans[cnt_state]){
                if(one_trans.input == mdl_state.value){
                    found = true;
                    cnt_out = one_trans.output[0];  // we pick first control for simplicity
                    cnt_next_state = one_trans.nextState;
                }
            }
            if(!found)
                throw std::runtime_error("no possible transition in the mealy machine !");

            std::cout << "the controller issues: u_" << cnt_out << std::endl;

            // prepare for next loop
            cnt_state = cnt_next_state;
            mdl_state = symControlProblem.get_posts(mdl_state, cnt_out)[0]; // we pick first state for simplicity
        }
        std::cout << "Simulation is done !";
    }

    return 0;
}

