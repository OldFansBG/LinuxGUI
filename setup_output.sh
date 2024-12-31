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

log "Updating package lists"
apt-get update || handle_error "Failed to update package lists"

log "Installing required packages"
apt-get install -y squashfs-tools xorriso isolinux || handle_error "Failed to install required packages"

log "setup_output.sh completed successfully"