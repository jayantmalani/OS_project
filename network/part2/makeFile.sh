#!/bin/bash

echo "[+] Creating dummy 512M file"
dd bs=4M count=128 if=/dev/urandom of=512M.file
