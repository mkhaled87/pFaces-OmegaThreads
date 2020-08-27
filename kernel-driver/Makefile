
#
# compiler
#
CC        		= g++
CXXFLAGS 		= -Wall -Wextra -std=c++11 -O3 -DNDEBUG -Wno-deprecated -Wno-ignored-attributes

#
# pfaces-sdk
# 
PFACES_SDK_INC = -I$(PFACES_SDK_ROOT)/include
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
KERNEL_SRCs  = $(KERNEL_NAME).cpp


.PHONY: all
	
all: kernel_driver

kernel_driver: libs
	for src in $(KERNEL_SRCs); do \
		$(CC) -c -o $${src}.o -fPIC $(CXXFLAGS) $(PFACES_SDK_INC) $${src}; \
	done
	$(CC) -shared -o combined_kernel.so $(PFACES_SDK_LNK) $(EXTRA_LNK) *.o
	cp combined_kernel.so ../kernel-pack/$(KERNEL_NAME).driver
	rm combined_kernel.so

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
