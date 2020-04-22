#!/bin/bash

FF_OPTIMIZE=0 CCFLAGS=-g make test_build

gdb build/tests $@
