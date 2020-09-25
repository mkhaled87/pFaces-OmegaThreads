cmake_minimum_required(VERSION 3.10)

project(testProject)

# compile for C++11
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Debug)

# add boost
find_package (Boost 1.53 REQUIRED)
include_directories (${Boost_INCLUDE_DIRS})

# Additional directories: the ltl2dpa lib
include_directories("../../include")

# Additional directories: the owl lib + cudd lib
include_directories("../../owl/build/native-library/" "../../owl/src/main/headers/")

# link libs: the ltl2dpa lib + strix
link_directories("../../build")
link_directories("../../owl/build/native-library")

# add pFaces sdk to be able to use cudd from there
include_directories("$ENV{PFACES_SDK_ROOT}/include/")
include_directories("$ENV{PFACES_SDK_ROOT}/../lib/cudd/")
link_directories("$ENV{PFACES_SDK_ROOT}/lib")

# compile and link
add_executable(test test.cpp)
target_link_libraries(test ltl2dpa owl pfaces-sdk)
