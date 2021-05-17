cmake_minimum_required(VERSION 3.6)

# Check: supported OS
if(NOT APPLE AND NOT UNIX AND NOT WIN32)
    message(FATAL_ERROR "You are using not-supported OS.")
endif()

# check installation of pFaces/pFaces-SDK
if (WIN32)
find_program(PFACES_BINARY "pfaces.bat")
else()
find_program(PFACES_BINARY "pfaces")
endif()
if( (NOT PFACES_BINARY) OR ("$ENV{PFACES_SDK_ROOT}" STREQUAL "") OR (NOT EXISTS "$ENV{PFACES_SDK_ROOT}/include") OR (NOT EXISTS "$ENV{PFACES_SDK_ROOT}/lib"))
    message(FATAL_ERROR "Couln't find pFaces binary. Please make sure to insttall it correctly.")
else()
    file(READ "$ENV{PFACES_SDK_ROOT}/VERSION" PFACES_VERSION)
    string(SUBSTRING "${PFACES_VERSION}" 24 5 PFACES_VERSION)
    message("-- pFaces binary and SDK enviroment found (pFaces v${PFACES_VERSION}).")
endif()

# [Update Here] any additional subdirs
add_subdirectory("./kernel-driver/lib/ltl2dpa/")

# [Update Here] give a name to the project
project(pFaces-OmegaThreads)

# [Update Here] set kernel name and its sources
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

# [Update Here] Must use C++11 or later
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Check: Win -> MSVC
if(WIN32 AND NOT MSVC)
    message(FATAL_ERROR "We only support MSVC++ compiler in Windows.")
endif()

# fix CMake for Ubuntu
if(UNIX)
	set(CMAKE_PREFIX_PATH /usr/lib/x86_64-linux-gnu/cmake/)
endif()

# Needed packages
find_package(OpenCL REQUIRED)
find_package(cpprestsdk REQUIRED NAMES cpprestsdk cpprest)

# Generic CMake instructions to make the kernel
link_directories(
    $ENV{PFACES_SDK_ROOT}/lib
    "./kernel-driver/lib/ltl2dpa/owl/build/native-library/"
)
ADD_LIBRARY(${PFACES_KERNEL} SHARED
    ${PFACES_KERNEL_SOURCES}
)
target_include_directories(${PFACES_KERNEL} PUBLIC
    ${OpenCL_INCLUDE_DIR}
    $ENV{PFACES_SDK_ROOT}/include
    $ENV{PFACES_SDK_ROOT}/include/cudd
    "./kernel-driver/lib/ltl2dpa/include/"
    "./kernel-driver/lib/ltl2dpa/owl/build/native-library/"
    "./kernel-driver/lib/ltl2dpa/owl/src/main/headers/"
)
set_target_properties(${PFACES_KERNEL}
    PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${PFACES_KERNEL_OUT_PATH}"
    LIBRARY_OUTPUT_DIRECTORY_RELEASE "${PFACES_KERNEL_OUT_PATH}"
    LIBRARY_OUTPUT_DIRECTORY_DEBUG "${PFACES_KERNEL_OUT_PATH}"
    ARCHIVE_OUTPUT_DIRECTORY "${PFACES_KERNEL_OUT_PATH}"
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${PFACES_KERNEL_OUT_PATH}"
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${PFACES_KERNEL_OUT_PATH}"
    RUNTIME_OUTPUT_DIRECTORY "${PFACES_KERNEL_OUT_PATH}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${PFACES_KERNEL_OUT_PATH}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${PFACES_KERNEL_OUT_PATH}"
    SUFFIX ".driver"
    PREFIX ""
)
if (MSVC)
    set_target_properties(${PFACES_KERNEL} PROPERTIES LINK_FLAGS "/WHOLEARCHIVE:pfaces-sdk.lib")
    target_link_libraries(${PFACES_KERNEL} ${OpenCL_LIBRARY} cpprestsdk::cpprest ws2_32)
elseif (APPLE)
    set_target_properties(${PFACES_KERNEL} PROPERTIES LINK_FLAGS "-Wl,-all_load -lpfaces-sdk")
    target_link_libraries(${PFACES_KERNEL} ${OpenCL_LIBRARY} cpprestsdk::cpprest dl pthread ${OPENSSL_LIBRARIES} ltl2dpa owl)
else()
    set_target_properties(${PFACES_KERNEL} PROPERTIES LINK_FLAGS "-Wl,--whole-archive -lpfaces-sdk -Wl,--no-whole-archive")
    target_link_libraries(${PFACES_KERNEL} ${OpenCL_LIBRARY} cpprestsdk::cpprest pthread boost_system ltl2dpa owl)
endif()

# suppress some warnings
if(UNIX OR APPLE)
    target_compile_options(${PFACES_KERNEL} PRIVATE -Wno-deprecated -Wno-ignored-attributes -Wno-nullability-completeness-on-arrays)
elseif(MSVC)
    target_compile_options(${PFACES_KERNEL} PRIVATE /wd4267 /wd4996 /wd4101 /wd4390 /wd4244)
endif()


