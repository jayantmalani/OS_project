#!/bin/bash

R_IP=$1
R_HWADDR=$(echo $2|sed s/:/\ /g)
R_PORT=$3
IFACE=$4

rm *.log

if [ -z "$R_IP" ] || [ -z "$R_HWADDR" ] || [ -z "$R_PORT" ] || [ -z "$IFACE" ]; then
	echo "usage: run.sh <destination IP> <destination MAC> <port> <router>"
	echo "[!] Note: if destination IP is not on the local network, <destination MAC> should be the MAC of the gateway"
	exit
fi

SRC_IP=$(ifconfig wlan0|awk {'print $2'}|head -2|tail -1|sed s/addr://g)
if [ -z "$SRC_IP" ]; then
	echo "[-] No network connection"
	exit
fi

echo "[+] Updating IPTables..."
sudo iptables -A OUTPUT -p tcp --tcp-flags RST RST -s $SRC_IP -d $R_IP -j DROP

for i in `seq 1 2`; do
	echo "[*] Running trial $i/10"
	echo "[+] Testing loopback"
	echo "  [+] Gathering ping stats..."
	ping -c10 127.0.0.1|awk {'print $7'}|head -11|tail -10|sed s/time=//g >> loopback_ping.log
	sudo ./rtt 127.0.0.1 00 00 00 00 00 00 8000 lo >> loopback.log
	echo "[+] Testing remote host"
	echo "  [+] Gathering ping stats..."
	ping -c10 $R_IP|awk {'print $7'}|head -11|tail -10|sed s/time=//g >> remote_ping.log
	sudo ./rtt $R_IP $R_HWADDR $R_PORT $IFACE >> remote.log
done
