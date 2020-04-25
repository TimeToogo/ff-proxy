#!/bin/bash

docker build -t ff-dev -f ./docker/dev.Dockerfile .

docker run --cap-add SYS_PTRACE -e FF_OPTIMIZE --rm -it -v$PWD:/app ff-dev $@