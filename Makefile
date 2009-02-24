CC=gcc
CXX=g++
NVCC=nvcc 
PROFILE=-pg
OPTIMIZE=-O2
C99="-std=c99"
COMMON_FLAGS=-Wall -g $(OPTIMIZE) -I../tipsylib -I/usr/local/NVIDIA_CUDA_SDK/common/inc -I/usr/local/cuda/include -Winline $(PROFILE)
CFLAGS=$(COMMON_FLAGS) $(C99) -fnested-functions
CUFLAGS=-g -O3 -I/usr/local/NVIDIA_CUDA_SDK/common/inc
CXXFLAGS=$(COMMON_FLAGS)
LDFLAGS=-lm  -L../tipsylib -lcudart -L/usr/local/cuda/lib  $(PROFILE)
OBJS=main.o tree.o env.o mem.o prioq.o moments.c n2.o io.o io_ascii.o fmm.o sighandler.o \
	rungs.o
CXXOBJS=io_tipsy.o
CUDAOBJS=nvidia.o
LIBTIPSY=../tipsylib/libtipsy.a

INCLUDES=env.h cello_types.h io_tipsy.h macros.h mem.h prioq.h tree.h moments.h

cello: $(OBJS) $(CXXOBJS) $(CUDAOBJS) $(INCLUDES)
	$(NVCC) $(LDFLAGS) $(OBJS) $(CXXOBJS) $(CUDAOBJS) $(LIBTIPSY) -o cello

wineff: wineff.o
	$(CXX) wineff.o -o wineff

interactdia: interactdia.o
	$(CC) interactdia.o -lm -ljpeg -lpng -o interactdia

isort: isort.o
	$(CC) isort.o -o isort

isort2: isort2.o
	$(CC) isort2.o -o isort2

ishuffle: ishuffle.o
	$(CC) ishuffle.o -o ishuffle

#$(OBJS): %.o : %.c
#	$(CC) $(CFLAGS) -c $< -o $@

%.o : %.cpp 
	@echo Compiling $^
	@$(CXX) -c $^ $(CXXFLAGS) -o $@ 
 
%.o : %.c 
	@echo Compiling $^
	@$(CC) -c $^ $(CFLAGS) -o $@ 

%.o : %.cu 
	@echo Compiling $^
	@$(NVCC) -c $^ $(CUFLAGS) -o $@ 

#$(CXXOBJS): %.o : %.cpp
#	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	-rm *.o
