# Usage:
# make			# compile binaries
# make test		# run tests
# make clean	# remove all binaries and objects

.PHONY: build test

LD=gcc
CC=gcc

CC_FLAGS=-Wall
LD_FLAGS=

build: server.o parser.o
	$(LD) $(LD_FLAGS) -o build/server build/obj/server.o build/obj/parser.o

parser.o: src/parser.c
	$(CC) $(CC_FLAGS) -c $< -o build/obj/$@

server.o: src/server.c
	$(CC) $(CC_FLAGS) -c $< -o build/obj/$@

test: parser.o
	$(CC) $(CC_FLAGS) -o build/tests build/obj/parser.o tests/include/*.c tests/run.c
	build/tests

clean:
	rm -f build/obj/*.o
	rm -f build/server
	rm -f build/tests