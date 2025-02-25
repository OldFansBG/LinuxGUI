import docker
import os
import time
import traceback
import sys
import tarfile
import io
import argparse
import logging
from datetime import datetime
import json  # Added for JSON parsing
import re  # Added for sanitizing container names

def setup_logging():
    """Set up logging configuration."""
    log_dir = "logs"
    os.makedirs(log_dir, exist_ok=True)
    
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    log_file = os.path.join(log_dir, f"iso_builder_{timestamp}.log")
    
    logging.basicConfig(
        level=logging.DEBUG,
        format='%(asctime)s - %(levelname)s - %(message)s',
        handlers=[
            logging.FileHandler(log_file),
            logging.StreamHandler(sys.stdout)
        ]
    )
    return log_file

def get_project_name(project_dir):
    """
    Read the project name from settings.json located in the provided project directory.
    If the file is missing or the JSON is malformed, defaults to "default_project".
    """
    try:
        settings_path = os.path.join(project_dir, "settings.json")
        with open(settings_path, "r") as f:
            settings = json.load(f)
            return settings.get("project_name", "default_project")
    except (FileNotFoundError, json.JSONDecodeError) as e:
        logging.error(f"Error reading settings.json from {project_dir}: {str(e)}")
        return "default_project"

def sanitize_container_name(name):
    """Sanitize the container name to meet Docker naming rules."""
    # Docker allows: [a-zA-Z0-9][a-zA-Z0-9_.-]
    sanitized = re.sub(r'[^a-zA-Z0-9_.-]', '_', name)
    
    # Ensure it starts with alphanumeric
    if not sanitized[0].isalnum():
        sanitized = "project_" + sanitized
    
    # Truncate to 64 chars (Docker limit)
    return sanitized[:64]

def create_setup_scripts(project_dir):
    """Create necessary shell scripts with proper permissions and Unix line endings."""
    try:
        logging.info("Creating setup scripts in: %s", project_dir)
        
        # setup_output.sh content
        setup_output_content = """#!/bin/bash

# Create working directory if it doesn't exist 
mkdir -p ~/custom_iso

# Update package lists and install necessary tools 
apt-get update
apt-get install -y squashfs-tools xorriso isolinux syslinux-utils genisoimage
"""

        # setup_chroot.sh content (removed sudo and enforced Unix line endings)
        setup_chroot_content = """#!/bin/bash

# Log the start of the script
echo "Starting setup_chroot.sh..."

# Mount the ISO file
mkdir -p /mnt/iso
mount -o loop base.iso /mnt/iso

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

# Detect the GUI environment in the chroot and save the output outside the chroot
echo "Detecting the GUI environment in the chroot environment..."
chroot squashfs-root /bin/bash -c '
    declare -A GUI_ENV_MAP=(
        ["GNOME"]="/usr/bin/gnome-shell"
        ["KDE"]="/usr/bin/plasmashell"
        ["XFCE"]="/usr/bin/xfwm4"
        ["LXDE"]="/usr/bin/lxsession"
        ["Cinnamon"]="/usr/bin/cinnamon-session"
    )

    SESSION_NAME="Unknown"

    for ENV in "${!GUI_ENV_MAP[@]}"; do
        DETECT_PATH=${GUI_ENV_MAP[$ENV]}
        if [ -f "$DETECT_PATH" ]; then
            SESSION_NAME=$ENV
            break
        fi
    done

    if [ "$SESSION_NAME" == "Unknown" ]; then
        echo "No GUI binary detected, checking /usr/share/xsessions/*.desktop..."
        SESSION_NAME=$(grep -m 1 "Name=" /usr/share/xsessions/*.desktop | cut -d"=" -f2 | head -n 1)
        [ -z "$SESSION_NAME" ] && SESSION_NAME="Unknown"
    fi

    echo "Detected GUI environment: $SESSION_NAME"
    echo "$SESSION_NAME" > /output/detected_gui.txt
'

# Display and save process ID
chroot squashfs-root /bin/bash -c "echo 'Process ID inside chroot: $$'; echo $$ > /process_id.txt"

echo "Ready"

# Check if a command is provided
if [ -n "$1" ]; then
    # Execute the command and then start an interactive shell
    exec chroot squashfs-root /bin/bash -c "$1; exec /bin/bash"
else
    # Start an interactive shell
    exec chroot squashfs-root /bin/bash
fi
"""

        # create_iso.sh content (removed sudo from umount commands)
        create_iso_content = """#!/bin/bash

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
    umount "squashfs-root/$mountpoint" || echo "Failed to unmount /$mountpoint"
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
xorriso -as mkisofs \\
    -r -V "Custom Linux Mint" \\
    -J -l \\
    -b isolinux/isolinux.bin -c isolinux/boot.cat \\
    -no-emul-boot -boot-load-size 4 -boot-info-table \\
    -eltorito-alt-boot \\
    -e EFI/boot/bootx64.efi -no-emul-boot \\
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
"""

        # Create directory if it doesn't exist
        os.makedirs(project_dir, exist_ok=True)

        # Create and set permissions for each script, enforcing Unix LF line endings.
        scripts = {
            "setup_output.sh": setup_output_content,
            "setup_chroot.sh": setup_chroot_content,
            "create_iso.sh": create_iso_content
        }

        for script_name, content in scripts.items():
            script_path = os.path.join(project_dir, script_name)
            # Replace any CRLF with LF and write with newline='\n'
            fixed_content = content.replace("\r\n", "\n")
            with open(script_path, 'w', newline='\n') as f:
                f.write(fixed_content)
            # Set executable permissions (equivalent to chmod +x)
            os.chmod(script_path, 0o755)
            logging.debug(f"Created and set permissions for {script_name}")
            
        logging.info("All setup scripts created successfully")
        return True
    except Exception as e:
        logging.error("Failed to create setup scripts: %s", str(e))
        logging.debug("Detailed error:", exc_info=True)
        return False

