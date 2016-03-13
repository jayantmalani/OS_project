#!/bin/bash

PORT=$(($RANDOM + 32768))

echo "Server running on port $PORT"
./server $PORT
