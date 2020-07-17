#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>

#include "owl.h"

inline bool file_exists (const std::string& name) {
	std::ifstream f(name.c_str());
	      return f.good();
}

std::string owlClassPath() {
    const std::string owlJarPath = std::string(OWL_JAR_LIB_NAME);

    if(!file_exists(owlJarPath))
	    throw std::runtime_error("jar file not found !");

    std::stringstream cp;
    cp << "-Djava.class.path=" << "\":" << owlJarPath << ":\"";
    return cp.str();
}

int main(){

    std::string strOwlClassPath = owlClassPath();

    std::cout << "Loading OWL from the jar (" << strOwlClassPath << ") ... " << std::flush;
    owl::OwlJavaVM owlJavaVM(strOwlClassPath.c_str(), false, 0, 0, false);
    owl::OwlThread owl = owlJavaVM.attachCurrentThread();
    std::cout << "done!" << std::endl;

    std::cout << "Parsing the LTL formula ... " << std::flush;
    std::vector<std::string> mapping = std::vector<std::string>({"T0", "T1", "O0"}); 
    std::string ltlText = "FG(T0) && FG(T1) && G(!(O0))";
    owl::FormulaFactory factory = owl.createFormulaFactory();
    owl::Formula formula = factory.parse(ltlText, mapping);
    std::cout << "done!" << std::endl;

    std::cout << "Formula: ";
    formula.print();
    std::cout << std::endl;

    std::cout << "Creating the DPA ... " << std::flush;
    std::cout << "done!" << std::endl;
    

    return 0;
}

