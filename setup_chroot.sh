#!/bin/bash

# Log the start of the script
echo "Starting setup_chroot.sh..."

# Mount the ISO file
sudo mkdir -p /mnt/iso
sudo mount -o loop base.iso /mnt/iso

# Copy ISO contents to working directory
cp -av /mnt/iso/* ~/custom_iso/

# Extract squashfs filesystem
cd ~/custom_iso
unsquashfs -d squashfs-root casper/filesystem.squashfs

# Prepare chroot environment
mount -t proc none squashfs-root/proc
mount -t sysfs none squashfs-root/sys
mount -o bind /dev squashfs-root/dev
mount -o bind /dev/pts squashfs-root/dev/pts

mkdir -p squashfs-root/output

# Bind mount working directory
mount --bind ~/custom_iso squashfs-root/output

# Copy resolv.conf
cp /etc/resolv.conf squashfs-root/etc/

# Install Flatpak inside the chroot environment
chroot squashfs-root /bin/bash -c "apt-get update && apt-get install -y flatpak"

# Add Flathub repository
chroot squashfs-root /bin/bash -c "flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo"

# Display and save process ID
chroot squashfs-root /bin/bash -c "echo 'Process ID inside chroot: $$'; echo $$ > /process_id.txt"

# Check if a command is provided
if [ -n "$1" ]; then
    # Execute the command and then start an interactive shell
    exec chroot squashfs-root /bin/bash -c "$1; exec /bin/bash"
else
    # Start an interactive shell
    exec chroot squashfs-root /bin/bash
fi