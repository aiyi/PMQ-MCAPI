HOW TO BUILD AND RUN THE EXAMPLE: ./launch

Get started by calling "./launch" in the folder of this file. This will build
and run the example application found in directory "example", which contains a
process sending scalar to another and print about it. A cleaner is built and
ran before the actual application processes.

Example application is intended to provide the simplest possible example of
channel oriented communication between two processes.

If you wish to add more processes to your application, insert them to line
beginning with "PROCESSES".

If you make modifications to example, the script will try to run all processes
even if one of them does not compile. Executing the script again will kill
previously launched processes.

Sleeps are inserted to example code, so that the flow of the communication
is better illustrated.

MORE DETAILS IN DETAILS.txt
