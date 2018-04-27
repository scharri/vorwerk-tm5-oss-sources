DESCRIPTION = "Rotates, compresses, removes and mails system log files"
SECTION = "console/utils"
HOMEPAGE = "https://fedorahosted.org/releases/l/o/logrotate"
LICENSE = "GPLv2"
PR = "r0"

DEPENDS="coreutils popt"

LIC_FILES_CHKSUM = "file://COPYING;md5=18810669f13b87348459e611d31ab760"

SRC_URI = "https://fedorahosted.org/releases/l/o/logrotate/logrotate-${PV}.tar.gz \
           file://act-as-mv-when-rotate.patch \
           file://disable-check-different-filesystems.patch \
	   file://s-flag.patch \
            "
SRC_URI[md5sum] = "df67c8bda9139131d919931da443794d"
SRC_URI[sha256sum] = "0776bf491171edbcc3ba577751fc912e721e99b834c14251df8109fd3bfa1977"

EXTRA_OEMAKE = "CC='${CC}'"

do_install(){
    oe_runmake install DESTDIR=${D} PREFIX=${D} MANDIR=${mandir} BINDIR=${sbindir}
}

do_install_append(){
    mkdir -p ${D}${sysconfdir}/logrotate.d
    mkdir -p ${D}${localstatedir}/lib
    touch ${D}${localstatedir}/lib/logrotate.status
}
