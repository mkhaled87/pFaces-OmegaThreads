cmake_minimum_required(VERSION 3.10)

project(testProject)

# compile for C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Debug)


# add OpenCL
find_package(OpenCL REQUIRED)
include_directories(${OpenCL_INCLUDE_DIRS})
link_directories(${OpenCL_LIBRARY})


# add boost
find_package (Boost 1.53 REQUIRED)
include_directories (${Boost_INCLUDE_DIRS})


# add ltl2dpa lib
include_directories("../../include")
link_directories("../../build")


# add pFaces sdk
include_directories("$ENV{PFACES_SDK_ROOT}/include")
link_directories("$ENV{PFACES_SDK_ROOT}/lib")


# add owl lib + cudd lib
include_directories("../../owl/build/native-library/" "../../owl/src/main/headers/")
include_directories("../../../cudd/")
link_directories("../../owl/build/native-library")
link_directories("../../../cudd/lib")


# compile and link
add_executable(test test.cpp)
target_link_libraries(test ltl2dpa owl cudd pfaces-sdk "-framework OpenCL")
