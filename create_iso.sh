#!/bin/bash

# Log the start of the script
echo "Starting create_iso.sh..."

# Remove the sentinel file if it exists (to avoid false positives)
rm -f ~/custom_iso/creation_complete

# Change to the working directory
WORKDIR=~/custom_iso
echo "Changing to the working directory: $WORKDIR"
cd "$WORKDIR" || { echo "Failed to change to $WORKDIR"; exit 1; }

# Step 1: Unmount chroot environment
echo "Unmounting chroot environment..."
for mountpoint in proc sys dev/pts dev output; do
    sudo umount "squashfs-root/$mountpoint" || echo "Failed to unmount /$mountpoint"
done

# Step 2: Rebuild squashfs filesystem
echo "Rebuilding squashfs filesystem..."
if [ -f casper/filesystem.squashfs ]; then
    rm -f casper/filesystem.squashfs || echo "Failed to remove old squashfs"
fi
mksquashfs squashfs-root casper/filesystem.squashfs -comp xz || { echo "Failed to create squashfs"; exit 1; }

# Step 3: Update filesystem.size
echo "Updating filesystem.size..."
du -sx --block-size=1 squashfs-root | cut -f1 > casper/filesystem.size || { echo "Failed to update filesystem.size"; exit 1; }

# Step 4: Remove extracted filesystem
echo "Removing extracted filesystem..."
rm -rf squashfs-root || { echo "Failed to remove squashfs-root"; exit 1; }

# Step 5: Create the ISO using xorriso
echo "Creating the ISO using xorriso..."
xorriso -as mkisofs \
    -r -V "Custom Linux Mint" \
    -J -l \
    -b isolinux/isolinux.bin -c isolinux/boot.cat \
    -no-emul-boot -boot-load-size 4 -boot-info-table \
    -eltorito-alt-boot \
    -e EFI/boot/bootx64.efi -no-emul-boot \
    -o custom_linuxmint.iso . || { echo "Failed to create ISO"; exit 1; }

# Step 6: Verify the ISO file
echo "Verifying the ISO file..."
if [ ! -f custom_linuxmint.iso ]; then
    echo "ISO file not found."
    exit 1
fi

ISO_SIZE=$(du -h custom_linuxmint.iso | cut -f1)
echo "ISO file created successfully. Size: $ISO_SIZE"

# Step 7: (Optional) Check ISO hybrid compatibility for legacy BIOS
if command -v isohybrid >/dev/null 2>&1; then
    echo "Making the ISO hybrid using isohybrid..."
    isohybrid --uefi custom_linuxmint.iso || echo "Failed to make ISO hybrid (optional step)."
else
    echo "isohybrid command not found, skipping hybridization step."
fi

# Step 8: Create a sentinel file to indicate completion
SENTINEL_FILE=~/custom_iso/creation_complete
touch "$SENTINEL_FILE" || { echo "Failed to create sentinel file"; exit 1; }
echo "Sentinel file created at: $SENTINEL_FILE"

# Wait for file system sync
sleep 2

# Log the completion of the script
echo "create_iso.sh completed successfully."