# be sure to fetch local configuration in priority
FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://defconfig-debug   \
            file://rename_dhcp.patch \
            file://dhclient          \
            file://udhcpc-script     \
            file://nslookup_timeout.patch \
           "

BUSYBOX_CONFIG = "${@base_contains("IMAGE_FEATURES", "debug-tweaks", "defconfig-debug", "defconfig",d)}"

# Overload the default busybox configure which deals with  fragments
# and which automatically adds applets depending on distro features
# to stick basically with custom busybox configuration
do_configure () {
    bbnote "Using configuration file : ${BUSYBOX_CONFIG}"
    cp ${WORKDIR}/${BUSYBOX_CONFIG} ${S}/.config
    cml1_do_configure
}

do_install_append() {
    USE_BUSYBOX_DHCP=${@bb.utils.contains("IMAGE_FEATURES", "busybox-dhcp", "yes", "no",d)}
    if [ "$USE_BUSYBOX_DHCP" = "yes" ] ; then
      install -d  ${D}/sbin/
      install -d  ${D}/etc
      install -m 0755 ${WORKDIR}/dhclient ${D}/sbin/
      install -m 0755 ${WORKDIR}/udhcpc-script ${D}/etc
      rm -rf ${D}/etc/udhcpc.d
    fi
}
