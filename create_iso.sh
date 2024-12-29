#!/bin/bash
cd /output/custom_iso

# Cleanup chroot environment
umount squashfs-root/proc
umount squashfs-root/sys
umount squashfs-root/dev/pts
umount squashfs-root/dev

# Rebuild squashfs
rm casper/filesystem.squashfs
mksquashfs squashfs-root casper/filesystem.squashfs -comp xz

# Update filesystem size
printf $(du -sx --block-size=1 squashfs-root | cut -f1) > casper/filesystem.size

# Remove extracted filesystem
rm -rf squashfs-root

# Create final ISO
xorriso -as mkisofs -r -V "Custom Linux Mint" \
    -cache-inodes -J -l \
    -b isolinux/isolinux.bin -c isolinux/boot.cat \
    -no-emul-boot -boot-load-size 4 -boot-info-table \
    -eltorito-alt-boot -e EFI/boot/bootx64.efi -no-emul-boot \
    -isohybrid-gpt-basdat \
    -output ../custom.iso .

chmod 777 ../custom.iso