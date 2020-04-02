FROM alpine:latest as build

COPY . /code

# Add build tools
RUN apk add --update alpine-sdk openssl-dev

# Build ff
RUN cd /code && \
    FF_OPTIMIZE=1 make build && \
    make install

FROM alpine:latest

# Install dependencies
RUN apk add --update openssl

COPY --from=build /usr/local/bin/ff /usr/local/bin/ff
COPY --from=build /usr/local/bin/ff_client /usr/local/bin/ff_client

ENTRYPOINT [ "ff" ]
