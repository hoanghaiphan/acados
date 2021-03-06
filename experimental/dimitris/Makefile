OS := $(shell uname)

ACADOS=$(abspath ../../)
ACADOS_EXT=$(ACADOS)/external

OOQP=$(ACADOS_EXT)/OOQP
OOQPINCLUDEDIR=$(OOQP)/include
OOQPLIBDIR=$(OOQP)/lib
SORTOBJ = $(OOQP)/src/Utilities/sort.o

BLASFEO=$(ACADOS_EXT)/blasfeo
BLASFEOLIBDIR=$(BLASFEO)

HPMPC=$(ACADOS_EXT)/hpmpc
HPMPCLIBDIR=$(HPMPC)

QPOASES=$(ACADOS_EXT)/qpOASES
QPOASESINCLUDEDIR=$(QPOASES)/include
QPOASESLIBDIR=$(QPOASES)/bin

QPDUNES=$(ACADOS_EXT)/qpDUNES-dev
QPDUNESINCLUDEDIR=$(QPDUNES)/include
QPDUNESLIBDIR=$(QPDUNES)/build/lib

LDFLAGS =-L$(OOQPLIBDIR)
LDFLAGS+=-L$(BLASFEOLIBDIR)
LDFLAGS+=-L$(QPOASESLIBDIR)
LDFLAGS+=-L$(HPMPCLIBDIR)
LDFLAGS+=-L$(QPDUNESLIBDIR)

BLAS     = -lblas
MA27LIB  = $(OOQP)/libma27.a
ifeq ($(OS), Darwin)
	FLIBS = /usr/local/lib/gcc/6/libgfortran.dylib
else
	FLIBS = -lgfortran
endif

OOQP_SPARSE_LIBS = -looqpgensparse -looqpsparse  -looqpgondzio -looqpbase \
                 $(BLAS) $(MA27LIB) $(FLIBS)

LIBS= -lqpOASES_e -lhpmpc -lblasfeo -lqpdunes
LIBS+=$(OOQP_SPARSE_LIBS)
LIBS+=-lm

toy_memory_allocation: toy_memory_allocation.c
	gcc -g -O0 toy_memory_allocation.c -o toy_memory_allocation.exe
	valgrind --leak-check=full ./toy_memory_allocation.exe

qpdunes: test_qpdunes.c
	gcc -c -g -O0 ../../acados/utils/allocate_ocp_qp.c -o allocate_ocp_qp.o -I$(ACADOS) -I$(ACADOS_EXT)
	gcc -c -g -O0 ../../test/test_utils/read_ocp_qp_in.c -o read_ocp_qp_in.o -I$(ACADOS) -I$(ACADOS_EXT)
	gcc -c -g -O0 ../../acados/ocp_qp/ocp_qp_qpdunes.c -o ocp_qp_qpdunes.o -I$(ACADOS) -I$(ACADOS_EXT) -I$(QPDUNESINCLUDEDIR)
	gcc -c -g -O0 ../../acados/ocp_qp/ocp_qp_ooqp.c -o ocp_qp_ooqp.o -I$(OOQPINCLUDEDIR) -I$(ACADOS)  -I$(ACADOS_EXT)
	gcc -c -g -O0 ../../acados/utils/timing.c -o timing.o -I$(ACADOS)
	gcc -c -g -O0 test_qpdunes.c -o test_qpdunes.o -I$(ACADOS) -I$(ACADOS_EXT) -I$(QPDUNESINCLUDEDIR)
	g++ -g -O0 timing.o ocp_qp_qpdunes.o ocp_qp_ooqp.o allocate_ocp_qp.o read_ocp_qp_in.o test_qpdunes.o -o qpdunes.exe $(LDFLAGS) $(LIBS)
	./qpdunes.exe
	# valgrind --leak-check=full ./qpdunes.exe

ooqp: test_qp.c
	gcc -c -g -O0 ../../acados/ocp_qp/ocp_qp_ooqp.c -o ocp_qp_ooqp.o -I$(OOQPINCLUDEDIR) -I$(ACADOS)  -I$(ACADOS_EXT)
	gcc -c -g -O0 ../../acados/utils/timing.c -o timing.o -I$(ACADOS)
	gcc -c -g -O0 ../../acados/utils/allocate_ocp_qp.c -o allocate_ocp_qp.o -I$(ACADOS) -I$(ACADOS_EXT)
	gcc -c -g -O0 ../../test/test_utils/read_ocp_qp_in.c -o read_ocp_qp_in.o -I$(ACADOS) -I$(ACADOS_EXT)
	gcc -c -g -O0 -DSOLVER=1 test_qp.c -o test_qp.o -I$(ACADOS) -I$(ACADOS_EXT)
	g++ -g -O0 ocp_qp_ooqp.o timing.o allocate_ocp_qp.o read_ocp_qp_in.o test_qp.o $(SORTOBJ) -o test_qp.exe $(LDFLAGS) $(LIBS)
	valgrind --leak-check=full ./test_qp.exe
	# ./test_qp.exe

