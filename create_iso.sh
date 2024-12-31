#!/bin/bash

# Enable real-time debug output
set -x

# Exit on error
set -e

# Configurable Variables
CUSTOM_ISO_DIR="/output/iso_contents"  # Update this to match your working directory
CUSTOM_ISO_NAME="custom_linuxmint.iso"
SQUASHFS_ROOT="$CUSTOM_ISO_DIR/squashfs-root"
LOG_FILE="/output/create_iso.log"  # Log file location

# Redirect all output (including debug) to the log file and the terminal
exec > >(tee -a "$LOG_FILE") 2>&1

# Logging function
log() {
    echo "[$(date +'%Y-%m-%d %H:%M:%S')] $*" | tee -a "$LOG_FILE"
}

# Error handling function
handle_error() {
    log "ERROR: $*"
    exit 1
}

# Initialize log file
log "Starting create_iso.sh"

# Step 0: Check if the working directory exists
log "Checking if working directory exists: $CUSTOM_ISO_DIR"
if [ ! -d "$CUSTOM_ISO_DIR" ]; then
    handle_error "Directory $CUSTOM_ISO_DIR does not exist."
fi

# Step 1: Rebuild the squashfs filesystem with better compression
log "Rebuilding the squashfs filesystem..."
cd "$CUSTOM_ISO_DIR" || handle_error "Failed to cd to $CUSTOM_ISO_DIR"
rm -f casper/filesystem.squashfs || handle_error "Failed to remove old filesystem.squashfs"
mksquashfs "$SQUASHFS_ROOT" casper/filesystem.squashfs -comp xz || handle_error "Failed to create new filesystem.squashfs"

# Step 2: Update the filesystem.size file
log "Updating the filesystem.size file..."
bash -c 'printf $(du -sx --block-size=1 "$SQUASHFS_ROOT" | cut -f1) > casper/filesystem.size' || handle_error "Failed to update filesystem.size"

# Step 3: Remove the extracted filesystem to save space
log "Removing the extracted filesystem..."
rm -rf "$SQUASHFS_ROOT" || handle_error "Failed to remove squashfs-root"

# Step 4: Recreate the ISO
log "Recreating the ISO..."
xorriso -as mkisofs -r -V "Custom Linux Mint" \
    -cache-inodes -J -l \
    -b isolinux/isolinux.bin -c isolinux/boot.cat \
    -no-emul-boot -boot-load-size 4 -boot-info-table \
    -eltorito-alt-boot -e EFI/boot/bootx64.efi -no-emul-boot \
    -isohybrid-gpt-basdat \
    -output "$CUSTOM_ISO_DIR/$CUSTOM_ISO_NAME" "$CUSTOM_ISO_DIR" || handle_error "Failed to create ISO"

log "Custom ISO created successfully: $CUSTOM_ISO_DIR/$CUSTOM_ISO_NAME"