def parse_arguments():
    parser = argparse.ArgumentParser(description='Docker ISO Builder')
    parser.add_argument('--project-dir', required=True, help='Directory containing the script files and settings.json')
    parser.add_argument('--iso-path', required=True, help='Path to the source ISO file')
    return parser.parse_args()

def copy_iso_to_container(container, host_path, container_dest_path):
    """Copy ISO file into container using a tar archive."""
    try:
        logging.info(f"Copying ISO: {os.path.basename(host_path)}")
        
        if not os.path.exists(host_path):
            raise FileNotFoundError(f"ISO file not found: {host_path}")
        
        tarstream = io.BytesIO()
        with tarfile.open(fileobj=tarstream, mode="w") as tar:
            tar.add(host_path, arcname=os.path.basename(container_dest_path))
        tarstream.seek(0)
        
        dest_dir = os.path.dirname(container_dest_path) or "/"
        container.put_archive(dest_dir, tarstream)
        logging.info("ISO copy completed successfully")
        return True
    except Exception as e:
        logging.error("Failed to copy ISO: %s", str(e))
        logging.debug("Detailed error:", exc_info=True)
        return False

def validate_scripts(base_dir):
    """Validate required scripts exist and are executable."""
    try:
        required_scripts = ["setup_output.sh", "setup_chroot.sh", "create_iso.sh"]
        for script in required_scripts:
            script_path = os.path.join(base_dir, script)
            if not os.path.exists(script_path):
                raise FileNotFoundError(f"Missing script: {script_path}")
            if not os.access(script_path, os.X_OK):
                raise PermissionError(f"Script not executable: {script_path}")
        logging.info("All scripts validated successfully")
        return True
    except Exception as e:
        logging.error("Script validation failed: %s", str(e))
        logging.debug("Detailed error:", exc_info=True)
        return False

