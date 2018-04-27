#!/bin/bash
# expects openssl 1.0.0c in the path

# create pass phrase for protecting private keys
####sudo echo "test" > ./keys/key_pass.txt
####sudo echo "test" >> ./keys/key_pass.txt

# generate HAB code signing keys (only once)
####cd ./keys
####sudo ./hab4_pki_tree.sh
####cd ..
####exit

# generate SRK table (only once)
# need srktool in the path
####export PATH=$PATH:./linux
####srktool -h 4 -t ./build/SRK_1_2_3_4.table -e ./build/SRK_1_2_3_4.fuses -d sha256 -c ./crts/SRK1_sha256_2048_65537_v3_ca_crt.pem,./crts/SRK2_sha256_2048_65537_v3_ca_crt.pem,./crts/SRK3_sha256_2048_65537_v3_ca_crt.pem,./crts/SRK4_sha256_2048_65537_v3_ca_crt.pem
####exit

# generate OTP key (only once)
####cd ../otp_tools
####./keygen otp.key_Bunker2
####cd ../cst_tools
####exit
