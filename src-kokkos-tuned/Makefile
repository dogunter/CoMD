# Makefile for src-openmp

SHELL = /bin/bash

### CoMD can be built in either double or single precision and with or
### without MPI.  Select desired precision and MPI here.

# double precision (ON/OFF)
DOUBLE_PRECISION = ON
# MPI for parallel (ON/OFF)
DO_MPI = ON

# Kokkos - clean this up later
#KOKKOS_PATH=/users/dog/software/grizzly/kokkos-intel17.0.2-openmpi2.1.1
# On Trinitite
KOKKOS_PATH=/users/dog/software/trinitite/kokkos-intel18

KOKKOS_DEVICES = "OpenMP"
# "HSW" = haswell, "KNL" = knights landing, "KNC" = knights corner, "BDW" = Broadwell
# "SNB" = Sandybridge
# "Power8" = IBM Power 8 GPU systems, etc.
KOKKOS_ARCH = "KNL"

## Set executable name and add includes & libraries for MPI if needed.
CoMD_VARIANT = CoMD-kokkos-tuned
ifeq ($(DO_MPI), ON)
CoMD_VARIANT = CoMD-kokkos-tuned-mpi
INCLUDES += ${MPI_INCLUDE}
CXXFLAGS += -DDO_MPI
LDFLAGS += ${MPI_LIB}
endif
CoMD_EXE = ${BIN_DIR}/${CoMD_VARIANT}

# Intel flags are for Broadwell/Haswell CPUs
ifneq (,$(findstring Cuda,$(KOKKOS_DEVICES)))
CXX = ${KOKKOS_PATH}/bin/nvcc_wrapper
CXXFLAGS += -O3
LINK = ${CXX}
LINKFLAGS =
EXE = $(CoMD_EXE).cuda
KOKKOS_DEVICES = "Cuda,OpenMP"
KOKKOS_ARCH = "KNL,Kepler35"
else
CXX = CC
CXXFLAGS += -std=c++11 -qopenmp -xmic-avx512 -qopt-report=5 -DKOKKOS_OPT_RANGE_AGGRESSIVE_VECTORIZATION
#OPTFLAGS = -g -O3 -xCORE-AVX2
OPTFLAGS = -g -O3
LINK = ${CXX}
LINKFLAGS =
KOKKOS_DEVICES = "OpenMP"
KOKKOS_ARCH = "KNL"
EXE = $(CoMD_EXE).KNL.host
endif
#$(error $(OPTFLAGS))

INCLUDES = -I$(KOKKOS_PATH)/include

### If you need to specify include paths, library paths, or link flags
### for MPI, put them here.  Put both -L and -l switches into MPI_LIB.
MPI_LIB =
MPI_INCLUDE =

### A place to specify any other include or library switches your
### platform requires.
OTHER_LIB =
OTHER_INCLUDE =


#########################################
### Below here, it is pitch black.  
### You are likely to be eaten by a grue.
##########################################

# clear all suffixes
.SUFFIXES:
# list only those that we use 
.SUFFIXES: .c .o

.PHONY: DEFAULT clean distclean depend

BIN_DIR=../bin

# Check for double precision
# Using -DDOUBLE will break things for compiling C++ code using MPICH, which
# has a exter Datatype DOUBLE defined in a header file.
# I'm changing DOUBLE to COMD_DOUBLE, SINGLE to COMD_SINGLE in the src files.
# -dog
ifeq ($(DOUBLE_PRECISION), ON)
CFLAGS += -DCOMD_DOUBLE
CXXFLAGS += -DCOMD_DOUBLE
else
CFLAGS += -DCOMD_SINGLE
CXXFLAGS += -DCOMD_SINGLE
endif

LDFLAGS   += ${OTHER_LIB}
CXXFLAGS  += ${OPTFLAGS} ${INCLUDES} ${OTHER_INCLUDE}

SOURCES=$(wildcard *.cpp)
OBJECTS=$(SOURCES:.cpp=.o)

DEFAULT: ${EXE}

include $(KOKKOS_PATH)/Makefile.kokkos

%.o: %.cpp
	${CXX} $(CXXFLAGS) -c $< -o $@

${EXE}: ${BIN_DIR} $(KOKKOS_LINK_DEPENDS) $(KOKKOS_CPP_DEPENDS) CoMD_info.h ${OBJECTS} 
	$(LINK) $(KOKKOS_LDFLAGS) $(KOKKOS_CPP_FLAGS) $(CXXFLAGS) -o $(EXE) $(OBJECTS) $(LDFLAGS) $(KOKKOS_LIBS) 

CoMD_info.h: Makefile
	./generate_info_header ${CoMD_VARIANT} "$(CXX)" "$(CXXFLAGS)" "$(LDFLAGS)"

${BIN_DIR}:
	@if [ ! -d ${BIN_DIR} ]; then mkdir -p ${BIN_DIR} ; fi

clean:
	rm -f *.o CoMD_info.h .depend make.errors *.host *.cuda *.optrpt

distclean: clean
	rm -f ${EXE} .depend.bak
	rm -rf html latex

.depend: $(SOURCES)
	@touch .depend
	@$(MAKE) --no-print-directory depend

depend:
	@echo "Rebuilding dependencies..."
	@$(MAKE) CoMD_info.h
	@makedepend -f .depend -Y. --$(CXXFLAGS)-- $(SOURCES) 2> /dev/null


-include .depend
