#!/bin/bash
export PATH=$PATH:./

# generate OtpInit.sb for burning OTP key, SRK hash and other bits to OTP bits permanently
# (-z -k, so 2 OTP keys will be possible)
otp_burner.py --input=bit_settings_tm41.txt --key=otp.key_Bunker2 --srk=../cst_tools/build/SRK_1_2_3_4.fuses --output=OtpInit.sb --hab --elftosb --print-otp
otp_convert.py bit_settings_tm41.dat

# verify bootstream with OTP key
sbtool -k otp.key_Bunker2 OtpInit.sb
sbtool -z OtpInit.sb

# convert to .bin/.srec
./mk_sbtoelf.sh
