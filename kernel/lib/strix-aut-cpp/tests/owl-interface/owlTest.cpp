#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <set>

#include "owl.h"

std::vector<std::string> jar_files = {
    "owl.jar"
/*
    "owl-19.06.03.jar",
    "antlr4-runtime-4.7.2.jar",
    "SparseBitSet-1.1.jar",
    "j2objc-annotations-1.3.jar",
    "animal-sniffer-annotations-1.17.jar",
    "jbdd-0.4.0.jar",
    "jhoafparser-1.1.1-patched.jar",
    "checker-qual-2.8.1.jar",
    "jsr305-3.0.2.jar",
    "commons-cli-1.4.jar ",
    "error_prone_annotations-2.3.2.jar",
    "naturals-util-0.10.0.jar",
    "failureaccess-1.0.1.jar",
    "fastutil-8.2.1.jar",
    "value-2.7.1-annotations.jar",
    "guava-28.0-jre.jar",
    "listenablefuture-9999.0-empty-to-avoid-conflict-with-guava.jar"
*/
};                             

inline bool file_exists (const std::string& name) {
	std::ifstream f(name.c_str());
	      return f.good();
}

std::string owlClassPath() {
    const std::string owlLibPath = std::string(OWL_LIB_PATH);
    std::stringstream cp;

    cp << "-D" << "java.class.path=" << "\":";
    for (size_t i = 0; i < jar_files.size(); i++){
        std::string jar_file = owlLibPath + jar_files[i];
        if(!file_exists(jar_file))
	        throw std::runtime_error(std::string("A required jar file is not found: ") + jar_file);        
        cp << jar_file << ":";
    }
    cp << "\"";
    return cp.str();
}

void printAutomatonInfo(const owl::Automaton& dpa){
    std::cout << "\tAutomaton constructed with ";
    switch(dpa.acceptance()) {
        case owl::PARITY_MIN_EVEN:
            std::cout << "(min,even) parity" << std::endl;
            break;

        case owl::PARITY_MAX_EVEN:
            std::cout << "(max,even) parity" << std::endl;
            break;

        case owl::PARITY_MIN_ODD:
            std::cout << "(min,odd) parity" << std::endl;
            break;

        case owl::PARITY_MAX_ODD:
            std::cout << "(max,odd) parity" << std::endl;
            break;

        default:
            std::cout << "not a DPA" << std::endl;
    }
}

std::vector<std::string> inVars = {"t0", "t1", "o0"};
std::vector<std::string> outVars = {"a","b"};
std::string ltlText = "F(G(t0)) & F(G(t1)) & G(!(o0))";

int main(){

    std::string strOwlClassPath = owlClassPath();
    std::cout << "Creating a JVM with ... " << std::flush;
    owl::OwlJavaVM owlJavaVM(strOwlClassPath.c_str(), true, 0, 0, false);
    owl::OwlThread owl = owlJavaVM.attachCurrentThread();
    std::cout << "done!" << std::endl;

    std::cout << "Parsing the LTL formula ... " << std::flush;
    std::vector<std::string> mapping;
    for (size_t i = 0; i < inVars.size(); i++) mapping.push_back(inVars[i]);
    for (size_t i = 0; i < outVars.size(); i++) mapping.push_back(outVars[i]);    
    owl::FormulaFactory factory = owl.createFormulaFactory();
    owl::Formula formula = factory.parse(ltlText, mapping);
    std::cout << "done!" << std::endl;

    std::cout << "Formula: ";
    formula.print();

    std::cout << "Creating the DPA ... " << std::flush;
    owl::DecomposedDPA dDpa = owl.createAutomaton(formula, true, true, inVars.size());
    std::cout << "done!" << std::endl;
    size_t numDPAs = dDpa.automata().size();
    std::cout << "# Automata constructed: " <<  numDPAs << std::endl;
    for (size_t i = 0; i < numDPAs; i++){
        std::cout << "Automaton " << i << ":" << std::endl;
        printAutomatonInfo(dDpa.automata()[i]);
    }


        

    return 0;
}

