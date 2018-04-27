#!/bin/bash
./uaputl.exe -i wfd0 bss_stop
sleep 1
iwpriv wfd0 bssrole 0
./wifidirectutl wfd0 wifidirect_mode 0

