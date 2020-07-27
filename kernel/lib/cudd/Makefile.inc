#----
# TO DO: check the output of "uname -o" on HPUX and AIX
#----
OS= $(shell uname)

ifeq ($(OS),Gnu/Linux)
OS=Linux
endif
ifeq ($(OS),GNU/Linux)
OS=Linux
endif


LIB_DIR=$(PROJECT_ROOT)/lib
INC_DIR=$(PROJECT_ROOT)/include

ifeq ($(BUILD_TYPE),debug)
LIB_CUDD=libcudd.a
LINK_WITH_CUDD=-L$(LIB_DIR) -lcudd
else
LIB_CUDD=libcudd.a
LINK_WITH_CUDD=-L$(LIB_DIR) -lcudd
endif


#--------
# Update CXX_FLAGS based on BUILD_TYPE (debug or release)
#--------
ifeq ($(BUILD_TYPE),release)
override CXX_FLAGS+= -I$(INC_DIR) -fPIC -O -DNDEBUG
else
BUILD_TYPE=debug
override CXX_FLAGS+= -I$(INC_DIR) -fPIC -g
endif


LINK_FLAGS=	


#-----
# Determine:
#	- The commands used to build a library
#	- The C++ compiler
#-----
ifeq ($(OS),Linux)
#-----
# Linux
#-----
AR		= ar
AR_FLAGS	= -r
RANLIB		= /bin/true
RANLIB_FLAGS	= 
CXX		= g++
GCC		= gcc
else ifeq ($(OS),Cygwin)
#-----
# Cygwin: same as Linux
#-----
AR		= ar
AR_FLAGS	= -r
RANLIB		= /bin/true
RANLIB_FLAGS	= 
CXX		= g++
GCC		= gcc
else ifeq ($(OS),Darwin)
#-----
# MAC OS
#-----
AR		= ar
AR_FLAGS	= rc
RANLIB		= ranlib
RANLIB_FLAGS	= 
CXX		= g++
GCC		= gcc
else ifeq ($(OS),SunOS)
#-----
# Solaris
#-----
AR		= ar
AR_FLAGS	= cr
RANLIB		= ranlib
RANLIB_FLAGS	= 
CXX		= g++
GCC		= gcc
else ifeq ($(OS),HPUX)
#-----
# HPUX
#-----
AR		= ar
AR_FLAGS	= cr
RANLIB		= /bin/true
RANLIB_FLAGS	= 
CXX		= g++
GCC		= gcc
else ifeq ($(OS),Aix)
#-----
# AIX
#-----
AR		= ar
AR_FLAGS	= -r
RANLIB		= ranlib
RANLIB_FLAGS	= 
CXX		= g++
GCC		= gcc
endif


#--------
# Generic build rules
#--------
.SUFFIXES:		.c

.c.o:
	$(GCC) -c $(CXX_FLAGS) $<
	
	
.SUFFIXES:		.cc

.cc.o:
	$(CXX) -c $(CXX_FLAGS) -std=c++11 $<	
