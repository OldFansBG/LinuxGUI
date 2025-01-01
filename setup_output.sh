#!/bin/bash 

# Create working directory if it doesn't exist 
mkdir -p ~/custom_iso

 # Update package lists and install necessary tools 
apt-get update
apt-get install -y squashfs-tools xorriso isolinux sudo syslinux-utils genisoimage