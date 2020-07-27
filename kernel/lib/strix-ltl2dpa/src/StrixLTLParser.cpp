#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

#include "StrixLTLParser.h"

namespace strix_ltl {

    // --------------------------------------------------
    // class: Parser
    // --------------------------------------------------  
    Parser::Parser(JVM owl) : owl(owl) { }

    Parser::~Parser() { }

    std::string Parser::parse_file(const std::string& input_file) {
        // read input file
        std::string input_string;

        std::ifstream infile(input_file);
        if (infile.is_open()) {
            input_string = std::string((std::istreambuf_iterator<char>(infile)), std::istreambuf_iterator<char>());
        }
        else {
            throw std::runtime_error("could not open input file");
        }
        if (infile.bad()) {
            throw std::runtime_error("could not read input file");
        }
        infile.close();

        return input_string;
    }

    Specification Parser::parse_string(const std::string& input_string, const int verbosity) const {
        if (verbosity >= 4) {
            std::cout << "Input: " << std::endl << input_string << std::endl;
        }
        Specification spec = parse_string_with_owl(owl, input_string);
        if (verbosity >= 2) {
            if (spec.owl_object != nullptr) {
                std::cout << "Object: " << strix_owl::owl_object_to_string(owl, spec.owl_object) << std::endl;
            }
            std::cout << "Inputs:";
            for (const auto& i : spec.inputs) {
                std::cout << " " << i;
            }
            std::cout << std::endl;
            std::cout << "Outputs:";
            for (const auto& o : spec.outputs) {
                std::cout << " " << o;
            }
            std::cout << std::endl;
        }

        return spec;
    }

    // --------------------------------------------------
    // class: LTLParser
    // --------------------------------------------------
    LTLParser::LTLParser(JVM owl, const std::vector<std::string>& inputs, const std::vector<std::string>& outputs, const std::string finite) :
        Parser(owl),
        inputs(inputs),
        outputs(outputs),
        finite(finite)
    { }

    LTLParser::~LTLParser() { }

    Specification LTLParser::parse_string_with_owl(
            JVM owl,
            const std::string& text
    ) const {
        // Create list of variables for parser
        int num_vars = inputs.size() + outputs.size();
        char** variables = new char*[num_vars];
        for (size_t i = 0; i < inputs.size(); i++) {
            variables[i] = const_cast<char*>(inputs[i].c_str());
        }
        for (size_t i = 0; i < outputs.size(); i++) {
            variables[i + inputs.size()] = const_cast<char*>(outputs[i].c_str());
        }

        // Parse with provided variables
        const bool finite_semantics = !finite.empty();
        OwlFormula formula;
        if (finite_semantics) {
            formula = ltl_formula_parse_with_finite_semantics(owl, const_cast<char*>(text.c_str()), variables, num_vars);
        }
        else {
            formula = ltl_formula_parse(owl, const_cast<char*>(text.c_str()), variables, num_vars);
        }
        delete[] variables;

        // Create list of variables for specification
        std::vector<std::string> spec_inputs = inputs;
        std::vector<std::string> spec_outputs = outputs;
        if (finite_semantics) {
            if (std::find(inputs.begin(), inputs.end(), finite) != inputs.end() || std::find(outputs.begin(), outputs.end(), finite) != outputs.end()) {
                std::stringstream message;
                message << "Proposition '" << finite << "' for LTLf transformation already appears in list of propositions. ";
                message << "Please rename it manually by giving a different argument to the '--from-ltlf' option.";
                throw std::invalid_argument(message.str());
            }
            spec_outputs.emplace_back(finite);
        }

        // Return specification with input and output variables
        return Specification(owl, formula, SpecificationType::FORMULA,  std::move(spec_inputs), std::move(spec_outputs));
    }
    
}