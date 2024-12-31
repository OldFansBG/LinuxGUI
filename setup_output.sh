#!/bin/bash
set -e  # Exit immediately if a command exits with a non-zero status
set -x  # Print commands and their arguments as they are executed

# Create the /output directory if it doesn't exist
mkdir -p /output
chmod 777 /output

# Extensive logging function
log() {
    echo "[$(date +'%Y-%m-%d %H:%M:%S')] $*" | tee -a /output/setup_output.log
}

# Error handling function
handle_error() {
    log "ERROR: $*"
    exit 1
}

log "Starting setup_output.sh"

# Update package lists
log "Updating package lists"
apt-get update || handle_error "Failed to update package lists"

# Install required packages
log "Installing required packages"
apt-get install -y squashfs-tools xorriso isolinux || handle_error "Failed to install required packages"

# Copy create_iso.sh script to /output (if it exists on the host)
if [ -f /host/create_iso.sh ]; then
    log "Copying create_iso.sh from host to /output"
    cp /host/create_iso.sh /output/create_iso.sh || handle_error "Failed to copy create_iso.sh"
    chmod +x /output/create_iso.sh || handle_error "Failed to set execute permissions on create_iso.sh"
else
    log "create_iso.sh not found on host. Skipping copy."
fi

log "setup_output.sh completed successfully"