CALL "MAKE"

...and the makefile will compile and run unit tests and delete both object
files and the executable.

Unit test executable will print every ran unit test, every failed assertion
and total number of tests ran. Failure may result in segfault or similar dead
end. Some tests use "fork" to test communication between nodes properly.

The produced and used object files are independent from the "obj" folder and
its contents.
