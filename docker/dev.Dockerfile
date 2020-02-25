FROM ubuntu:latest

RUN apt-get update

RUN apt-get install -y git-core valgrind g++ nano libssl-dev build-essential

WORKDIR /app
