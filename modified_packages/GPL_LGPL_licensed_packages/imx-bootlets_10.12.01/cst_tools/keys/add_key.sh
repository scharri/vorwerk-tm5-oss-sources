#!/bin/sh

#-----------------------------------------------------------------------------
#
# File: add_key.sh
#
# Description: This script adds a key to an existing HAB PKI tree to be used
#              with the HAB code signing feature.  Both the private key and
#              corresponding certificate are generated.  This script can 
#              generate new SRKs, CSF keys and Images keys for HAB3 or HAB4.
#              This script is not intended for generating CA root keys.
#
#            (c) Freescale Semiconductor, Inc. 2011. All rights reserved.
#
#-----------------------------------------------------------------------------

stty erase 

printf "Enter new key name (e.g. SRK5):  \b"
read key_name

printf "Enter new key length in bits:  \b"
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

printf "Enter certificate duration (years):  \b"
read duration

# Compute validity period
val_period=$((duration*365))

printf "Which version of HAB do you want to generate the key for (3/4)?:  \b"
read hab_ver
case $hab_ver in
    3) ;;
    4) ;;
    *) 
            echo Error - HAB version selected must be either 3 or 4     
   exit 1
esac

# ---------------- Add SRK Key and Certificate -------------------
printf "Is this an SRK key?:  \b"
read srk

if [ $srk = "y" ]
then

    printf "Enter CA signing key name:  \b"
    read ca_key
    printf "Enter CA signing certificate name:  \b"
    read ca_cert

    # Generate key
    openssl genrsa -des3 -passout file:./key_pass.txt -f4 \
                   -out ./${key_name}_sha256_${kl}_65537_v3_ca_key.pem ${kl}

    # Generate SRK certificate signing request
    openssl req -new -batch -passin file:./key_pass.txt \
                -subj /CN=${key_name}_sha256_${kl}_65537_v3_ca/ \
                -key ./${key_name}_sha256_${kl}_65537_v3_ca_key.pem \
                -out ./${key_name}_sha256_${kl}_65537_v3_ca_req.pem

    # Generate SRK certificate (this is a CA cert)
    openssl ca -batch -passin file:./key_pass.txt \
               -md sha256 -outdir ./ \
               -in ./${key_name}_sha256_${kl}_65537_v3_ca_req.pem \
               -cert ${ca_cert} \
               -keyfile ${ca_key} \
               -extfile ../ca/v3_ca.cnf \
               -out ../crts/${key_name}_sha256_${kl}_65537_v3_ca_crt.pem \
               -days ${val_period} \
               -config ../ca/openssl.cnf

    if [ $hab_ver = "3" ]
    then
        # Generate WTLS certificate, ...
        ../linux/x5092wtls \
            -c ../crts/${key_name}_sha256_${kl}_65537_v3_ca_crt.pem \
            -k ./${ca_key} \
            -w ../crts/${key_name}_sha256_${kl}_65537_wtls.crt \
            -p ./key_pass.txt

        # C file and add corresponding fuse information to srktool_wtls.txt
        ../linux/srktool \
            -h 3 \
            -c ../crts/${key_name}_sha256_${kl}_65537_wtls.crt \
            -o >> ../crts/srk_wtls_cert_efuse_info.txt    
    fi

# Clean up
\rm -f *_req.pem
exit 0
fi


# ---------------- Add CSF Key and Certificate -------------------
printf "Is this a CSF key?:  \b"
read csf

