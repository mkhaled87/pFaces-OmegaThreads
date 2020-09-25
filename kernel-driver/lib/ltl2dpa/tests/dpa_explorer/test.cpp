#include <iostream>
#include <string>
#include <vector>

#include "dpa.h"

void PrintStringVector(std::vector<std::string> vec){
    for (auto str : vec) std::cout << str << " ";
    std::cout << std::endl;
}

int main(){

    size_t verbosity = 5;
    std::vector<std::string> inVars = {"s", "t1", "t2", "t3", "a"};
    std::vector<std::string> outVars = {"c"};
    std::string ltlSpec = "G(!(a)) & (F(t1) & (t1 => F(t2)) & (t2 => F(t3)) & (t3 => F(t1)))";
    std::cout << "In vars: "; PrintStringVector(inVars);
    std::cout << "Out vars: "; PrintStringVector(outVars);
    std::cout << "LTL formula: " << ltlSpec << std::endl;

    // extract the DPA as a simple labelled graph
    TotalDPA dpa1(inVars, outVars, ltlSpec);
    dpa1.printInfo();
    dpa1.writeToFile("dpa1.txt");
    TotalDPA dpa2("dpa1.txt");
    dpa2.writeToFile("dpa2.txt");
    std::cout << "Check the files (dpa1.txt) and (dpa2.txt) and they should be identical." << std::endl;

    return 0;
}

