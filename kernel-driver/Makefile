
#
# compiler
#
CC        		= g++
CXXFLAGS 		= -Wall -Wextra -std=c++11 -Wno-deprecated -Wno-ignored-attributes -O3
#CXXFLAGS 		= -g -Wall -Wextra -std=c++11 -Wno-deprecated -Wno-ignored-attributes

#
# pfaces-sdk
# 
PFACES_SDK_INC = -I$(PFACES_SDK_ROOT)/include -I$(PFACES_SDK_ROOT)/include/cudd
EXTRA_LNK=-L/usr/local/opt/openssl/lib -lcpprest -lssl -lcrypto -lboost_system -ldl -lpthread

OS := $(shell uname)
ifeq ($(OS),Darwin)
PFACES_SDK_LNK = -L$(PFACES_SDK_ROOT)/lib -Wl,-all_load -lpfaces-sdk -Wl,-noall_load -framework OpenCL -L/usr/local/opt/openssl/lib -lcpprest -lssl -lcrypto -lboost_system -ldl -lpthread
EXTRA_LNK += -lboost_thread-mt -lboost_chrono-mt
CXXFLAGS += -Wno-ignored-attributes
else
PFACES_SDK_LNK = -L$(PFACES_SDK_ROOT)/lib -Wl,--whole-archive -lpfaces-sdk -Wl,--no-whole-archive
endif

#
# TODO: change the kernel source here to point to your c++ file
#
SUBDIRS = lib
KERNEL_NAME = omega
KERNEL_OBJs  = func_implement.o func_solve_pgame.o func_construct_pgame.o func_construct_symmodel.o func_discover_u_aps.o func_discover_x_aps.o omegaImplementation.o omegaParityGames.o omegaControlProblem.o omegaDpa.o omegaUtils.o omega.o
KERNEL_INCs = -Ilib/ltl2dpa/include -Ilib/ltl2dpa/owl/build/native-library -Ilib/ltl2dpa/owl/src/main/headers
ifeq ($(OS),Darwin)
KERNEL_LNKs = -Llib/ltl2dpa/build -lltl2dpa -Llib/ltl2dpa/owl/build/native-library -lowl
else
KERNEL_LNKs = -Llib/ltl2dpa/build -Wl,--whole-archive -lltl2dpa -Wl,--no-whole-archive -Llib/ltl2dpa/owl/build/native-library -Wl,--whole-archive -lowl -Wl,--no-whole-archive
endif

.PHONY: all
	
all: kernel_driver

kernel_driver: libs $(KERNEL_OBJs)
	$(CC) -shared -o combined_kernel.so $(PFACES_SDK_LNK) $(EXTRA_LNK) $(KERNEL_LNKs) $(KERNEL_OBJs)
	cp combined_kernel.so ../kernel-pack/$(KERNEL_NAME).driver
	rm combined_kernel.so

%.o: %.cpp *.h
	$(CC) -c -fPIC $(CXXFLAGS) $(PFACES_SDK_INC) $(KERNEL_INCs) $< -o $@

libs:
	cd lib; make; cd ..;

clean:
	rm -f -r Debug
	rm -f -r Release
	rm -f -r x64
	rm -f -r .vs
	rm -f *.o
	rm -f *.so
	for dir in $(SUBDIRS); do $(MAKE) clean -C $$dir $@; done
