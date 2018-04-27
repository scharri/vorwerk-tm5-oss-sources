#!/bin/sh

#-----------------------------------------------------------------------------
#
# File: hab4_pki_tree.sh
#
# Description: This script generates a basic HAB4 PKI tree for the Freescale
#              HAB code signing feature.  This script will generate the
#              following PKI tree:
#
#                                      CA Key
#                                      | | |
#                             -------- + | +---------------
#                            /           |                 \
#                         SRK1          SRK2       ...      SRKN
#                         / \            / \                / \
#                        /   \          /   \              /   \
#                   CSF1_1  IMG1_1  CSF2_1  IMG2_1 ... CSFN_1  IMGN_1
#
#              where: N can be 1 to 4.
#
#              Additional keys can be added to the tree separately.
#
#            (c) Freescale Semiconductor, Inc. 2011. All rights reserved.
#
#-----------------------------------------------------------------------------

printf "\n"
printf "    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n"
printf "    This script is a part of the Code signing tools for Freescale's\n"
printf "    High Assurance Boot.  It generates a basic PKI tree.  The PKI\n"
printf "    tree consists of one or more Super Root Keys (SRK), with each\n"
printf "    SRK having two subordinate keys: \n"
printf "        + a Command Sequence File (CSF) key \n"
printf "        + Image key. \n"
printf "    Additional keys can be added to the PKI tree but a separate \n"
printf "    script is available for this.  This this script assumes openssl\n"
printf "    is installed on your system and is included in your search \n"
printf "    path.  Finally, the private keys generated are password \n"
printf "    protectedwith the password provided by the file key_pass.txt.\n"
printf "    The format of the file is the password repeated twice:\n"
printf "        my_password\n"
printf "        my_password\n"
printf "    All private keys in the PKI tree are in PKCS #8 format will be\n"
printf "    protected by the same password.\n\n"
printf "    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n"

stty erase 

printf "Do you want to use an existing CA key (y/n)?:  \b"
read existing_ca
if [ $existing_ca = "y" ]
then
    printf "Enter CA key name:  \b"
    read ca_key
    printf "Enter CA certificate name:  \b"
    read ca_cert
fi

printf "Enter key length in bits for PKI tree:  \b"
read kl

# Confirm that a valid key length has been entered
case $kl in
    1024) ;;
    2048) ;;
    3072) ;;
    4096) ;;
    *)
        echo Invalid key length. Supported key lengths: 1024, 2048, 3072, 4096
	exit 1 ;; 
esac

printf "Enter PKI tree duration (years):  \b"
read duration

# Compute validity period
val_period=$((duration*365))

printf "How many Super Root Keys should be generated?  \b"
read num_srk

# Check that 0 < num_srk <= 4 (Max. number of SRKs)
if [ $num_srk -lt 1 ] || [ $num_srk -gt 4 ]
then
    echo The number of SRKs generated must be between 1 and 4
    exit 1
fi

# Check that the file "serial" is present, if not create it:
if [ ! -f serial ]
then
    echo "12345678" > serial
    echo "A default 'serial' file was created!"
fi

# Check that the file "key_pass.txt" is present, if not create it with default user/pwd:
if [ ! -f key_pass.txt ]
then
    echo "test" > key_pass.txt
    echo "test" >> key_pass.txt
    echo "A default file 'key_pass.txt' was created with password = test!"
fi

# The following is required otherwise OpenSSL complains
touch index.txt
echo "unique_subject = no" > index.txt.attr


