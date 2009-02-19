CC=gcc
CXX=g++
NVCC=nvcc 
CFLAGS=-Wall -g -O2 -std=c99 -I../tipsylib -I/usr/local/NVIDIA_CUDA_SDK/common/inc -I/usr/local/cuda/include -fnested-functions
CUFLAGS=-g -O3 -I/usr/local/NVIDIA_CUDA_SDK/common/inc
CXXFLAGS=$(CFLAGS)
LDFLAGS=-lm  -L../tipsylib -lcudart -L/usr/local/cuda/lib 
OBJS=main.o tree.o env.o mem.o prioq.o moments.c n2.o io.o io_ascii.o fmm.o
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
	$(CXX) -c $^ $(CXXFLAGS) -o $@ 
 
%.o : %.c 
	$(CC) -c $^ $(CFLAGS) -o $@ 

%.o : %.cu 
	$(NVCC) -c $^ $(CUFLAGS) -o $@ 

#$(CXXOBJS): %.o : %.cpp
#	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	-rm *.o
