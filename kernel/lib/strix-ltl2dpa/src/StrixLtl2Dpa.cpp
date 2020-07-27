#include "StrixLtl2Dpa.h"

namespace strix_ltl2dpa{

    Ltl2DpaConverter::Ltl2DpaConverter(){
        if (graal_create_isolate(NULL, &isolate, &owl) != 0) {
            throw std::runtime_error("Could not create GraalVM.");
        }
    }

    Ltl2DpaConverter::~Ltl2DpaConverter(){
        graal_tear_down_isolate(owl);
    }

    strix_aut::AutomatonTreeStructure Ltl2DpaConverter::toAutTreeStruct(
        std::string ltlFormula,
        std::vector<std::string> vecStrInputs,
        std::vector<std::string> vecStrOutputs,
        bool isSimplifyFormula
    ){
        
        if (ltlFormula.empty()) {
            throw std::invalid_argument("No input formula was given.");
        }
        
        strix_ltl::LTLParser parser(owl, vecStrInputs, vecStrOutputs);
        strix_ltl::Specification spec = parser.parse_string(ltlFormula);

        return
        std::move(
            strix_aut::AutomatonTreeStructure::fromFormula(
                isolate, owl, spec.owl_object, 
                spec.inputs.size(), spec.outputs.size(), 
                isSimplifyFormula)
        );
    }
}