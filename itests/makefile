#INSERT PROCESSES HERE IN THE ORDER THEY ARE LAUNCHED. SUPPORTS ARGUMENTS.
PROCESSES = nice -n 11 ./ITcyan.exe & nice -n 11 ./ITmag.exe & nice -n 10 ./ITgreen.exe & nice -n 10 ./ITyellow.exe & nice -n 10 ./ITred.exe 32 & nice -n 10 ./ITblue.exe 32

#INSERT BUILD RULES FOR PROCESSES HERE IF SUCH IS NEEDED
PBUILD = cd ..; make -f makefile.mcapi; cd $(ITDIR); make ITcyan; make ITmag; make ITgreen; make ITblue; make ITred; make ITyellow

#compiler used
CC=gcc

#flags: includes, unit tests, posix message queue, cmath, posix thread
CFLAGS=-I$(IDIR) -lrt -lm -pthread

#folders of most files except executable
IDIR=../include
ODIR=../obj
SDIR=../src
ITDIR=itests

#below shell command is used to compile and run the cleaner
CRUN = cd ..; make -f makefile.cleaner; cd $(ITDIR)

#rule to create integration tests: every target beginning IT is seen as such
IT%:
	$(CC) -o $@.exe $@.c $(ODIR)/*.o $(CFLAGS)

#(re)creates cleaner, then it is ran and then the apps. apps are also removed
make:
	$(CRUN); \
    mkdir -p $(ODIR); \
	$(PBUILD); \
    $(PROCESSES); \
    wait $!; \
    rm -f *.exe

.PHONY: clean

#make clean would cause cleaner compile and run
clean:
	$(CRUN)
