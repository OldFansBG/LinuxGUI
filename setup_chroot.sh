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

# Wait for the ISO file to be copied
log "Waiting for ISO file to be copied..."
while [ ! -f /mnt/iso/base.iso ]; do
    sleep 1
done
log "ISO file found: /mnt/iso/base.iso"

# Wait for the ISO to be fully accessible
log "Waiting for ISO file to be fully accessible..."
while ! ls -l /mnt/iso/base.iso > /dev/null 2>&1; do
    sleep 1
done
log "ISO file is fully accessible."

# Create a writable directory for ISO contents
mkdir -p /output/iso_contents
chmod 777 /output/iso_contents
log "Created writable directory for ISO contents"

# Mount the ISO file to a temporary read-only directory
mkdir -p /mnt/iso_temp
log "Mounting ISO file to temporary directory"
mount /mnt/iso/base.iso /mnt/iso_temp || handle_error "Failed to mount ISO"

# Wait for the mount to stabilize
log "Waiting for mount to stabilize..."
sleep 5

# Copy contents to the writable directory
log "Copying ISO contents to writable directory"
for file in /mnt/iso_temp/*; do
    log "Copying $file"
    if ! cp -av "$file" /output/iso_contents/ 2>> /output/chroot_setup.log; then
        log "ERROR: Failed to copy $file"
        exit 1
    fi
done

# Unmount the ISO file
umount /mnt/iso_temp
log "Unmounted ISO file"

# Check if squashfs file exists
if [ ! -f /output/iso_contents/casper/filesystem.squashfs ]; then
    handle_error "filesystem.squashfs not found in casper directory"
fi

# Extract squashfs
log "Starting squashfs extraction"
unsquashfs -d /output/iso_contents/squashfs-root /output/iso_contents/casper/filesystem.squashfs || handle_error "Squashfs extraction failed"

# Ensure mount points exist
mkdir -p /output/iso_contents/squashfs-root/proc /output/iso_contents/squashfs-root/sys /output/iso_contents/squashfs-root/dev/pts
log "Created mount points for chroot environment"

# Setup chroot environment
mount -t proc none /output/iso_contents/squashfs-root/proc || handle_error "Failed to mount proc"
mount -t sysfs none /output/iso_contents/squashfs-root/sys || handle_error "Failed to mount sys"
mount -o bind /dev /output/iso_contents/squashfs-root/dev || handle_error "Failed to bind /dev"
mount -o bind /dev/pts /output/iso_contents/squashfs-root/dev/pts || handle_error "Failed to bind /dev/pts"

# Ensure resolv.conf is copied
cp /etc/resolv.conf /output/iso_contents/squashfs-root/etc/ || handle_error "Failed to copy resolv.conf"

log "Chroot environment setup complete"

# Check if a command is provided
if [ -n "$1" ]; then
    # Execute the command and then start an interactive shell
    exec chroot /output/iso_contents/squashfs-root /bin/bash -c "$1; exec /bin/bash"
else
    # Start an interactive shell
    exec chroot /output/iso_contents/squashfs-root /bin/bash
fi