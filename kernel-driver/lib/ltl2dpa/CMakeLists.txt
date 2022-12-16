cmake_minimum_required(VERSION 3.10)
project(StrixAutomataLibC)

# JNI
set(JAVA_AWT_LIBRARY NotNeeded)
set(JAVA_JVM_LIBRARY NotNeeded)
set(JAVA_INCLUDE_PATH2 NotNeeded)
set(JAVA_AWT_INCLUDE_PATH NotNeeded)
find_package(JNI REQUIRED)

# Add lib ltl2dpa
add_library(ltl2dpa 
    ./src/StrixOwl.cpp 
    ./src/StrixAutomaton.cpp 
    ./src/StrixLTLParser.cpp 
    ./src/StrixLtl2Dpa.cpp
)

# Include directories
target_include_directories(ltl2dpa PRIVATE
    ${JNI_INCLUDE_DIRS} 
    "${PROJECT_SOURCE_DIR}/owl/build/native-library/" 
    "${PROJECT_SOURCE_DIR}/owl/src/main/headers/" 
    "${PROJECT_SOURCE_DIR}/include/"
    "$ENV{PFACES_SDK_ROOT}/include/cudd/"
)
link_directories(
    "./owl/build/native-library/"
    "$ENV{PFACES_SDK_ROOT}/lib"
)

# link some other libs with our target lib
if(MSVC)
  target_link_libraries(ltl2dpa PRIVATE "libowl.lib" pfaces-sdk)
else()
  target_link_libraries(ltl2dpa PRIVATE owl pfaces-sdk)
endif()