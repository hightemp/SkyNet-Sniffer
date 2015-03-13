#!/bin/sh
while true
    do python skynet_sniffer.py 2>&1 | sudo pi-display/anzeige
done
