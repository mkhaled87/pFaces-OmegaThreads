cmake_minimum_required(VERSION 3.31)

project(testProject)

# compile for C++17
set(CMAKE_CXX_STANDARD 14)
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

# add owl lib
include_directories("../../owl/build/native-library/" "../../owl/src/main/headers/")
link_directories("../../owl/build/native-library")

# add pFaces sdk to be able to use cudd from there
include_directories("$ENV{PFACES_SDK_ROOT}/include/")
include_directories("$ENV{PFACES_SDK_ROOT}/../lib/cudd/")
link_directories("$ENV{PFACES_SDK_ROOT}/lib")

# compile and link
add_executable(test test.cpp)
target_link_libraries(test ltl2dpa owl pfaces-sdk "-framework OpenCL")
