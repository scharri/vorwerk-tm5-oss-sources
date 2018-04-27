#!/bin/bash
iwpriv mlan0 drvdbg 0x20037
# change the mac address
ifconfig mlan0 hw ether 00:50:43:21:0e:08
ifconfig wfd0 hw ether 00:50:43:21:0e:08

iwpriv mlan0 deepsleep 0
#iwpriv wfd0 deepsleep 0
iwconfig mlan0 power off
iwconfig wfd0 power off

./wifidirectutl wfd0 wifidirect_config config/wifidirect.conf
iwpriv wfd0 bssrole 1
./uaputl.exe -i wfd0 sys_config config/uaputl_wifidirect.conf
./uaputl.exe -i wfd0 sys_cfg_cipher 8 8
./uaputl.exe -i wfd0 sys_cfg_eapol_gwk_hsk 2000 3
./uaputl.exe -i wfd0 sys_cfg_eapol_pwk_hsk 2000 3
iwpriv wfd0 bssrole 0
./wifidirectutl wfd0 wifidirect_mode 1
iwpriv wfd0 bssrole 1
./wifidirectutl wfd0 wifidirect_mode 2
./uaputl.exe -i wfd0 bss_start
