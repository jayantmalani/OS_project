#!/bin/bash

DST_IP=$1
HWADDR=$(echo $2|sed s/:/\ /g)
PORT=$3
IFACE=$4

if [ -z "$DST_IP" ] || [ -z "$HWADDR" ] || [ -z "$PORT" ] || [ -z "$IFACE" ]; then
	echo "usage: run.sh <destination IP> <destination MAC> <port> <router>"
	echo "[!] Note: if destination IP is not on the local network, <destination MAC> should be the MAC of the gateway"
	exit
fi

SRC_IP=$(ifconfig wlan0|awk {'print $2'}|head -2|tail -1|sed s/addr://g)
if [ -z "$SRC_IP" ]; then
	echo "[-] No network connection"
	exit
fi

echo "$SRC_IP"

echo "Updating IPTables..."
./updateIpTables $SRC_IP $DST_IP

echo "Attempting TCP Handshake..."
sudo ./rtt $DST_IP $HWADDR $PORT $IFACE
