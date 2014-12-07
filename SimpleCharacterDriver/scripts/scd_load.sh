#!/bin/bash

INSMOD=/sbin/insmod;
MODULE="scd"
DEVICE="scd"
MODE="666"
GROUP=""
MAJOR="0"
MINOR="0"

# User must be root to proceed.
if [ "$(id -u)" != "0" ]; then
  echo "User must be root to load kernel modules"
  exit 1
fi

# Get group.
STAFF=`cat /etc/group | grep staff | cut -d: -f 1`
if [ -n "$STAFF" ]; then
    GROUP="staff"
else
    GROUP="wheel"
fi

show_help() {
cat << EOF
		Usage: ${0##*/} [-h] [-M MajorNum] [-m MinorNum]
		Will load a module with predefined MajorNum and MinorNum.
       		-h          Display this help and exit
       		-M MajorNum Set desired MajorNum.
       		-m MinorNum Set desired MinorNum.
EOF
}

while getopts "hM:m:" opt; do
    case "$opt" in
    h)
        show_help
        exit 0
        ;;
    M)  TMP=$OPTARG
		if [[ $TMP =~ ^-?[0-9]+$ ]] 
		then MAJOR=$TMP
		else echo "Major is not a number" && show_help && exit 0
		fi
        ;;
    m)  TMP=$OPTARG
		if [[ $TMP =~ ^-?[0-9]+$ ]] 
		then MINOR=$TMP
		else echo "Minor is not a number" && show_help && exit 0
		fi
        ;;
    esac
done

if [[ $MAJOR -eq 0 ]]
then $INSMOD ../$MODULE.ko || exit 1
else $INSMOD ../$MODULE.ko scd_major=$MAJOR scd_minor=$MINOR || exit 1
fi

# get major number if it is determined dynamicaly.
if [[ $MAJOR -eq 0 ]]
then MAJOR=`cat /proc/devices | grep $MODULE | cut -d' ' -f 1`
fi

rm -f /dev/${DEVICE}
mknod /dev/${DEVICE} c $MAJOR 0
#ln -sf ${DEVICE}0 /dev/${DEVICE}
chgrp $GROUP /dev/${DEVICE} 
chmod $MODE  /dev/${DEVICE}

