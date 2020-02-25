# FROM alpine:latest as build

# COPY . /code

# # Add build tools
# RUN apk add --update alpine-sdk openssl-dev

# # Build ff
# RUN cd /code && \
#     make build && \
#     make install

FROM alpine:latest

COPY . /code/

# Install dependencies
RUN apk add --update alpine-sdk openssl-dev openssl

# Build ff
RUN cd /code && \
    make build && \
    make install

ENTRYPOINT [ "ff" ]
