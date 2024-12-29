#!/bin/bash
apt-get update
apt-get install -y squashfs-tools xorriso isolinux
mkdir -p /output
chmod 777 /output