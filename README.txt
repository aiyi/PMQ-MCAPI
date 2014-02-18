HOW TO BUILD AND RUN THE EXAMPLE: make

Build and run example by calling "make". This will build and run example
application found directory "example", which contains a process sending scalar
to another and print about it. A cleaner is built and ran before the actual
application processes.

Example application is intended to provide the simplest possible example of
channel oriented communication between two processes.

Calling "make clean" will rebuild and rerun the cleaner and remove object files
from object directory.

If you wish to add more processes to your application, insert them to line
beginning with "PROCESSES".

If you make modifications to example, the make will try to run all procesesses
even if one of them does not compile.

FOR MORE DETAILS IN DETAILS.txt 
