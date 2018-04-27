cp ${1}/boot/updater*.bd ${TOP}
cd ${TOP}
sed -i "s,[^ *]zImage.*;,\tzImage=\"${1}/boot/zImage\";," updater.bd
sed -i "s,[^ *]sdram_prep.*;,\tsdram_prep=\"${1}/boot/boot_prep\";," updater.bd
sed -i "s,[^ *]linux_prep.*;,\tlinux_prep=\"${1}/boot/linux_prep\";," updater.bd
sed -i "s,[^ *]power_prep.*;,\tpower_prep=\"${1}/boot/power_prep\";," updater.bd

sed -i "s,[^ *]zImage.*;,\tzImage=\"${1}/boot/zImage\";," updater_ivt.bd
sed -i "s,[^ *]sdram_prep.*;,\tsdram_prep=\"${1}/boot/boot_prep\";," updater_ivt.bd
sed -i "s,[^ *]linux_prep.*;,\tlinux_prep=\"${1}/boot/linux_prep\";," updater_ivt.bd
sed -i "s,[^ *]power_prep.*;,\tpower_prep=\"${1}/boot/power_prep\";," updater_ivt.bd

elftosb -d -z -c updater.bd -o updater.sb
elftosb -d -z -f imx28 -c updater_ivt.bd -o updater_ivt.sb

rm -f updater*.bd

