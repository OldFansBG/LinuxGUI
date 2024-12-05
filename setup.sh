#!/bin/bash
set -e

# Create required directories
cd /tmp
mkdir -p iso
mkdir -p edit squashfs-root

# Mount ISO
mount -o loop /iso.iso /tmp/iso
cd /tmp/iso

# Find squashfs file
for file in $(find . -type f -name "*.squashfs"); do
    echo "Found squashfs: $file"
    SQUASHFS_PATH="$file"
    break
done

if [ -z "$SQUASHFS_PATH" ]; then
    echo "No squashfs file found"
    exit 1
fi

# Extract squashfs
cd /tmp
unsquashfs -f -d squashfs-root "iso/$SQUASHFS_PATH"

# Setup chroot environment
mount --bind squashfs-root edit
mount --bind /proc edit/proc
mount --bind /sys edit/sys
mount --bind /dev edit/dev
mount --bind /dev/pts edit/dev/pts
cp /etc/resolv.conf edit/etc/

# Show contents and enter chroot
ls -la edit/
chroot edit /bin/bash