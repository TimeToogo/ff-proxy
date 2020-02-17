#!/bin/bash

docker build -t ff .

docker run --rm -it -v$PWD:/app ff $@