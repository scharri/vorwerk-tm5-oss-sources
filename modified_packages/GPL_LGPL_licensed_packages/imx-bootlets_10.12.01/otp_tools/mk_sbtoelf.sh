#!/bin/bash
export PATH=$PATH:/opt/freescale/usr/local/gcc-4.4.4-glibc-2.11.1-multilib-1.0/arm-fsl-linux-gnueabi/bin

# convert .sb to .elf
./sbtoelf -d -o OtpInit -z OtpInit.sb
mv OtpInit____.0.elf OtpInit.elf

# convert .elf to .srec
arm-fsl-linux-gnueabi-objcopy -O srec OtpInit.elf OtpInit.srec

# convert .elf to .bin
arm-fsl-linux-gnueabi-objcopy -O binary OtpInit.elf OtpInit.bin