def main():
    success = False
    log_file = setup_logging()
    start_time = datetime.now()
    
    try:
        logging.info("Starting ISO builder process")
        args = parse_arguments()
        
        # Create and set up scripts
        logging.info("Creating setup scripts")
        if not create_setup_scripts(args.project_dir):
            raise RuntimeError("Failed to create setup scripts")

        logging.info("Initializing Docker client")
        client = docker.from_env()
        client.ping()

        # Get project name from settings.json (located in project-dir) and sanitize it
        project_name = get_project_name(args.project_dir)
        container_name = sanitize_container_name(project_name)
        logging.info(f"Using container name: {container_name}")

        # Remove old container if it exists
        try:
            old_container = client.containers.get(container_name)
            logging.info("Removing old container")
            old_container.remove(force=True)
            time.sleep(2)
        except docker.errors.NotFound:
            logging.debug("No existing container found")

        # Create new container
        logging.info("Creating new container")
        container = client.containers.run(
            "ubuntu:latest",
            command="sleep infinity",
            name=container_name,  # Use sanitized container name
            detach=True,
            privileged=True
        )
        container.reload()
        if container.status != "running":
            raise RuntimeError(f"Container failed to start. Status: {container.status}")

        # Save container ID to file in the project directory
        container_id_path = os.path.join(args.project_dir, "container_id.txt")
        with open(container_id_path, "w") as f:
            f.write(container.id)
        logging.info(f"Container ID saved to {container_id_path}: {container.id}")

        logging.info("Validating scripts")
        if not validate_scripts(args.project_dir):
            raise RuntimeError("Script validation failed")

        if not copy_iso_to_container(container, args.iso_path, "/base.iso"):
            raise RuntimeError("Failed to copy ISO to container")

        logging.info("Copying scripts to container")
        for script in ["setup_output.sh", "setup_chroot.sh", "create_iso.sh"]:
            host_path = os.path.join(args.project_dir, script)
            
            tarstream = io.BytesIO()
            with tarfile.open(fileobj=tarstream, mode="w") as tar:
                tar.add(host_path, arcname=script)
            tarstream.seek(0)
            container.put_archive("/", tarstream)
            logging.debug(f"Copied {script} to container")

        logging.info("Setting script permissions in container")
        exit_code, output = container.exec_run("chmod +x /setup_output.sh /setup_chroot.sh /create_iso.sh")
        if exit_code != 0:
            raise RuntimeError(f"Permission setup failed: {output.decode()}")

        logging.info("Running setup_output.sh")
        exit_code, output = container.exec_run("/setup_output.sh")
        if exit_code != 0:
            raise RuntimeError(f"setup_output.sh failed: {output.decode()}")

        logging.info("Running setup_chroot.sh")
        exit_code, output_gen = container.exec_run("/setup_chroot.sh", stream=True)
        ready_signal = False
        for line in output_gen:
            line_str = line.decode().strip()
            logging.debug(f"[CHROOT] {line_str}")
            if "Ready" in line_str:
                ready_signal = True
                break
        if not ready_signal or exit_code != 0:
            raise RuntimeError("setup_chroot.sh did not complete successfully")

        logging.info("Running create_iso.sh")
        exit_code, output = container.exec_run("/create_iso.sh", timeout=300)
        if exit_code != 0:
            raise RuntimeError(f"create_iso.sh failed: {output.decode()}")

        success = True
        end_time = datetime.now()
        duration = end_time - start_time
        
        logging.info("="*50)
        logging.info("ISO BUILD COMPLETED SUCCESSFULLY")
        logging.info("="*50)
        logging.info(f"Build Duration: {duration}")
        logging.info(f"Log file: {log_file}")
        logging.info("="*50)

    except Exception as e:
        end_time = datetime.now()
        duration = end_time - start_time
        
        logging.error("="*50)
        logging.error("ISO BUILD FAILED")
        logging.error("="*50)
        logging.error(f"Error: {str(e)}")
        logging.error(f"Build Duration: {duration}")
        logging.error(f"Log file: {log_file}")
        logging.error("See log file for detailed debug information")
        logging.error("="*50)
        
        logging.debug("Detailed error trace:", exc_info=True)
        sys.exit(1)
    
    finally:
        if success:
            print("\nBuild completed successfully! 🎉")
            print(f"Build duration: {duration}")
            print(f"Log file: {log_file}")
        else:
            print("\nBuild failed! ❌")
            print(f"Build duration: {duration}")
            print(f"Check the log file for details: {log_file}")

if __name__ == "__main__":
    main()
