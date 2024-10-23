# CC=gcc
# CFLAGS = -I/usr/mpi/gcc/openmpi-4.0.0rc5/include
# LDFLAGS = -L/usr/mpi/gcc/openmpi-4.0.0rc5/lib64 -lmpi
# MPIRUN = /usr/mpi/gcc/openmpi-4.0.0rc5/bin/mpirun

MPIRUN = mpirun
MPICC = mpic++

run: parcount
	$(MPIRUN) $(ARGS) parcount
	
parcount:parcount.cpp
	$(MPICC) -o parcount parcount.cpp

clean: 
	rm -f parcount
	