PC Windows:

Open the .sln file in the m2mxml.sln folder with VisualStudio 2008 and build the "Release" build.

PC Linux:

Build dynamic library:
gcc -c -m32 -fPIC  m2mxmlin.c m2mxmlou.c m2mmem.c m2mbsm.c expat/explight.c m2mitoa.c
gcc -shared -m32 -Wl,-soname,libm2m.so -o libm2m.so m2mxmlin.o m2mxmlou.o m2mmem.o m2mbsm.o explight.o m2mitoa.o 
cp libm2m.so ../../../deps/lib/linux/pc/m2m

Building test main against library:
gcc testmain.c -m32 -o testmain -L../../../deps/lib/linux/pc/m2m -lm2m

Running the test main:
LD_LIBRARY_PATH=../../../deps/lib/linux/pc/m2m
export LD_LIBRARY_PATH
./testmain unittestin.txt >unittestout_from_binary.txt



imx28:

Build dynamic library:
/opt/freescale/usr/local/gcc-4.4.4-glibc-2.11.1-multilib-1.0/arm-fsl-linux-gnueabi/bin/arm-linux-cc -c -fPIC  m2mxmlin.c m2mxmlou.c m2mmem.c m2mbsm.c expat/explight.c m2mitoa.c
/opt/freescale/usr/local/gcc-4.4.4-glibc-2.11.1-multilib-1.0/arm-fsl-linux-gnueabi/bin/arm-linux-cc -shared -Wl,-soname,libm2m.so -o libm2m.so m2mxmlin.o m2mxmlou.o m2mmem.o m2mbsm.o explight.o m2mitoa.o 
cp libm2m.so ../../../deps/lib/linux/imx28/m2m

Building test main against library:
/opt/freescale/usr/local/gcc-4.4.4-glibc-2.11.1-multilib-1.0/arm-fsl-linux-gnueabi/bin/arm-linux-cc testmain.c -o testmain -L../../../deps/lib/linux/imx28/m2m -lm2m

Running the test main:
LD_LIBRARY_PATH=../../../deps/lib/linux/imx28/m2m
export LD_LIBRARY_PATH
./testmain unittestin.txt >unittestout_from_binary.txt



OSX:

Build universal (fat) static library:
gcc -c -arch i386 -arch x86_64 m2mxmlin.c m2mxmlou.c m2mmem.c m2mbsm.c expat/explight.c m2mitoa.c
ar rcs libm2m.a m2mxmlin.o m2mxmlou.o m2mmem.o m2mbsm.o explight.o m2mitoa.o

