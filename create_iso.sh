#!/bin/bash

# Change to the working directory
cd ~/custom_iso

# Unmount chroot environment
sudo umount squashfs-root/proc
sudo umount squashfs-root/sys
sudo umount squashfs-root/dev/pts
sudo umount squashfs-root/dev
sudo umount squashfs-root/output

# Rebuild squashfs filesystem
rm -f casper/filesystem.squashfs
mksquashfs squashfs-root casper/filesystem.squashfs -comp xz

# Update filesystem.size
printf "$(du -sx --block-size=1 squashfs-root | cut -f1)" > casper/filesystem.size

# Remove extracted filesystem
rm -rf squashfs-root

# Recreate the ISO
xorriso -as mkisofs -r -V "Custom Linux Mint" \
    -cache-inodes -J -l \
    -b isolinux/isolinux.bin -c isolinux/boot.cat \
    -no-emul-boot -boot-load-size 4 -boot-info-table \
    -eltorito-alt-boot -e EFI/boot/bootx64.efi -no-emul-boot \
    -isohybrid-gpt-basdat \
    -output custom_linuxmint.iso .