qpoases: test_qp.c
	gcc -c -g -O0 ../../acados/ocp_qp/ocp_qp_condensing_qpoases.c -o ocp_qp_condensing_qpoases.o -I$(ACADOS_EXT) -I$(ACADOS) -I$(QPOASESINCLUDEDIR)
	gcc -c -g -O0 ../../acados/ocp_qp/condensing.c -o ocp_qp_condensing.o -I$(ACADOS)
	gcc -c -g -O0 ../../acados/utils/allocate_ocp_qp.c -o allocate_ocp_qp.o -I$(ACADOS) -I$(ACADOS_EXT)
	gcc -c -g -O0 ../../test/test_utils/read_ocp_qp_in.c -o read_ocp_qp_in.o -I$(ACADOS) -I$(ACADOS_EXT)
	gcc -c -g -O0 -DSOLVER=2 test_qp.c -o test_qp.o -I$(ACADOS) -I$(ACADOS_EXT)
	gcc -g -O0 ocp_qp_condensing.o ocp_qp_condensing_qpoases.o allocate_ocp_qp.o read_ocp_qp_in.o test_qp.o -o test_qp.exe $(LDFLAGS) $(LIBS)
	valgrind --leak-check=full ./test_qp.exe

hpmpc: test_qp.c # TODO FIX FOR MAC!
	gcc -c -g -O0 ../../acados/ocp_qp/ocp_qp_hpmpc.c -o ocp_qp_hpmpc.o -I$(ACADOS_EXT) -I$(ACADOS)
	gcc -c -g -O0 ../../acados/utils/allocate_ocp_qp.c -o allocate_ocp_qp.o -I$(ACADOS) -I$(ACADOS_EXT)
	gcc -c -g -O0 ../../test/test_utils/read_ocp_qp_in.c -o read_ocp_qp_in.o -I$(ACADOS) -I$(ACADOS_EXT)
	gcc -c -g -O0 -DSOLVER=3 test_qp.c -o test_qp.o -I$(ACADOS) -I$(ACADOS_EXT)
	gcc -g -O0 ocp_qp_hpmpc.o allocate_ocp_qp.o read_ocp_qp_in.o test_qp.o -o test_qp.exe $(LDFLAGS) $(LIBS)
	./test_qp.exe

all: test_all_qp_solvers.c
	gcc -c -g -O0 ../../acados/utils/timing.c -o timing.o -I$(ACADOS)
	gcc -c -g -O0 ../../acados/ocp_qp/ocp_qp_ooqp.c -o ocp_qp_ooqp.o -I$(OOQPINCLUDEDIR) -I$(ACADOS)  -I$(ACADOS_EXT)
	gcc -c -g -O0 ../../acados/ocp_qp/ocp_qp_condensing_qpoases.c -o ocp_qp_condensing_qpoases.o -I$(ACADOS_EXT) -I$(ACADOS) -I$(QPOASESINCLUDEDIR)
	gcc -c -g -O0 ../../acados/ocp_qp/condensing.c -o ocp_qp_condensing.o -I$(ACADOS)
	gcc -c -g -O0 ../../acados/ocp_qp/ocp_qp_hpmpc.c -o ocp_qp_hpmpc.o -I$(ACADOS_EXT) -I$(ACADOS)
	gcc -c -g -O0 ../../acados/utils/allocate_ocp_qp.c -o allocate_ocp_qp.o -I$(ACADOS) -I$(ACADOS_EXT)
	gcc -c -g -O0 ../../test/test_utils/read_ocp_qp_in.c -o read_ocp_qp_in.o -I$(ACADOS) -I$(ACADOS_EXT)
	gcc -c -g -O0 test_all_qp_solvers.c -o test_all_qp_solvers.o -I$(ACADOS) -I$(ACADOS_EXT)
	g++ -g -O0 timing.o ocp_qp_ooqp.o ocp_qp_condensing.o ocp_qp_condensing_qpoases.o ocp_qp_hpmpc.o allocate_ocp_qp.o read_ocp_qp_in.o test_all_qp_solvers.o $(SORTOBJ) -o test_qp.exe $(LDFLAGS) $(LIBS)
	# valgrind --leak-check=full ./test_qp.exe
	./test_qp.exe

simple: simple.c
	gcc -g simple.c -o simple.exe -I$(ACADOS)
	./simple.exe

clean:
	rm *.exe *.o
