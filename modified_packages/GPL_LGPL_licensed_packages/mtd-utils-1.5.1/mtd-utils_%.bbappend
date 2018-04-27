# Force install in bin instead of sbin to mimic previous image
do_install() {
    oe_runmake install DESTDIR=${D} SBINDIR=${bindir}
    # create symlinks to replace busybox applets
    install -d ${D}/${sbindir}/
    ln -s ${bindir}/ubiupdatevol ${D}/${sbindir}/
    ln -s ${bindir}/flash_erase ${D}/${sbindir}/
    ln -s ${bindir}/flash_eraseall ${D}/${sbindir}/
    ln -s ${bindir}/nanddump ${D}/${sbindir}/
    ln -s ${bindir}/nandwrite ${D}/${sbindir}/
}

# Create a new mtd-utils package containing relevant files for vorwerk target
PACKAGES =+ " mtd-utils-vorwerk "
FILES_mtd-utils-vorwerk = "${bindir}/ubiupdatevol ${bindir}/flash_erase \
                           ${bindir}/flash_eraseall ${bindir}/nanddump \
                           ${bindir}/nandwrite ${sbindir}/"
