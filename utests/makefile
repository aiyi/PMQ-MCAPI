#compiler used
CC=gcc
#flags: includes, unit tests, posix message queue, include unit-test spesific stuff
CFLAGS=-I$(IDIR) -I$(UTDIR) -lrt -DUTEST
#name of the unittester executable
UEXE = utest

#folders of various stuff needed
IDIR=../include
ODIR=.
SDIR=../src
UTDIR=.

#the header files
_DEPS = mca.h mcapi.h mcapi_config.h endpoint.h node.h endpointdef.h endpointlist.h channel.h utester.h pmq_layer.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

#unit test suites
_UTESTS = suite_node.h suite_endpoint.h suite_msg.h suite_packet_con_open.h suite_packet_tx_rx.h suite_scalar_con_open.h suite_scalar_tx_rx.h
UTESTS = $(patsubst %,$(UTDIR)/%,$(_UTESTS))

#object files
_OBJ = endpoint.o msg.o node.o packet.o endpointdef.o channel.o scalar.o utester.o cleaner.o pmq_layer.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

#rule to create object files
$(ODIR)/%.o: $(SDIR)/%.c $(DEPS) $(UTESTS)
	$(CC) -c -o $@ $< $(CFLAGS)

#rule to create unit test runner. object files are also cleaned away as arent really needed
make: $(OBJ)
	$(CC) -o $(UEXE) $(OBJ) $(CFLAGS); \
	rm -f $(_OBJ); \
	./$(UEXE); \
	rm $(UEXE)
