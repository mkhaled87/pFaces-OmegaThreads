cmake_minimum_required(VERSION 3.10)
project(StrixAutomataLibCpp)

# compile for C++11
set(CMAKE_CXX_STANDARD 11)

# Declare rquired stuff
find_package(JNI REQUIRED)

# Check JNI
message("JNI Headers:")
message(${JNI_INCLUDE_DIRS})
message("JNI Libraries:")
message(${JNI_LIBRARIES})

# Additional directories
include_directories(${JNI_INCLUDE_DIRS} "./owl/include" "./include")
link_directories("./owl/lib/")

# Defines to be passed to the source file
add_definitions(-DOWL_LIB_PATH=\"../owl/java/\")

# Add the lib
add_library(StrixAutomaton ./src/Automaton.cpp)

# special measures for MacOs
if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(LIB_JLI "/Library/Java/JavaVirtualMachines/openjdk-11.0.2.jdk/Contents/Home/lib/jli/libjli.dylib")
    target_link_libraries(StrixAutomaton ${LIB_JLI} ${JNI_LIBRARIES} owl)
else()
    target_link_libraries(StrixAutomaton ${${JNI_LIBRARIES} owl)
endif()
