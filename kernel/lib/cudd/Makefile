PROJECT_ROOT=.

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
#-----
ifeq ($(OS),Linux)
#-----
# Linux
#-----
AR		= ar
AR_FLAGS	= -r
RANLIB		= /bin/true
RANLIB_FLAGS	= 
else ifeq ($(OS),Cygwin)
#-----
# Cygwin: same as Linux
#-----
AR		= ar
AR_FLAGS	= -r
RANLIB		= /bin/true
RANLIB_FLAGS	= 
else ifeq ($(OS),Darwin)
#-----
# MAC OS
#-----
AR		= ar
AR_FLAGS	= rc
RANLIB		= ranlib
RANLIB_FLAGS	= 
else ifeq ($(OS),SunOS)
#-----
# Solaris
#-----
AR		= ar
AR_FLAGS	= cr
RANLIB		= ranlib
RANLIB_FLAGS	= 
else ifeq ($(OS),HPUX)
#-----
# HPUX
#-----
AR		= ar
AR_FLAGS	= cr
RANLIB		= /bin/true
RANLIB_FLAGS	= 
else ifeq ($(OS),Aix)
#-----
# AIX
#-----
AR		= ar
AR_FLAGS	= -r
RANLIB		= ranlib
RANLIB_FLAGS	= 
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


LIB_HDR_FILES	= \
		config.h \
		cstringstream.h \
		cudd.h \
		cuddInt.h \
		cuddObj.hh \
		dddmp.h \
		dddmpInt.h \
		epd.h \
		epdInt.h \
		mtr.h \
		mtrInt.h \
		st.h \
		util.h \
		cuddSymbolicset.hh

LIB_OBJ_FILES	= \
		cpu_stats.o \
		cpu_time.o \
		cstringstream.o \
		cuddAddAbs.o \
		cuddAddApply.o \
		cuddAddFind.o \
		cuddAddInv.o \
		cuddAddIte.o \
		cuddAddNeg.o \
		cuddAddWalsh.o \
		cuddAndAbs.o \
		cuddAnneal.o \
		cuddApa.o \
		cuddAPI.o \
		cuddApprox.o \
		cuddBddAbs.o \
		cuddBddCorr.o \
		cuddBddIte.o \
		cuddBridge.o \
		cuddCache.o \
		cuddCheck.o \
		cuddClip.o \
		cuddCof.o \
		cuddCompose.o \
		cuddDecomp.o \
		cuddEssent.o \
		cuddExact.o \
		cuddExport.o \
		cuddGenCof.o \
		cuddGenetic.o \
		cuddGroup.o \
		cuddHarwell.o \
		cuddInit.o \
		cuddInteract.o \
		cuddLCache.o \
		cuddLevelQ.o \
		cuddLinear.o \
		cuddLiteral.o \
		cuddMatMult.o \
		cuddObj.o \
		cuddPriority.o \
		cuddRead.o \
		cuddRef.o \
		cuddReorder.o \
		cuddSat.o \
		cuddSign.o \
		cuddSolve.o \
		cuddSplit.o \
		cuddSubsetHB.o \
		cuddSubsetSP.o \
		cuddSymmetry.o \
		cuddTable.o \
		cuddUtil.o \
		cuddWindow.o \
		cuddZddCount.o \
		cuddZddFuncs.o \
		cuddZddGroup.o \
		cuddZddIsop.o \
		cuddZddLin.o \
		cuddZddMisc.o \
		cuddZddPort.o \
		cuddZddReord.o \
		cuddZddSetop.o \
		cuddZddSymm.o \
		cuddZddUtil.o \
		datalimit.o \
		dddmpBinary.o \
		dddmpConvert.o \
		dddmpDbg.o \
		dddmpLoad.o \
		dddmpLoadCnf.o \
		dddmpNodeAdd.o \
		dddmpNodeBdd.o \
		dddmpNodeCnf.o \
		dddmpStoreAdd.o \
		dddmpStoreBdd.o \
		dddmpStoreCnf.o \
		dddmpStoreMisc.o \
		dddmpUtil.o \
		epd.o \
		mtrBasic.o \
		mtrGroup.o \
		pathsearch.o \
		pipefork.o \
		prtime.o \
		safe_mem.o \
		st.o \
		strsav.o \
		texpand.o \
		ucbqsort.o \
		cuddSymbolicSet.o

.PHONY: all clean

all:	RUN_SIZECHECK $(LIB_CUDD)	

RUN_SIZECHECK:
	 gcc sizecheck.c -o sizecheck
	 ./sizecheck

$(LIB_CUDD):	$(LIB_OBJ_FILES)
	$(AR) $(AR_FLAGS) $(LIB_CUDD) $(LIB_OBJ_FILES)
	$(RANLIB) $(RANLIB_FLAGS) $(LIB_CUDD)
	rm -r $(LIB_DIR)
	mkdir $(LIB_DIR)
	cp $(LIB_CUDD) $(LIB_DIR)

$(LIB_OBJ_FILES):	$(LIB_HDR_FILES)

clean:
	rm -f *.o
	rm -f libcudd*.a
	rm -f datasizes.h
	rm -f lib/*.*
	touch lib/.ignore
	rm -f -r Debug
	rm -f -r Release
	rm -f -r x64

