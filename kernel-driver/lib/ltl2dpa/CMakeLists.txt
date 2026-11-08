cmake_minimum_required(VERSION 3.22...3.31)
project(StrixAutomataLibC)

set(OWL_NATIVE_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/owl/build/native-library")

# Add lib ltl2dpa
add_library(ltl2dpa
    ./src/StrixOwl.cpp
    ./src/StrixAutomaton.cpp
    ./src/StrixLTLParser.cpp
    ./src/StrixLtl2Dpa.cpp
)

# Include directories
target_include_directories(ltl2dpa PRIVATE
    "${OWL_NATIVE_LIB_DIR}/"
    "${CMAKE_CURRENT_SOURCE_DIR}/owl/src/main/headers/"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/"
    "$ENV{PFACES_SDK_ROOT}/include/cudd/"
)
target_link_directories(ltl2dpa PRIVATE
    "${OWL_NATIVE_LIB_DIR}/"
    "$ENV{PFACES_SDK_ROOT}/lib"
)

# link some other libs with our target lib
if(MSVC)
  target_link_libraries(ltl2dpa PRIVATE "${OWL_NATIVE_LIB_DIR}/libowl.lib")
else()
  target_link_libraries(ltl2dpa PRIVATE owl)
endif()
