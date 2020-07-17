cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 11)

# Define Executable
find_package(JNI REQUIRED)

# Check JNI
message("JNI Headers:")
message(${JNI_INCLUDE_DIRS})
message("JNI Libraries:")
message(${JNI_LIBRARIES})

# Additional directories
include_directories(${JNI_INCLUDE_DIRS} "../../kernel/lib/owl/src/main/headers")
link_directories("../../kernel/lib/owl/src/main/cpp/library/build/")

# a define to be passed to the source file
add_definitions(-DOWL_JAR_LIB_NAME=\"../../../kernel/lib/owl/build/libs/owl-19.06.03.jar\")

add_executable(owlTest owlTest.cpp)

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(DARWIN_FIX "/Library/Java/JavaVirtualMachines/graalvm-ce-java11-20.1.0/Contents/Home/lib/jli/libjli.dylib")
    target_link_libraries(owlTest ${DARWIN_FIX} ${JNI_LIBRARIES} owl)
else()
    target_link_libraries(owlTest owl ${JNI_LIBRARIES})
endif()
