#!/bin/bash

BIND_IP=0.0.0.0
PORT=1234

set -e


while true;
do
    PAYLOADX=$(nc -lu -w0 $BIND_IP $PORT; echo x)
    PAYLOAD="${PAYLOADX%x}"
    HOST=$(echo "$PAYLOAD" | grep -E "^ *[Hh][Oo][Ss][Tt] *: *" | sed "s/^ *[Hh][Oo][Ss][Tt] *: *//g" | tr -d "\r\n ")
    
    RESPONSE=$(echo -n "$PAYLOAD" | nc $HOST 80)

    echo "$RESPONSE"
done

