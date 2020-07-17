#
# compiler
#
CC					= clang++
CXXFLAGS			= -Wall -Wextra -std=c++11 -O3 -DNDEBUG -Wno-ignored-qualifiers

#
# JNI
#
JNI_INC				= -I/Library/Java/JavaVirtualMachines/graalvm-ce-java11-20.1.0/Contents/Home/include/ -I/Library/Java/JavaVirtualMachines/graalvm-ce-java11-20.1.0/Contents/Home/include/darwin/
JNI_LIB 			= -L/Library/Java/JavaVirtualMachines/graalvm-ce-java11-20.1.0/Contents/Home/lib/ -L/Library/Java/JavaVirtualMachines/graalvm-ce-java11-20.1.0/Contents/Home/lib/server -ljvm -ljawt -Wl,-rpath,/Library/Java/JavaVirtualMachines/graalvm-ce-java11-20.1.0/Contents/Home/lib/server/ -Wl,-rpath,/Library/Java/JavaVirtualMachines/graalvm-ce-java11-20.1.0/Contents/Home/lib/
#
# owl
#
OWL_INSTALL_PATH	= ../../kernel/lib/owl
OWL_JAR_LIB_NAME	= $(OWL_INSTALL_PATH)/build/libs/owl-19.06.03.jar
OWL_INCLUDE_PATH	= $(OWL_INSTALL_PATH)/src/main/headers
OWL_LIBRARY_PATH	= $(OWL_INSTALL_PATH)/src/main/cpp/library/build/


TARGET = owlTest
all: $(TARGET)

%.o:%.cpp
	$(CC) -c $(CXXFLAGS) -I$(OWL_INCLUDE_PATH) $(JNI_INC) -DOWL_JAR_LIB_NAME=\"$(OWL_JAR_LIB_NAME)\" $< -o $@

$(TARGET): $(TARGET).o
	$(CC) $(CXXFLAGS) -o $(TARGET).exe $(TARGET).o -L$(OWL_LIBRARY_PATH) $(JNI_LIB) -lowl

clean:
	rm -f *.o *.exe