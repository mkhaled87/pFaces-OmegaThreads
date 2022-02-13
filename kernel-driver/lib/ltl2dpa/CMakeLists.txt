cmake_minimum_required(VERSION 3.10)
project(StrixAutomataLibC)

# compile for C++11
set(CMAKE_CXX_STANDARD 17)
add_definitions("-fPIC")

# Declare rquired stuff
find_package(JNI REQUIRED)

add_library(ltl2dpa 
    ./src/StrixOwl.cpp 
    ./src/StrixAutomaton.cpp 
    ./src/StrixLTLParser.cpp 
    ./src/StrixLtl2Dpa.cpp
)

# Additional directories
target_include_directories(ltl2dpa PRIVATE
    ${JNI_INCLUDE_DIRS} 
    "${PROJECT_SOURCE_DIR}/owl/build/native-library/" 
    "${PROJECT_SOURCE_DIR}/owl/src/main/c/headers/" 
    "${PROJECT_SOURCE_DIR}/include/"
    "$ENV{PFACES_SDK_ROOT}/include/cudd/"
)
link_directories(
    "./owl/build/native-library/"
    "$ENV{PFACES_SDK_ROOT}/lib"
)

# link some other libs with our target lib
target_link_libraries(ltl2dpa PRIVATE
    owl pfaces-sdk
)

