#include <iostream>
#include <string>
#include <vector>

#include "StrixLtl2Dpa.h"

//std::vector<std::string> inVars = {"t0", "t1", "o0"};
//std::vector<std::string> outVars = {"a","b"};
//std::string ltlText = "F(G(t0)) & F(G(t1)) & G(!(o0))";

std::vector<std::string> inVars = {"r1", "r2"};
std::vector<std::string> outVars = {"g1","g2"};
std::string ltlText = "G(!g1 | !g2) & G(r1 -> F(g1)) & G(r2 -> F(g2))";


int main(){

    std::cout << "LTL Formula: " << ltlText << std::endl;

    std::cout << "Creating the DPA-tree-struct ... " << std::flush;
    strix_ltl2dpa::Ltl2DpaConverter converter;
    auto autoStruct = converter.toAutTreeStruct(ltlText, inVars, outVars);
    std::cout << "done!" << std::endl;

    std::cout << "Alphabet: ";
    auto alpha = autoStruct.getAlphabet();
    std::set<strix_aut::letter_t>::iterator it = alpha.begin();
    while(it != alpha.end()){
        std::cout << (*it) << " ";
        it++;
    }
    std::cout << std::endl;    


    std::cout << "Tree Structure:" << std::endl;
    autoStruct.print(4);   

    std::cout << "Initial state of the tree: (";
    auto initState = autoStruct.getInitialState();
    size_t stateSize = initState.size();
    for (size_t i = 0; i < stateSize; i++){
        std::cout << initState[i];
        if (i < stateSize-1)
            std::cout << ", ";
    }
    std::cout << ")" << std::endl;     

    

    return 0;
}

