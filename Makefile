# Usage:
# make			# compile binaries
# make test		# run tests
# make clean	# remove all binaries and objects

.PHONY: build_check build build_server build_client test test_build

LD=gcc
CC=gcc

ifeq ($(FF_OPTIMIZE), 1)
OPTIMISE_FLAGS=-O3 -DFF_OPTIMIZE=1
else
OPTIMISE_FLAGS=
endif

CC_FLAGS=-Wall -Wextra -std=c99 -D_GNU_SOURCE $(OPTIMISE_FLAGS) $(CCFLAGS)
LD_FLAGS=$(LDFLAGS)
SERVER_LIBS=-lm -lssl -lcrypto -lpthread
CLIENT_LIBS=-lssl -lcrypto

ifeq ($(OPENSSL_SKIP_HOST_VALIDATION), 1)
        CC_FLAGS += -DOPENSSL_SKIP_HOST_VALIDATION
        OPENSSL_VER_OK = 1
else
        OPENSSL_VER	:= $(shell openssl version -v | cut -d " " -f 2)
        OPENSSL_MAJOR	:= $(shell echo $(OPENSSL_VER) | cut -d . -f 1)
        OPENSSL_MINOR	:= $(shell echo $(OPENSSL_VER) | cut -d . -f 2)
        OPENSSL_FIX	:= $(shell echo $(OPENSSL_VER) | cut -d . -f 3)
        OPENSSL_VER_OK	:= $(shell test $(OPENSSL_MAJOR) -ge 1 && test $(OPENSSL_MINOR) -ge 1 -o $(OPENSSL_FIX) \> 1z && echo 1)
endif

build_check:
# OpenSSL < 1.0.2 doesn't have direct support for hostname validation
ifneq "$(OPENSSL_VER_OK)" "1"
	@echo "***"
	@echo "*** OpenSSL too old (< 1.0.2)"
	@echo "***"
	@echo "*** If you are OK with skipping hostname validation then \
	please build with"
	@echo "***"
	@echo "*** make OPENSSL_SKIP_HOST_VALIDATION=1"
	@echo "***"
	@false
else
	$(MAKE) build
endif

build: build_server build_client

build_server: setup main.o config.o server.o request.o parser.o constants.o hash_table.o crypto.o http.o signals.o logging.o
	$(LD) $(LD_FLAGS) -o build/server $(wildcard build/obj/*.o) $(SERVER_LIBS)

build_client: setup client/main.o client/client.o client/config.o client/crypto.o config.o logging.o request.o crypto.o
	$(LD) $(LD_FLAGS) -o build/client $(wildcard build/obj/client/*.o) build/obj/config.o build/obj/logging.o build/obj/request.o build/obj/crypto.o $(CLIENT_LIBS)

setup: 
	mkdir -p build/obj/client

# Server

request.o: src/request.c
	$(CC) $(CC_FLAGS) -c $< -o build/obj/$@

parser.o: src/parser.c
	$(CC) $(CC_FLAGS) -c $< -o build/obj/$@

main.o: src/main.c
	$(CC) $(CC_FLAGS) -c $< -o build/obj/$@

config.o: src/config.c
	$(CC) $(CC_FLAGS) -c $< -o build/obj/$@

server.o: src/server.c
	$(CC) $(CC_FLAGS) -c $< -o build/obj/$@

constants.o: src/constants.c
	$(CC) $(CC_FLAGS) -c $< -o build/obj/$@

hash_table.o: src/hash_table.c
	$(CC) $(CC_FLAGS) -c $< -o build/obj/$@

logging.o: src/logging.c
	$(CC) $(CC_FLAGS) -c $< -o build/obj/$@

crypto.o: src/crypto.c
	$(CC) $(CC_FLAGS) -c $< -o build/obj/$@

http.o: src/http.c
	$(CC) $(CC_FLAGS) -c $< -o build/obj/$@

signals.o: src/signals.c
	$(CC) $(CC_FLAGS) -c $< -o build/obj/$@

# Client

client/main.o: client/c/main.c
	$(CC) $(CC_FLAGS) -c $< -o build/obj/$@

client/client.o: client/c/client.c
	$(CC) $(CC_FLAGS) -c $< -o build/obj/$@

client/config.o: client/c/config.c
	$(CC) $(CC_FLAGS) -c $< -o build/obj/$@

client/crypto.o: client/c/crypto.c
	$(CC) $(CC_FLAGS) -c $< -o build/obj/$@

# Tests

test_build: build
	$(CC) $(CC_FLAGS) $(LD_FLAGS) -o build/tests $(filter-out build/obj/main.o, $(wildcard build/obj/*.o)) \
									 $(filter-out build/obj/client/main.o, $(wildcard build/obj/client/*.o)) \
									 tests/include/*.c tests/run.c $(SERVER_LIBS)

test: test_build
	build/tests


fuzz: build
	clang -g -fsanitize=fuzzer,address \
		$(CC_FLAGS) $(LD_FLAGS) -o build/fuzzer $(filter-out build/obj/main.o, $(wildcard build/obj/*.o)) \
		$(filter-out build/obj/client/main.o, $(wildcard build/obj/client/*.o)) \
		tests/fuzzing/fuzzer.c $(SERVER_LIBS)
	build/fuzzer tests/fuzzing/corpus -artifact_prefix=tests/fuzzing/crashes/

valgrind: test_build
	valgrind --tool=memcheck --leak-check=full  --num-callers=100 --error-exitcode=1 build/tests

clean:
	rm -f build/obj/*.o
	rm -f build/obj/client/*.o
	rm -f build/server
	rm -f build/client
	rm -f build/tests

install:
	install -m +rx build/server /usr/local/bin/ff
	install -m +rx build/client /usr/local/bin/ff_client