if [ $existing_ca = "n" ]
then
    ca_key=./CA1_sha256_${kl}_65537_v3_ca_key
    ca_cert=../crts/CA1_sha256_${kl}_65537_v3_ca_crt

    # Generate CA key and certificate
    # -------------------------------
    echo
    echo +++++++++++++++++++++++++++++++++++++
    echo + Generating CA key and certificate +
    echo +++++++++++++++++++++++++++++++++++++
    echo
    openssl req -newkey rsa:${kl} -passout file:./key_pass.txt \
                -subj /CN=CA1_sha256_${kl}_65537_v3_ca/ \
                -x509 -extensions v3_ca \
                -keyout temp_ca.pem \
                -out ${ca_cert}.pem \
                -days ${val_period} -config ../ca/openssl.cnf

    # Generate CA key in PKCS #8 format - both PEM and DER
    openssl pkcs8 -passin file:./key_pass.txt -passout file:./key_pass.txt \
                  -topk8 -inform PEM -outform DER -v2 des3 \
                  -in temp_ca.pem \
                  -out ${ca_key}.der

    openssl pkcs8 -passin file:./key_pass.txt -passout file:./key_pass.txt \
                  -topk8 -inform PEM -outform PEM -v2 des3 \
                  -in temp_ca.pem \
                  -out ${ca_key}.pem

    # Convert CA Certificate to DER format
    openssl x509 -inform PEM -outform DER -in ${ca_cert}.pem -out ${ca_cert}.der

    # Cleanup
    \rm temp_ca.pem
fi