if [ $csf = "y" ]
then
    printf "Enter SRK signing key name:  \b"
    read srk_key
    printf "Enter SRK signing certificate name:  \b"
    read srk_cert

    if [ $hab_ver = "4" ]
    then
        # Generate key
        openssl genrsa -des3 -passout file:./key_pass.txt \
            -f4 -out ./${key_name}_sha256_${kl}_65537_v3_usr_key.pem ${kl}

        # Generate CSF certificate signing request
        openssl req -new -batch -passin file:./key_pass.txt \
                -subj /CN=${key_name}_sha256_${kl}_65537_v3_usr/ \
                -key ./${key_name}_sha256_${kl}_65537_v3_usr_key.pem \
                -out ./${key_name}_sha256_${kl}_65537_v3_usr_req.pem

        # Generate CSF certificate (this is a user cert)
        openssl ca -batch -md sha256 -outdir ./ \
               -passin file:./key_pass.txt \
               -in ./${key_name}_sha256_${kl}_65537_v3_usr_req.pem \
               -cert ${srk_cert} \
               -keyfile ./${srk_key} \
               -extfile ../ca/v3_usr.cnf \
               -out ../crts/${key_file}_sha256_${kl}_65537_v3_usr_crt.pem \
               -days ${val_period} \
               -config ../ca/openssl.cnf
      
    # HAB3 
    else        
        # Generate key
        openssl genrsa -des3 -passout file:./key_pass.txt \
            -f4 -out ./${key_name}_sha256_${kl}_65537_v3_ca_key.pem ${kl}

        # Generate CSF certificate signing request
        openssl req -new -batch -passin file:./key_pass.txt \
                -subj /CN=${key_name}_sha256_${kl}_65537_v3_ca/ \
                -key ./${key_name}_sha256_${kl}_65537_v3_ca_key.pem \
                -out ./${key_name}_sha256_${kl}_65537_v3_ca_req.pem

        # Generate CSF certificate (this is a user cert)
        openssl ca -batch -md sha256 -outdir ./ \
                -passin file:./key_pass.txt \
                -in ./${key_name}_sha256_${kl}_65537_v3_ca_req.pem \
                -cert ${srk_cert} \
                -keyfile ./${srk_key} \
                -extfile ../ca/v3_ca.cnf \
                -out ../crts/${key_name}_sha256_${kl}_65537_v3_ca_crt.pem \
                -days ${val_period} \
                -config ../ca/openssl.cnf

        # Generate WTLS certificate, ...
        ../linux/x5092wtls \
            -c ../crts/${key_name}_sha256_${kl}_65537_v3_ca_crt.pem \
            -k ./${srk_key} \
            -w ../crts/${key_name}_sha256_${kl}_65537_wtls.crt \
            -p ./key_pass.txt
    fi

# Clean up
\rm -f *_req.pem
exit 0
fi

# ---------------- Add IMG Key and Certificate -------------------

# Generate key
openssl genrsa -des3 -passout file:./key_pass.txt \
               -f4 -out ./${key_name}_sha256_${kl}_65537_v3_usr_key.pem ${kl}


# Generate IMG certificate signing request
openssl req -new -batch -passin file:./key_pass.txt \
            -subj /CN=${key_name}_sha256_${kl}_65537_v3_usr/ \
            -key ./${key_name}_sha256_${kl}_65537_v3_usr_key.pem \
            -out ./${key_name}_sha256_${kl}_65537_v3_usr_req.pem


if [ $hab_ver = "4" ]
then
    printf "Enter SRK signing key name:  \b"
    read srk_key
    printf "Enter SRK signing certificate name:  \b"
    read srk_cert

    openssl ca -batch -md sha256 -outdir ./ \
               -passin file:./key_pass.txt \
               -in ./${key_name}_sha256_${kl}_65537_v3_usr_req.pem \
               -cert ${srk_cert} \
               -keyfile ${srk_key} \
               -extfile ../ca/v3_usr.cnf \
               -out ../crts/${key_name}_sha256_${kl}_65537_v3_usr_crt.pem \
               -days ${val_period} \
               -config ../ca/openssl.cnf
else
    printf "Enter CSF signing key name:  \b"
    read csf_key
    printf "Enter CSF signing certificate name:  \b"
    read csf_cert

    openssl ca -batch -md sha256 -outdir ./ \
               -passin file:./key_pass.txt \
               -in ./${key_name}_sha256_${kl}_65537_v3_usr_req.pem \
               -cert ${csf_cert} \
               -keyfile ${csf_key} \
               -extfile ../ca/v3_usr.cnf \
               -out ../crts/${key_name}_sha256_${kl}_65537_v3_usr_crt.pem \
               -days ${val_period} \
               -config ../ca/openssl.cnf

    # Generate WTLS certificate, ...
    ../linux/x5092wtls \
        -c ../crts/${key_name}_sha256_${kl}_65537_v3_usr_crt.pem \
        -k ./${csf_key} \
        -w ../crts/${key_name}_sha256_${kl}_65537_wtls.crt \
        -p ./key_pass.txt

fi
