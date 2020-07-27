cmake_minimum_required(VERSION 3.10)

project(testProject)

# compile for C++11
set(CMAKE_CXX_STANDARD 17)

# Additional directories: the ltl2dpa lib
include_directories("../include")

# Additional directories: the owl lib + cudd lib
include_directories("../owl/build/native-library/" "../owl/src/main/headers/" "../../cudd/")

# link libs: the ltl2dpa lib + strix + cudd lib
link_directories("../build")
link_directories("../owl/build/native-library")
link_directories("../../cudd/lib")

# compile and link
add_executable(test test.cpp)
target_link_libraries(test StrixAutomaton owl cudd)
