SUMMARY = "libm2m"
DESCRIPTION = "C language library that helps embedded devices to handle M2MXML messages (worwerk's fork of existing library)"
LICENSE = "LGPLv2.1"
LIC_FILES_CHKSUM = "file://m2mbsm.c;beginline=1;endline=20;md5=db9f2c41b21384a9e2310f52586616df"

SRCREV = "${AUTOREV}"
SRCREV_FORMAT="libm2m"
inherit vorwerk_base

do_install_class-native(){
    install -d ${includedir}
    install -d ${includedir}/expat
    install -m 0644 *.h ${includedir}
    install -m 0644 expat/*.h ${includedir}/expat
}

BBCLASSEXTEND = "native nativesdk"
