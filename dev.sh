#!/bin/bash

docker build -t ff-dev -f ./docker/dev.Dockerfile .

docker run --rm -it -v$PWD:/app ff-dev $@