# Generate HAB  keys and certificates
# -----------------------------------
i=1;  # SRK Loop index
while [ $i -le $num_srk ]
do

    echo
    echo ++++++++++++++++++++++++++++++++++++++++
    echo + Generating SRK key and certificate $i +
    echo ++++++++++++++++++++++++++++++++++++++++
    echo

    # Generate SRK key
    openssl genrsa -des3 -passout file:./key_pass.txt -f4 \
                   -out ./temp_srk.pem ${kl}

    # Generate SRK certificate signing request
    openssl req -new -batch -passin file:./key_pass.txt \
                -subj /CN=SRK${i}_sha256_${kl}_65537_v3_ca/ \
                -key ./temp_srk.pem \
                -out ./temp_srk_req.pem

    # Generate SRK certificate (this is a CA cert)
    openssl ca -batch -passin file:./key_pass.txt \
               -md sha256 -outdir ./ \
               -in ./temp_srk_req.pem \
               -cert ${ca_cert}.pem \
               -keyfile ${ca_key}.pem \
               -extfile ../ca/v3_ca.cnf \
               -out ../crts/SRK${i}_sha256_${kl}_65537_v3_ca_crt.pem \
               -days ${val_period} \
               -config ../ca/openssl.cnf

    # Convert SRK Certificate to DER format
    openssl x509 -inform PEM -outform DER \
                 -in ../crts/SRK${i}_sha256_${kl}_65537_v3_ca_crt.pem \
                 -out ../crts/SRK${i}_sha256_${kl}_65537_v3_ca_crt.der

    # Generate SRK key in PKCS #8 format - both PEM and DER
    openssl pkcs8 -passin file:./key_pass.txt -passout file:./key_pass.txt \
                  -topk8 -inform PEM -outform DER -v2 des3 \
                  -in temp_srk.pem \
                  -out ./SRK${i}_sha256_${kl}_65537_v3_ca_key.der

    openssl pkcs8 -passin file:./key_pass.txt -passout file:./key_pass.txt \
                  -topk8 -inform PEM -outform PEM -v2 des3 \
                  -in temp_srk.pem \
                  -out ./SRK${i}_sha256_${kl}_65537_v3_ca_key.pem

    # Cleanup
    \rm ./temp_srk.pem ./temp_srk_req.pem


    echo
    echo ++++++++++++++++++++++++++++++++++++++++
    echo + Generating CSF key and certificate $i +
    echo ++++++++++++++++++++++++++++++++++++++++
    echo

    # Generate key
    openssl genrsa -des3 -passout file:./key_pass.txt \
                   -f4 -out ./temp_csf.pem ${kl}

    # Generate CSF certificate signing request
    openssl req -new -batch -passin file:./key_pass.txt \
                -subj /CN=CSF${i}_1_sha256_${kl}_65537_v3_usr/ \
                -key ./temp_csf.pem \
                -out ./temp_csf_req.pem 

    # Generate CSF certificate (this is a user cert)
    openssl ca -batch -md sha256 -outdir ./ \
               -passin file:./key_pass.txt \
               -in ./temp_csf_req.pem \
               -cert ../crts/SRK${i}_sha256_${kl}_65537_v3_ca_crt.pem \
               -keyfile ./SRK${i}_sha256_${kl}_65537_v3_ca_key.pem \
               -extfile ../ca/v3_usr.cnf \
               -out ../crts/CSF${i}_1_sha256_${kl}_65537_v3_usr_crt.pem \
               -days ${val_period} \
               -config ../ca/openssl.cnf

    # Convert CSF Certificate to DER format
    openssl x509 -inform PEM -outform DER \
                 -in ../crts/CSF${i}_1_sha256_${kl}_65537_v3_usr_crt.pem \
                 -out ../crts/CSF${i}_1_sha256_${kl}_65537_v3_usr_crt.der

    # Generate CSF key in PKCS #8 format - both PEM and DER
    openssl pkcs8 -passin file:./key_pass.txt -passout file:./key_pass.txt \
                  -topk8 -inform PEM -outform DER -v2 des3 \
                  -in temp_csf.pem \
                  -out ./CSF${i}_1_sha256_${kl}_65537_v3_usr_key.der

    openssl pkcs8 -passin file:./key_pass.txt -passout file:./key_pass.txt \
                  -topk8 -inform PEM -outform PEM -v2 des3 \
                  -in temp_csf.pem \
                  -out ./CSF${i}_1_sha256_${kl}_65537_v3_usr_key.pem

    # Cleanup
    \rm ./temp_csf.pem ./temp_csf_req.pem

    echo
    echo ++++++++++++++++++++++++++++++++++++++++
    echo + Generating IMG key and certificate $i +
    echo ++++++++++++++++++++++++++++++++++++++++
    echo

    # Generate key
    openssl genrsa -des3 -passout file:./key_pass.txt \
                   -f4 -out ./temp_img.pem ${kl}

    # Generate IMG certificate signing request
    openssl req -new -batch -passin file:./key_pass.txt \
                -subj /CN=IMG${i}_1_sha256_${kl}_65537_v3_usr/ \
                -key ./temp_img.pem \
                -out ./temp_img_req.pem

    openssl ca -batch -md sha256 -outdir ./ \
               -passin file:./key_pass.txt \
               -in ./temp_img_req.pem \
               -cert ../crts/SRK${i}_sha256_${kl}_65537_v3_ca_crt.pem \
               -keyfile ./SRK${i}_sha256_${kl}_65537_v3_ca_key.pem \
               -extfile ../ca/v3_usr.cnf \
               -out ../crts/IMG${i}_1_sha256_${kl}_65537_v3_usr_crt.pem \
               -days ${val_period} \
               -config ../ca/openssl.cnf

    # Convert IMG Certificate to DER format
    openssl x509 -inform PEM -outform DER \
                 -in ../crts/IMG${i}_1_sha256_${kl}_65537_v3_usr_crt.pem \
                 -out ../crts/IMG${i}_1_sha256_${kl}_65537_v3_usr_crt.der

    # Generate IMG key in PKCS #8 format - both PEM and DER
    openssl pkcs8 -passin file:./key_pass.txt -passout file:./key_pass.txt \
                  -topk8 -inform PEM -outform DER -v2 des3 \
                  -in temp_img.pem \
                  -out ./IMG${i}_1_sha256_${kl}_65537_v3_usr_key.der

    openssl pkcs8 -passin file:./key_pass.txt -passout file:./key_pass.txt \
                  -topk8 -inform PEM -outform PEM -v2 des3 \
                  -in temp_img.pem \
                  -out ./IMG${i}_1_sha256_${kl}_65537_v3_usr_key.pem

    # Cleanup
    \rm ./temp_img.pem ./temp_img_req.pem 


    i=$((i+1))
done
exit 0
