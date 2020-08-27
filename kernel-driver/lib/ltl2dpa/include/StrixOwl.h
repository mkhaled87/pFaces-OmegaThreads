#pragma once

#include "owltypes.h"
#include "libowl.h"

typedef graal_isolatethread_t* JVM;
typedef graal_isolate_t* Isolate;
typedef void* OwlFormula;
typedef void* OwlAutomaton;
typedef void* OwlDecomposedDPA;
typedef void* OwlStructure;
typedef void* OwlReference;

namespace strix_owl {
    std::string owl_object_to_string(JVM owl, void* object);
}