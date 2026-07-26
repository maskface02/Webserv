#!/bin/bash

URL="http://127.0.0.1:8080"

for i in {1..100}; do
    curl --http1.1 --keepalive-time 60 -s -o /dev/null "$URL"
done
