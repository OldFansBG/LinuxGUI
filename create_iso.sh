#!/bin/bash

# Log the start of the script
echo "Starting create_iso.sh..."

# Change to the working directory
echo "Changing to the working directory: ~/custom_iso"
cd ~/custom_iso || { echo "Failed to change to ~/custom_iso"; exit 1; }

# Step 1: Unmount chroot environment
echo "Unmounting chroot environment..."
sudo umount squashfs-root/proc || echo "Failed to unmount /proc"
sudo umount squashfs-root/sys || echo "Failed to unmount /sys"
sudo umount squashfs-root/dev/pts || echo "Failed to unmount /dev/pts"
sudo umount squashfs-root/dev || echo "Failed to unmount /dev"
sudo umount squashfs-root/output || echo "Failed to unmount /output"

# Step 2: Rebuild squashfs filesystem
echo "Rebuilding squashfs filesystem..."
rm -f casper/filesystem.squashfs || echo "Failed to remove old squashfs"
mksquashfs squashfs-root casper/filesystem.squashfs -comp xz || { echo "Failed to create squashfs"; exit 1; }

# Step 3: Update filesystem.size
echo "Updating filesystem.size..."
printf "$(du -sx --block-size=1 squashfs-root | cut -f1)" > casper/filesystem.size || { echo "Failed to update filesystem.size"; exit 1; }

# Step 4: Remove extracted filesystem
echo "Removing extracted filesystem..."
rm -rf squashfs-root || { echo "Failed to remove squashfs-root"; exit 1; }

# Step 5: Recreate the ISO
echo "Recreating the ISO..."
xorriso -as mkisofs -r -V "Custom Linux Mint" \
    -cache-inodes -J -l \
    -b isolinux/isolinux.bin -c isolinux/boot.cat \
    -no-emul-boot -boot-load-size 4 -boot-info-table \
    -eltorito-alt-boot -e EFI/boot/bootx64.efi -no-emul-boot \
    -isohybrid-gpt-basdat \
    -output custom_linuxmint.iso . || { echo "Failed to create ISO"; exit 1; }

# Step 6: Verify the ISO file
echo "Verifying the ISO file..."
if [ ! -f custom_linuxmint.iso ]; then
    echo "ISO file not found."
    exit 1
fi

ISO_SIZE=$(du -h custom_linuxmint.iso | cut -f1)
echo "ISO file created successfully. Size: $ISO_SIZE"

# Log the completion of the script
echo "create_iso.sh completed successfully."