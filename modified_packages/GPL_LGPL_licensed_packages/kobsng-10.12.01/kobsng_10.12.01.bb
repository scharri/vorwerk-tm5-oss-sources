SUMMARY = "kobs-ng"
DESCRIPTION = "Writes i.MX233-style boot streams to a NAND Flash medium."
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=393a5ca445f6965873eca0259a17f833"

# Explicit dependency upon kernel to use the specific headers (not the ones packaged in libc)
DEPENDS += "virtual/kernel"

SRCREV = "${AUTOREV}"

S = "${WORKDIR}/${PN}"

inherit autotools

EXTRA_OEMAKE = "CFLAGS='-I${STAGING_KERNEL_DIR}/include'"
