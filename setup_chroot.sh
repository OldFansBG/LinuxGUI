#!/bin/bash
set -e  # Exit immediately if a command exits with a non-zero status
set -x  # Print commands and their arguments as they are executed

# Extensive logging function
log() {
    echo "[$(date +'%Y-%m-%d %H:%M:%S')] $*" | tee -a /output/chroot_setup.log
}

# Error handling function
handle_error() {
    log "ERROR: $*"
    exit 1
}

# Prepare directories
mkdir -p /mnt/iso
chmod 777 /mnt/iso
log "Prepared ISO mount directory"

# Create working directory
mkdir -p /output/custom_iso
log "Created custom ISO working directory"

# Extract ISO contents
cd /output/custom_iso
log "Mounting ISO file"
mount /mnt/iso/base.iso /mnt/iso || handle_error "Failed to mount ISO"

# Copy contents to working directory
cp -av /mnt/iso/* . || handle_error "Failed to copy ISO contents"
log "Copied ISO contents successfully"

# Unmount ISO
umount /mnt/iso

# Check if squashfs file exists
if [ ! -f casper/filesystem.squashfs ]; then
    handle_error "filesystem.squashfs not found in casper directory"
fi

# Extract squashfs
log "Starting squashfs extraction"
unsquashfs -d squashfs-root casper/filesystem.squashfs || handle_error "Squashfs extraction failed"

# Ensure mount points exist
mkdir -p squashfs-root/proc squashfs-root/sys squashfs-root/dev/pts
log "Created mount points for chroot environment"

# Setup chroot environment
mount -t proc none squashfs-root/proc || handle_error "Failed to mount proc"
mount -t sysfs none squashfs-root/sys || handle_error "Failed to mount sys"
mount -o bind /dev squashfs-root/dev || handle_error "Failed to bind /dev"
mount -o bind /dev/pts squashfs-root/dev/pts || handle_error "Failed to bind /dev/pts"

# Ensure resolv.conf is copied
cp /etc/resolv.conf squashfs-root/etc/ || handle_error "Failed to copy resolv.conf"

log "Chroot environment setup complete"

# Enter chroot and install packages
chroot squashfs-root /bin/bash -c "
set -e
set -x

# Install apt-utils to resolve debconf warning
apt-get update
DEBIAN_FRONTEND=noninteractive apt-get install -y apt-utils

# Update package lists with verbose output
apt-get update

# Install packages, forcing yes to handle any interactive prompts
DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends gedit dolphin

# Clean up
apt-get clean
rm -rf /tmp/*
" || handle_error "Chroot package installation failed"

log "Chroot package installation completed successfully"