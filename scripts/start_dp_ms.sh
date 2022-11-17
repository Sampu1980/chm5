#!/bin/sh -x

# Since we need to support NSA upgrade, and BCM SDK upgrade is SA.  We need
# to run two version of chm6_dp_ms. Depending on the FPGA scratch pad setting
# this script will launch the correct version of
# chm6_dp_ms or chm6_dp_prev_rel_ms

bootmagicnumber=acbd
retcode=0

monitor_file=/home/bin/._monitor.tmp
rm -rf $monitor_file

echo "BoardInfo is: $BoardInfo"  >> $monitor_file
if [ $BoardInfo = sim ]; then
    echo "CHM6 SIM " >> $monitor_file
    echo "Load chm6_dp_ms " >> $monitor_file
    /home/bin/chm6_dp_ms
    exit $?
fi

echo "CHM6 HW " >> $monitor_file
bootmagic=`memtool md -w 0xa001ffb4-0xa001ffb5 | awk '{print $2}'`
# Wait here until BoardInitMs got a chance to set the SDK Version Number in
# FPGA scratch pad.
echo "bootmagic $bootmagic" >> $monitor_file
echo "bootmagicnumber $bootmagicnumber" >> $monitor_file

while [ $bootmagic != $bootmagicnumber ]
do
    echo "inloop bootmagic $bootmagic" >> $monitor_file
    echo "inloop bootmagicnumber $bootmagicnumber" >> $monitor_file
    sleep 5
    bootmagic=`memtool md -w 0xa001ffb4-0xa001ffb5 | awk '{print $2}'`
done

echo "after while loop bootmagic $bootmagic" >> $monitor_file

# check FPGA scratch pad for BCM retimer version.
bcmversion=`memtool md -b 0xa001ffb0-0xa001ffb0 | awk '{print $2}'`
echo "bcmversion $bcmversion" >> $monitor_file
if [ "$bcmversion" = 56 ]; then
    echo "Load chm6_dp_ms with new SDK" >> $monitor_file
    /home/bin/chm6_dp_ms
    retcode=$?
else
    echo "Load chm6_dp_prev_rel_ms with old SDK" >> $monitor_file
    /home/bin/chm6_dp_prev_rel_ms
    retcode=$?
fi

exit $retcode
