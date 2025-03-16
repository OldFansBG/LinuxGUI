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
import json
import re

def setup_logging():
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
    try:
        settings_path = os.path.join(project_dir, "settings.json")
        with open(settings_path, "r") as f:
            settings = json.load(f)
            return settings.get("project_name", "default_project")
    except (FileNotFoundError, json.JSONDecodeError) as e:
        logging.error(f"Error reading settings.json from {project_dir}: {str(e)}")
        return "default_project"

def sanitize_container_name(name):
    sanitized = re.sub(r'[^a-zA-Z0-9_.-]', '_', name)
    
    if not sanitized[0].isalnum():
        sanitized = "project_" + sanitized
    
    return sanitized[:64]

def create_setup_scripts(project_dir):
    try:
        logging.info("Creating setup scripts in: %s", project_dir)
        
        project_name = get_project_name(project_dir)
        fs_file_path = os.path.join(project_dir, f"{project_name}_selected_fs.txt")
        selected_fs = "casper/filesystem.squashfs"
                
        if os.path.exists(fs_file_path):
            with open(fs_file_path, 'r') as f:
                selected_fs = f.readline().strip()
                if " (" in selected_fs:
                    selected_fs = selected_fs.split(" (")[0].strip()
                selected_fs = selected_fs.replace("\\", "/")
                logging.info(f"Using selected filesystem: {selected_fs}")
        else:
            logging.warning(f"No filesystem selection file found at {fs_file_path}, using default")
                
        setup_output_content = """#!/bin/bash

mkdir -p ~/custom_iso

apt-get update
apt-get install -y squashfs-tools xorriso isolinux syslinux-utils genisoimage
"""

        setup_chroot_content = f"""#!/bin/bash

echo "Starting setup_chroot.sh..."

mkdir -p /mnt/iso
mount -o loop base.iso /mnt/iso

cp -av /mnt/iso/* ~/custom_iso/

SQUASHFS_PATH="/root/custom_iso/{selected_fs}"
echo "Using filesystem: $SQUASHFS_PATH"

if [ ! -f "$SQUASHFS_PATH" ]; then
    echo "ERROR: Selected filesystem $SQUASHFS_PATH not found!"
    find . -name "*.squashfs" -o -name "*.sfs" | sort
    exit 1
fi

unsquashfs -d squashfs-root "$SQUASHFS_PATH"

mount -t proc none squashfs-root/proc
mount -t sysfs none squashfs-root/sys
mount -o bind /dev squashfs-root/dev
mount -o bind /dev/pts squashfs-root/dev/pts

mkdir -p squashfs-root/output

mount --bind ~/custom_iso squashfs-root/output

if [ -L "squashfs-root/etc/resolv.conf" ]; then
    rm -f squashfs-root/etc/resolv.conf
    cp /etc/resolv.conf squashfs-root/etc/
else
    cp /etc/resolv.conf squashfs-root/etc/
fi

cat > squashfs-root/tmp/install_flatpak.sh << 'EOF'
#!/bin/bash
set -e

echo "Detecting package manager..."
if command -v apt-get >/dev/null 2>&1; then
    echo "Detected apt-get (Debian/Ubuntu/Mint)"
    apt-get update
    apt-get install -y flatpak || echo "Failed to install flatpak, continuing anyway"
elif command -v pacman >/dev/null 2>&1; then
    echo "Detected pacman (Arch/Manjaro)"
    mkdir -p /var/cache/pacman/pkg
    if ! pacman-key --list-keys >/dev/null 2>&1; then
        pacman-key --init
        pacman-key --populate archlinux
    fi
    pacman -Sy --needed --noconfirm flatpak || echo "Failed to install flatpak, continuing anyway"
elif command -v dnf >/dev/null 2>&1; then
    echo "Detected dnf (Fedora/RHEL)"
    dnf -y install flatpak || echo "Failed to install flatpak, continuing anyway"
elif command -v zypper >/dev/null 2>&1; then
    echo "Detected zypper (openSUSE)"
    zypper --non-interactive install flatpak || echo "Failed to install flatpak, continuing anyway"
elif command -v apk >/dev/null 2>&1; then
    echo "Detected apk (Alpine)"
    apk add flatpak || echo "Failed to install flatpak, continuing anyway"
elif command -v xbps-install >/dev/null 2>&1; then
    echo "Detected xbps-install (Void)"
    xbps-install -y flatpak || echo "Failed to install flatpak, continuing anyway"
else
    echo "Unknown package manager. Cannot install packages."
fi

if command -v flatpak >/dev/null 2>&1; then
    echo "Setting up Flathub repository..."
    flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo || \
    echo "Failed to add Flathub, continuing anyway"
else
    echo "Flatpak not found, skipping Flathub setup"
fi
EOF

chmod +x squashfs-root/tmp/install_flatpak.sh

echo "Attempting to install flatpak..."
chroot squashfs-root /tmp/install_flatpak.sh || echo "Package installation failed, continuing anyway"

echo "Detecting the GUI environment in the chroot environment..."
chroot squashfs-root /bin/bash -c '
    declare -A GUI_ENV_MAP=(
        ["GNOME"]="/usr/bin/gnome-shell"
        ["KDE"]="/usr/bin/plasmashell"
        ["Plasma"]="/usr/bin/plasmashell"
        ["XFCE"]="/usr/bin/xfwm4"
        ["LXDE"]="/usr/bin/lxsession"
        ["LXQt"]="/usr/bin/lxqt-session"
        ["Cinnamon"]="/usr/bin/cinnamon-session"
        ["MATE"]="/usr/bin/mate-session"
        ["Enlightenment"]="/usr/bin/enlightenment"
        ["i3"]="/usr/bin/i3"
        ["Sway"]="/usr/bin/sway"
        ["Awesome"]="/usr/bin/awesome"
        ["bspwm"]="/usr/bin/bspwm"
    )

    SESSION_NAME="Unknown"

    for ENV in "${{!GUI_ENV_MAP[@]}}"; do
        DETECT_PATH=${{GUI_ENV_MAP[$ENV]}}
        if [ -f "$DETECT_PATH" ]; then
            SESSION_NAME=$ENV
            break
        fi
    done

    if [ "$SESSION_NAME" == "Unknown" ]; then
        for xsessions_dir in /usr/share/xsessions /etc/X11/sessions /usr/local/share/xsessions; do
            if [ -d "$xsessions_dir" ] && [ "$(ls -A $xsessions_dir 2>/dev/null)" ]; then
                SESSION_NAME=$(grep -m 1 "Name=" $xsessions_dir/*.desktop 2>/dev/null | cut -d"=" -f2 | head -n 1)
                [ -n "$SESSION_NAME" ] && break
            fi
        done
    fi
    
    if [ "$SESSION_NAME" == "Unknown" ]; then
        for wm in i3 sway dwm spectrwm openbox fluxbox icewm jwm; do
            if command -v $wm >/dev/null 2>&1; then
                SESSION_NAME=$wm
                break
            fi
        done
    fi

    [ -z "$SESSION_NAME" ] && SESSION_NAME="Unknown"

    echo "Detected GUI environment: $SESSION_NAME"
    echo "$SESSION_NAME" > /output/detected_gui.txt
'

chroot squashfs-root /bin/bash -c "echo 'Process ID inside chroot: $$'; echo $$ > /process_id.txt"

echo "Ready"

if [ -n "$1" ]; then
    exec chroot squashfs-root /bin/bash -c "$1; exec /bin/bash"
else
    exec chroot squashfs-root /bin/bash
"""

        create_iso_content = f"""#!/bin/bash

echo "Starting create_iso.sh..."

rm -f ~/custom_iso/creation_complete

WORKDIR=~/custom_iso
echo "Changing to the working directory: $WORKDIR"
cd "$WORKDIR" || {{ echo "Failed to change to $WORKDIR"; exit 1; }}

echo "Unmounting chroot environment..."
for mountpoint in proc sys dev/pts dev output; do
    umount "squashfs-root/$mountpoint" || echo "Failed to unmount /$mountpoint"
done

SQUASHFS_PATH="{selected_fs}"
echo "Rebuilding filesystem: $SQUASHFS_PATH"

echo "Rebuilding squashfs filesystem..."
if [ -f "$SQUASHFS_PATH" ]; then
    rm -f "$SQUASHFS_PATH" || echo "Failed to remove old squashfs"
fi
mksquashfs squashfs-root "$SQUASHFS_PATH" -comp xz || {{ echo "Failed to create squashfs"; exit 1; }}

echo "Updating filesystem.size..."
du -sx --block-size=1 squashfs-root | cut -f1 > casper/filesystem.size || {{ echo "Failed to update filesystem.size"; exit 1; }}

echo "Removing extracted filesystem..."
rm -rf squashfs-root || {{ echo "Failed to remove squashfs-root"; exit 1; }}

echo "Creating the ISO using xorriso..."
xorriso -as mkisofs \\
    -r -V "Custom Linux Mint" \\
    -J -l \\
    -b isolinux/isolinux.bin -c isolinux/boot.cat \\
    -no-emul-boot -boot-load-size 4 -boot-info-table \\
    -eltorito-alt-boot \\
    -e EFI/boot/bootx64.efi -no-emul-boot \\
    -o custom_linuxmint.iso . || {{ echo "Failed to create ISO"; exit 1; }}

echo "Verifying the ISO file..."
if [ ! -f custom_linuxmint.iso ]; then
    echo "ISO file not found."
    exit 1
fi

ISO_SIZE=$(du -h custom_linuxmint.iso | cut -f1)
echo "ISO file created successfully. Size: $ISO_SIZE"

if command -v isohybrid >/dev/null 2>&1; then
    echo "Making the ISO hybrid using isohybrid..."
    isohybrid --uefi custom_linuxmint.iso || echo "Failed to make ISO hybrid (optional step)."
else:
    echo "isohybrid command not found, skipping hybridization step."
fi

SENTINEL_FILE=~/custom_iso/creation_complete
touch "$SENTINEL_FILE" || {{ echo "Failed to create sentinel file"; exit 1; }}
echo "Sentinel file created at: $SENTINEL_FILE"

sleep 2

echo "create_iso.sh completed successfully."
"""

        os.makedirs(project_dir, exist_ok=True)

        scripts = {
            "setup_output.sh": setup_output_content,
            "setup_chroot.sh": setup_chroot_content,
            "create_iso.sh": create_iso_content
        }

        for script_name, content in scripts.items():
            script_path = os.path.join(project_dir, script_name)
            fixed_content = content.replace("\r\n", "\n")
            with open(script_path, 'w', newline='\n') as f:
                f.write(fixed_content)
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
        
        logging.info("Creating setup scripts")
        if not create_setup_scripts(args.project_dir):
            raise RuntimeError("Failed to create setup scripts")

        logging.info("Initializing Docker client")
        client = docker.from_env()
        client.ping()

        project_name = get_project_name(args.project_dir)
        container_name = sanitize_container_name(project_name)
        logging.info(f"Using container name: {container_name}")

        try:
            old_container = client.containers.get(container_name)
            logging.info("Removing old container")
            old_container.remove(force=True)
            time.sleep(2)
        except docker.errors.NotFound:
            logging.debug("No existing container found")

        logging.info("Creating new container")
        container = client.containers.run(
            "ubuntu:latest",
            command="sleep infinity",
            name=container_name,
            detach=True,
            privileged=True
        )
        container.reload()
        if container.status != "running":
            raise RuntimeError(f"Container failed to start. Status: {container.status}")

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