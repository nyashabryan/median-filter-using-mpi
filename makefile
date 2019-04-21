# This Makefile requires GNU make, which is called gmake on Solaris systems
#
# 'make'        : builds and runs the project
# 'make clean'  : remove build products

# where the executable program binary is placed:
PROG = bin/*
# Where the object files are placed:
OBJS = obj/*

# Which compiler to use, note for MPI a special purpose compiler is used
CC = mpic++

# Precompiled libraries to link in:
LDLIBS  = -L/usr/lib/openmpi/lib -L/usr/lib -lm -ljpeg -lmpi
# Included H files needed during compiling:
INCLUDE = -ITools -I/usr/lib/openmpi/include

.PHONY: clean Prac4 run
all:    clean Prac4 run

clean:
	rm -f -r $(PROG) $(OBJS)

Prac4:
	$(CC) $(INCLUDE) -c Prac4.cpp -o obj/Prac4.o
	$(CC) $(INCLUDE) -c Tools/JPEG.cpp -o obj/JPEG.o
	$(CC) $(INCLUDE) -c Tools/Timer.cpp -o obj/Timer.o
	$(CC) -o bin/Prac4 obj/Prac4.o obj/JPEG.o obj/Timer.o $(LDLIBS)

# you can type "make run" to execute the program in this case with a default of 5 nodes
run:
	mpirun -np 5 bin/Prac4

doxy: Prac4.cpp
	doxygen Prac4.doxy

