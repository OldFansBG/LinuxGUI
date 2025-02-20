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
    """Create necessary shell scripts with proper permissions."""
    try:
        logging.info("Creating setup scripts in: %s", project_dir)
        
        # setup_output.sh content
        setup_output_content = """#!/bin/bash 

# Create working directory if it doesn't exist 
mkdir -p ~/custom_iso

# Update package lists and install necessary tools 
apt-get update
apt-get install -y squashfs-tools xorriso isolinux sudo syslinux-utils genisoimage"""

        # setup_chroot.sh content
        setup_chroot_content = """#!/bin/bash

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
fi"""

        # create_iso.sh content (placeholder - you'll need to provide the actual content)
        create_iso_content = """#!/bin/bash
echo "Creating ISO..."
# Add your ISO creation logic here
"""

        # Create directory if it doesn't exist
        os.makedirs(project_dir, exist_ok=True)

        # Create and set permissions for each script
        scripts = {
            "setup_output.sh": setup_output_content,
            "setup_chroot.sh": setup_chroot_content,
            "create_iso.sh": create_iso_content
        }

        for script_name, content in scripts.items():
            script_path = os.path.join(project_dir, script_name)
            with open(script_path, 'w') as f:
                f.write(content)
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

        # Save container ID to file
        with open("container_id.txt", "w") as f:
            f.write(container.id)
        logging.info(f"Container ID saved to container_id.txt: {container.id}")

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
