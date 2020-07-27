#pragma once

#include "StrixLTLParser.h"
#include "StrixAutomaton.h"

namespace strix_ltl2dpa {

    class Ltl2DpaConverter{
    private:
        Isolate isolate = NULL;
        JVM owl = NULL;

    public:
        Ltl2DpaConverter();
        ~Ltl2DpaConverter();

        strix_aut::AutomatonTreeStructure toAutTreeStruct(
            std::string ltlFormula,
            std::vector<std::string> vecStrInputs,
            std::vector<std::string> vecStrOutputs,
            bool isSimplifyFormula = true
        );    
    };
}
