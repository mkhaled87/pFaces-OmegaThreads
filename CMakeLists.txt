cmake_minimum_required(VERSION 3.22...3.31)

# Name of the project
project(pFaces-OmegaThreads)

if(APPLE)
    set(CMAKE_MACOSX_RPATH ON)
endif()

# Include CMake configs for pFaces
if("$ENV{PFACES_SDK_ROOT}" STREQUAL "")
  message(FATAL_ERROR "Please make sure pFaces is installed correctly (PFACES_SDK_ROOT is not set).")
else()
  include($ENV{PFACES_SDK_ROOT}/pfaces_kernel.cmake)
endif()

# Needed libraries: ltl2dpa
add_subdirectory("./kernel-driver/lib/ltl2dpa/")

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
set(OWL_NATIVE_LIB_DIR "${PROJECT_SOURCE_DIR}/kernel-driver/lib/ltl2dpa/owl/build/native-library")

# Add kernel as shared lib
ADD_LIBRARY(${PFACES_KERNEL} SHARED
    ${PFACES_KERNEL_SOURCES}
)
target_include_directories(${PFACES_KERNEL} PUBLIC
    $ENV{PFACES_SDK_ROOT}/include
    $ENV{PFACES_SDK_ROOT}/include/cudd
    "./kernel-driver/lib/ltl2dpa/include/"
    "${OWL_NATIVE_LIB_DIR}/"
    "./kernel-driver/lib/ltl2dpa/owl/src/main/headers/"
)
target_link_directories(${PFACES_KERNEL} PUBLIC
    "${OWL_NATIVE_LIB_DIR}/"
)
if(MSVC)
  target_link_libraries(${PFACES_KERNEL} ltl2dpa "${OWL_NATIVE_LIB_DIR}/libowl.lib")
else()
  target_link_libraries(${PFACES_KERNEL} ltl2dpa owl)
endif()

# Colocate libowl next to omega.driver and set RPATH so pfaces can load it from any directory
if(APPLE)
  set(OWL_SHARED_LIB "${OWL_NATIVE_LIB_DIR}/libowl.dylib")
  set_target_properties(${PFACES_KERNEL} PROPERTIES
    BUILD_RPATH "@loader_path"
    INSTALL_RPATH "@loader_path"
    BUILD_WITH_INSTALL_RPATH TRUE
  )
elseif(UNIX)
  set(OWL_SHARED_LIB "${OWL_NATIVE_LIB_DIR}/libowl.so")
  set_target_properties(${PFACES_KERNEL} PROPERTIES
    BUILD_RPATH "\$ORIGIN"
    INSTALL_RPATH "\$ORIGIN"
    BUILD_WITH_INSTALL_RPATH TRUE
  )
elseif(WIN32)
  set(OWL_SHARED_LIB "${OWL_NATIVE_LIB_DIR}/libowl.dll")
endif()

# Include link configs for the kernel.
include($ENV{PFACES_SDK_ROOT}/pfaces_link.cmake)

if(DEFINED OWL_SHARED_LIB)
  add_custom_command(TARGET ${PFACES_KERNEL} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${OWL_SHARED_LIB}"
      "$<TARGET_FILE_DIR:${PFACES_KERNEL}>/"
    COMMENT "Copying ${OWL_SHARED_LIB} next to $<TARGET_FILE:${PFACES_KERNEL}>"
  )
endif()
