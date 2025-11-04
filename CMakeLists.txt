cmake_minimum_required(VERSION 4.1)

# Name of the project
project(pFaces-OmegaThreads)

# Include CMake configs for pFaces
if("$ENV{PFACES_SDK_ROOT}" STREQUAL "")
  message(FATAL_ERROR "Please make sure pFaces is installed correctly.")
else()
  include($ENV{PFACES_SDK_ROOT}/pfaces_kernel.cmake)
endif()

# Needed libraries: ltl2dpa
add_subdirectory("./kernel-driver/lib/ltl2dpa/")

# Needed libraries: boost
find_package(Boost 1.53 REQUIRED)

# Kernel and its sources
set(PFACES_KERNEL omega)
set(PFACES_KERNEL_SOURCES 
    ${PROJECT_SOURCE_DIR}/kernel-driver/func_implement.cpp
    ${PROJECT_SOURCE_DIR}/kernel-driver/func_solve_pgame.cpp
    ${PROJECT_SOURCE_DIR}/kernel-driver/func_construct_pgame.cpp
    ${PROJECT_SOURCE_DIR}/kernel-driver/func_construct_symmodel.cpp
    ${PROJECT_SOURCE_DIR}/kernel-driver/func_discover_u_aps.cpp
    ${PROJECT_SOURCE_DIR}/kernel-driver/func_discover_x_aps.cpp
    ${PROJECT_SOURCE_DIR}/kernel-driver/omegaImplementation.cpp
    ${PROJECT_SOURCE_DIR}/kernel-driver/omegaParityGames.cpp
    ${PROJECT_SOURCE_DIR}/kernel-driver/omegaControlProblem.cpp
    ${PROJECT_SOURCE_DIR}/kernel-driver/omegaDpa.cpp
    ${PROJECT_SOURCE_DIR}/kernel-driver/omegaUtils.cpp
    ${PROJECT_SOURCE_DIR}/kernel-driver/omega.cpp
)
set(PFACES_KERNEL_OUT_PATH "${PROJECT_SOURCE_DIR}/kernel-pack")


# Add kernel as shared lib
ADD_LIBRARY(${PFACES_KERNEL} SHARED
    ${PFACES_KERNEL_SOURCES}
)
target_include_directories(${PFACES_KERNEL} PUBLIC
    $ENV{PFACES_SDK_ROOT}/include
    $ENV{PFACES_SDK_ROOT}/include/cudd
    "./kernel-driver/lib/ltl2dpa/include/"
    "./kernel-driver/lib/ltl2dpa/owl/build/native-library/"
    "./kernel-driver/lib/ltl2dpa/owl/src/main/headers/"
    ${Boost_INCLUDE_DIRS}
)
target_link_directories(${PFACES_KERNEL} PUBLIC
    "./kernel-driver/lib/ltl2dpa/owl/build/native-library/"
)
if(MSVC)
  target_link_libraries(${PFACES_KERNEL} ltl2dpa)
else()
  target_link_libraries(${PFACES_KERNEL} ltl2dpa owl)
endif()

# Include link configs for the kernel.
include($ENV{PFACES_SDK_ROOT}/pfaces_link.cmake)
