cmake_minimum_required(VERSION 3.10)
project(StrixAutomataLibC)

# compile for C++11
set(CMAKE_CXX_STANDARD 17)

# Declare rquired stuff
#find_package(JNI REQUIRED)

# Check JNI
#message("JNI Headers:")
#message(${JNI_INCLUDE_DIRS})
#message("JNI Libraries:")
#message(${JNI_LIBRARIES})

# Additional directories
include_directories(${JNI_INCLUDE_DIRS} "./owl/build/native-library/" "./owl/src/main/headers/" "./include/")
link_directories("./owl/build/native-library/")

# add pFaces sdk to be able to use cudd from there
include_directories("$ENV{PFACES_SDK_ROOT}/include/cudd/")
link_directories("$ENV{PFACES_SDK_ROOT}/lib")

# Add the source file to a libraray
add_library(ltl2dpa ./src/StrixOwl.cpp ./src/StrixAutomaton.cpp ./src/StrixLTLParser.cpp ./src/StrixLtl2Dpa.cpp)

# link some other libs with our target lib
target_link_libraries(ltl2dpa owl pfaces-sdk)
