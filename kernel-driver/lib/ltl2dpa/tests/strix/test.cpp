#include <iostream>
#include <string>
#include <vector>

#include "Ltl2Dpa.h"
#include "pgame.h"
#include "pgsolver.h"
#include "machine.h"

//std::vector<std::string> inVars = {"t0", "t1", "o0"};
//std::vector<std::string> outVars = {"a","b"};
//std::string ltlText = "F(G(t0)) & F(G(t1)) & G(!(o0))";

std::vector<std::string> inVars = {"r1", "r2"};
std::vector<std::string> outVars = {"g1","g2"};
std::string ltlText = "G(!g1 | !g2) & G(r1 -> F(g1)) & G(r2 -> F(g2))";


int main(){
    const size_t verbosity = 6;

    std::cout << "LTL Formula: " << ltlText << std::endl;

    std::cout << "Creating the DPA tree struct from the LTL formula ... " << std::flush;
    strix_ltl2dpa::Ltl2DpaConverter converter;
    strix_aut::AutomatonTreeStructure autoStruct = converter.toAutTreeStruct(ltlText, inVars, outVars);
    std::cout << "done!" << std::endl;

    std::cout << "Alphabet: ";
    auto alpha = autoStruct.getAlphabet();
    std::set<strix_aut::letter_t>::iterator alphaIterator = alpha.begin();
    while(alphaIterator != alpha.end()){
        std::cout << (*alphaIterator) << " ";
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
    std::cout << "Constructing a parity game from the DPA ... " << std::endl << std::flush;
    PGame game = PGame(inVars.size(), outVars.size(), autoStruct);
    game.constructArena(verbosity);
    game.print_info();

    // solve the the parity game
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
        if(verbosity >= 5)
            m.print_dot(std::cout);
    }
    
    

    return 0;
}

