#!/bin/sh
MODULE="scd"
DEVICE="scd"

# User must be root to proceed.
if [ "$(id -u)" != "0" ]; then
  echo "User must be root to load kernel modules"
  exit 1
fi

/sbin/rmmod $MODULE || exit 1

# Remove stale nodes
rm -f /dev/${DEVICE} /dev/${DEVICE}

