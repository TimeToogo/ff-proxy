#!/bin/bash

docker build -t ff-dev -f ./docker/dev.Dockerfile .

docker run -e FF_OPTIMIZE --rm -it -v$PWD:/app ff-dev $@