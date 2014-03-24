#INSERT PROCESSES HERE IN THE ORDER THEY ARE LAUNCHED. SUPPORTS ARGUMENTS.
PROCESSES = nice -n 11 ./esender.exe & nice -n 11 ./ereceiver.exe

#INSERT BUILD RULES FOR PROCESSES HERE IF SUCH IS NEEDED
PBUILD = make -f makefile.mcapi; $(ESENDER); $(ERECEIVER)
#build rules for individual processes
ESENDER = $(CC) -o esender.exe example/example_sender.c $(ODIR)/*.o -I$(IDIR) -lrt
ERECEIVER = $(CC) -o ereceiver.exe example/example_receiver.c $(ODIR)/*.o -I$(IDIR) -lrt

#compiler used
CC=gcc

#name of the cleaner executable
CNAME = cleaner

#folders of most files except executable
IDIR=include
ODIR=obj
SDIR=src

#below shell command is used to compile the cleaner
CRUN = make -f makefile.cleaner

#(re)creates the obj folder, (re)creates cleaner, then it is ran and then the apps
make:
	$(CRUN); \
	mkdir -p $(ODIR); \
	$(PBUILD); \
	$(PROCESSES); \
	wait $!; \
	./$(CNAME)

.PHONY: clean

#make clean would cause cleaner compile and run
clean:
	$(CRUN); \
	make -f makefile.mcapi clean; 
