CALL "MAKE"

...and the makefile will compile and run integration tests. Also the cleaner
program is compiled and ran. Some integration test executables are dependent on
flags -lm and -pthread.

WARNING: Calling "make clean" will remove the .o-files from folder ../obj

Each integration test is identified by specific color used in their terminal
prints. Each is described below:

ITred: After start up-messaging with yellow, sends sin values as scalar based
communication to yellow.

ITblue: Sends cos values as scalar based communication to yellow.

ITyellow: After start up-messaging with red, receives scalar values from red
and blue, sums and buffers them and then sends to green as packet.

ITgreen: After receiving a message from yellow, receives a packet from it.

ITcyan: One thread sends random packets to magenta and another receives and
inspects them from magenta.

Itmag: One thread receives packets from cyan and another increments each byte
by one and send them back cyan as a packet.
