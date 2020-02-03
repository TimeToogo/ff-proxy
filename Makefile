# Usage:
# make			# compile binaries
# make test		# run tests
# make clean	# remove all binaries and objects

.PHONY: build test clean

build:
	gcc src/server.c -o build/server

test: build
	gcc tests/include/*.c tests/run.c -o build/tests
	build/tests

clean:
	rm -f build/server
	rm -f build/tests