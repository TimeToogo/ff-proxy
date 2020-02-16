# Usage:
# make			# compile binaries
# make test		# run tests
# make clean	# remove all binaries and objects

.PHONY: build test test_build

LD=gcc
CC=gcc

CC_FLAGS=-Wall 
LD_FLAGS=

build: server.o parser.o constants.o hash_table.o logging.o
	$(LD) $(LD_FLAGS) -o build/server $(wildcard build/obj/*.o)

parser.o: src/parser.c
	$(CC) $(CC_FLAGS) -c $< -o build/obj/$@

server.o: src/server.c
	$(CC) $(CC_FLAGS) -c $< -o build/obj/$@

constants.o: src/constants.c
	$(CC) $(CC_FLAGS) -c $< -o build/obj/$@

hash_table.o: src/hash_table.c
	$(CC) $(CC_FLAGS) -c $< -o build/obj/$@

logging.o: src/logging.c
	$(CC) $(CC_FLAGS) -c $< -o build/obj/$@

test_build: build
	$(CC) $(CC_FLAGS) -o build/tests $(filter-out build/obj/server.o, $(wildcard build/obj/*.o)) tests/include/*.c tests/run.c

test: test_build
	build/tests

valgrind: test_build
	valgrind --tool=memcheck --leak-check=full --num-callers=100 build/tests

clean:
	rm -f build/obj/*.o
	rm -f build/server
	rm -f build/tests