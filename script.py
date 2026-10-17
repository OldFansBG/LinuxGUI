"""LinuxISOPro backend: prepares a Docker container with the ISO's root filesystem unpacked.

The GUI (LinuxISOPro.exe) starts this script (bundled as script.exe with PyInstaller) with
--project-dir and --iso-path. The script:

1. writes setup_output.sh, setup_chroot.sh and create_iso.sh into the project directory,
2. starts a privileged ubuntu container and saves its ID to <project-dir>/container_id.txt,
3. copies the ISO into the container and runs setup_output.sh + setup_chroot.sh,
   which unpack the root filesystem to /root/custom_iso/squashfs-root and mount it as a chroot.

The container is then left running so the GUI can customise the chroot. The GUI runs
/create_iso.sh when the user clicks "Create ISO".
"""

import argparse
import io
import json
import logging
import os
import re
import subprocess
import sys
import tarfile
import time
from datetime import datetime

import docker

SCRIPT_NAMES = ["setup_output.sh", "setup_chroot.sh", "create_iso.sh"]
DEFAULT_SQUASHFS = "casper/filesystem.squashfs"

SETUP_OUTPUT_SCRIPT = """#!/bin/bash
set -e
export DEBIAN_FRONTEND=noninteractive

mkdir -p /root/custom_iso

apt-get update
apt-get install -y --no-install-recommends squashfs-tools xorriso ca-certificates
"""

SETUP_CHROOT_SCRIPT = r"""#!/bin/bash
echo "Starting setup_chroot.sh..."

WORKDIR=/root/custom_iso
ROOTFS="$WORKDIR/squashfs-root"
SQUASHFS_PATH="$WORKDIR/__SQUASHFS_PATH__"

mkdir -p /mnt/iso "$WORKDIR"
mount -o loop,ro /base.iso /mnt/iso || { echo "ERROR: could not mount /base.iso"; exit 1; }

# Copy the ISO contents, including hidden folders such as .disk/
cp -a /mnt/iso/. "$WORKDIR/"
umount /mnt/iso

cd "$WORKDIR" || exit 1
echo "Using filesystem: $SQUASHFS_PATH"

if [ ! -f "$SQUASHFS_PATH" ]; then
    echo "ERROR: Selected filesystem $SQUASHFS_PATH not found!"
    find . -name "*.squashfs" -o -name "*.sfs" | sort
    exit 1
fi

rm -rf "$ROOTFS"
unsquashfs -d "$ROOTFS" "$SQUASHFS_PATH" || { echo "ERROR: unsquashfs failed"; exit 1; }

mount -t proc none "$ROOTFS/proc"
mount -t sysfs none "$ROOTFS/sys"
mount -o bind /dev "$ROOTFS/dev"
mount -o bind /dev/pts "$ROOTFS/dev/pts"

mkdir -p "$ROOTFS/output"
mount --bind "$WORKDIR" "$ROOTFS/output"

# Use the container's DNS while customising; create_iso.sh restores the original file.
RESOLV="$ROOTFS/etc/resolv.conf"
rm -rf /root/resolv.conf.orig
if [ -e "$RESOLV" ] || [ -L "$RESOLV" ]; then
    cp -a "$RESOLV" /root/resolv.conf.orig
fi
rm -f "$RESOLV"
cp /etc/resolv.conf "$RESOLV"

cat > "$ROOTFS/tmp/install_flatpak.sh" << 'EOF'
#!/bin/bash
echo "Detecting package manager..."
if command -v apt-get >/dev/null 2>&1; then
    echo "Detected apt-get (Debian/Ubuntu/Mint)"
    export DEBIAN_FRONTEND=noninteractive
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

# The names on the left must match the desktop cards in the GUI (DesktopTab.cpp).
cat > "$ROOTFS/tmp/detect_gui.sh" << 'EOF'
#!/bin/bash
SESSION_NAME="Unknown"
while IFS='|' read -r NAME BINARY; do
    if [ -x "$BINARY" ]; then
        SESSION_NAME="$NAME"
        break
    fi
done << 'LIST'
GNOME|/usr/bin/gnome-shell
KDE Plasma|/usr/bin/plasmashell
XFCE|/usr/bin/xfwm4
Cinnamon|/usr/bin/cinnamon-session
MATE|/usr/bin/mate-session
LXQt|/usr/bin/lxqt-session
Budgie|/usr/bin/budgie-wm
Deepin|/usr/bin/startdde
Pantheon|/usr/bin/gala
LXDE|/usr/bin/lxsession
Enlightenment|/usr/bin/enlightenment
LIST

if [ "$SESSION_NAME" == "Unknown" ]; then
    for xsessions_dir in /usr/share/xsessions /usr/share/wayland-sessions /etc/X11/sessions; do
        if [ -d "$xsessions_dir" ] && [ "$(ls -A "$xsessions_dir" 2>/dev/null)" ]; then
            NAME_FROM_FILE=$(grep -h -m 1 "^Name=" "$xsessions_dir"/*.desktop 2>/dev/null | head -n 1 | cut -d"=" -f2)
            if [ -n "$NAME_FROM_FILE" ]; then
                SESSION_NAME="$NAME_FROM_FILE"
                break
            fi
        fi
    done
fi

if [ "$SESSION_NAME" == "Unknown" ]; then
    for wm in i3 sway dwm spectrwm openbox fluxbox icewm jwm awesome bspwm; do
        if command -v $wm >/dev/null 2>&1; then
            SESSION_NAME=$wm
            break
        fi
    done
fi

echo "Detected GUI environment: $SESSION_NAME"
echo "$SESSION_NAME" > /output/detected_gui.txt
EOF

chmod +x "$ROOTFS/tmp/install_flatpak.sh" "$ROOTFS/tmp/detect_gui.sh"

echo "Attempting to install flatpak..."
chroot "$ROOTFS" /tmp/install_flatpak.sh || echo "Package installation failed, continuing anyway"

echo "Detecting the GUI environment in the chroot environment..."
chroot "$ROOTFS" /tmp/detect_gui.sh

rm -f "$ROOTFS/tmp/install_flatpak.sh" "$ROOTFS/tmp/detect_gui.sh"

echo "Ready"
"""

CREATE_ISO_SCRIPT = r"""#!/bin/bash
# Repacks the customised root filesystem and writes a new ISO.
# The boot setup (BIOS + UEFI) of the original ISO is replayed by xorriso,
# so this works for Ubuntu/Mint, Debian, Arch and other live ISOs alike.
echo "Starting create_iso.sh..."

WORKDIR=/root/custom_iso
ROOTFS="$WORKDIR/squashfs-root"
SQUASHFS_REL="__SQUASHFS_PATH__"
OUTPUT_ISO="$WORKDIR/custom_linux.iso"
NEW_SQUASHFS=/root/new_rootfs.squashfs

cd "$WORKDIR" || { echo "Failed to change to $WORKDIR"; exit 1; }
rm -f creation_complete "$OUTPUT_ISO" "$NEW_SQUASHFS"

if [ ! -d "$ROOTFS" ]; then
    echo "ERROR: $ROOTFS does not exist. Run setup_chroot.sh first."
    exit 1
fi

echo "Cleaning up temporary files in the chroot..."
rm -f "$ROOTFS/process_id.txt"
rm -rf "$ROOTFS/tmp/"* "$ROOTFS/var/tmp/"*
if [ -x "$ROOTFS/usr/bin/apt-get" ]; then
    chroot "$ROOTFS" apt-get clean || true
fi

echo "Unmounting chroot environment..."
for mountpoint in output dev/pts dev sys proc; do
    if mountpoint -q "$ROOTFS/$mountpoint"; then
        umount "$ROOTFS/$mountpoint" || umount -l "$ROOTFS/$mountpoint" || echo "Warning: could not unmount /$mountpoint"
    fi
done

echo "Restoring the original resolv.conf..."
rm -f "$ROOTFS/etc/resolv.conf"
if [ -e /root/resolv.conf.orig ] || [ -L /root/resolv.conf.orig ]; then
    cp -a /root/resolv.conf.orig "$ROOTFS/etc/resolv.conf"
fi

echo "Compressing the customised filesystem (this can take a while)..."
mksquashfs "$ROOTFS" "$NEW_SQUASHFS" -comp xz -noappend || { echo "Failed to create squashfs"; exit 1; }

MAP_ARGS=(-map "$NEW_SQUASHFS" "/$SQUASHFS_REL")
FS_DIR=$(dirname "$SQUASHFS_REL")
SQUASHFS_NAME=$(basename "$SQUASHFS_REL")

# Ubuntu/Mint (casper) keep the unpacked size next to the squashfs
if [ -f "$WORKDIR/$FS_DIR/filesystem.size" ]; then
    du -sx --block-size=1 "$ROOTFS" | cut -f1 > /root/filesystem.size
    MAP_ARGS+=(-map /root/filesystem.size "/$FS_DIR/filesystem.size")
fi

# Arch (archiso) keeps a checksum of the squashfs
SHA_NAME="${SQUASHFS_NAME%.*}.sha512"
if [ -f "$WORKDIR/$FS_DIR/$SHA_NAME" ]; then
    (cd /root && sha512sum new_rootfs.squashfs | sed "s/new_rootfs.squashfs/$SQUASHFS_NAME/") > /root/rootfs.sha512
    MAP_ARGS+=(-map /root/rootfs.sha512 "/$FS_DIR/$SHA_NAME")
fi

echo "Writing $OUTPUT_ISO ..."
xorriso -indev /base.iso -outdev "$OUTPUT_ISO" "${MAP_ARGS[@]}" -boot_image any replay \
    || { echo "Failed to create ISO"; exit 1; }
rm -f "$NEW_SQUASHFS"

if [ ! -f "$OUTPUT_ISO" ]; then
    echo "ISO file not found."
    exit 1
fi
ISO_SIZE=$(du -h "$OUTPUT_ISO" | cut -f1)
echo "ISO file created successfully: $OUTPUT_ISO ($ISO_SIZE)"

echo "Re-mounting the chroot so it can be customised further..."
mount -t proc none "$ROOTFS/proc"
mount -t sysfs none "$ROOTFS/sys"
mount -o bind /dev "$ROOTFS/dev"
mount -o bind /dev/pts "$ROOTFS/dev/pts"
mount --bind "$WORKDIR" "$ROOTFS/output"
rm -f "$ROOTFS/etc/resolv.conf"
cp /etc/resolv.conf "$ROOTFS/etc/resolv.conf"

touch "$WORKDIR/creation_complete"
echo "create_iso.sh completed successfully."
"""


def setup_logging():
    log_dir = "logs"
    os.makedirs(log_dir, exist_ok=True)

    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    log_file = os.path.join(log_dir, f"iso_builder_{timestamp}.log")

    handlers = [logging.FileHandler(log_file, encoding="utf-8")]
    if sys.stdout is not None:  # None when bundled without a console
        handlers.append(logging.StreamHandler(sys.stdout))

    logging.basicConfig(
        level=logging.DEBUG,
        format="%(asctime)s - %(levelname)s - %(message)s",
        handlers=handlers,
    )
    return log_file


def get_project_name(project_dir):
    try:
        settings_path = os.path.join(project_dir, "settings.json")
        with open(settings_path, "r", encoding="utf-8") as f:
            settings = json.load(f)
            return settings.get("project_name") or "default_project"
    except (FileNotFoundError, json.JSONDecodeError) as e:
        logging.error(f"Error reading settings.json from {project_dir}: {str(e)}")
        return "default_project"


def sanitize_container_name(name):
    sanitized = re.sub(r"[^a-zA-Z0-9_.-]", "_", name or "")

    if not sanitized or not sanitized[0].isalnum():
        sanitized = "project_" + sanitized

    return sanitized[:64]


def get_selected_filesystem(project_dir):
    """Path of the root filesystem inside the ISO, as chosen in the GUI."""
    project_name = get_project_name(project_dir)
    fs_file_path = os.path.join(project_dir, f"{project_name}_selected_fs.txt")

    if not os.path.exists(fs_file_path):
        logging.warning(f"No filesystem selection file found at {fs_file_path}, using default")
        return DEFAULT_SQUASHFS

    with open(fs_file_path, "r", encoding="utf-8") as f:
        selected_fs = f.readline().strip()
    if " (" in selected_fs:  # The GUI appends a description such as " (Root filesystem)"
        selected_fs = selected_fs.split(" (")[0].strip()
    selected_fs = selected_fs.replace("\\", "/").lstrip("/")
    logging.info(f"Using selected filesystem: {selected_fs}")
    return selected_fs or DEFAULT_SQUASHFS


def create_setup_scripts(project_dir):
    try:
        logging.info("Creating setup scripts in: %s", project_dir)
        selected_fs = get_selected_filesystem(project_dir)

        scripts = {
            "setup_output.sh": SETUP_OUTPUT_SCRIPT,
            "setup_chroot.sh": SETUP_CHROOT_SCRIPT.replace("__SQUASHFS_PATH__", selected_fs),
            "create_iso.sh": CREATE_ISO_SCRIPT.replace("__SQUASHFS_PATH__", selected_fs),
        }

        os.makedirs(project_dir, exist_ok=True)
        for script_name, content in scripts.items():
            script_path = os.path.join(project_dir, script_name)
            with open(script_path, "w", newline="\n", encoding="utf-8") as f:
                f.write(content.replace("\r\n", "\n"))
            logging.debug(f"Created {script_name}")

        logging.info("All setup scripts created successfully")
        return True
    except Exception as e:
        logging.error("Failed to create setup scripts: %s", str(e))
        logging.debug("Detailed error:", exc_info=True)
        return False


def parse_arguments():
    parser = argparse.ArgumentParser(description="LinuxISOPro Docker backend")
    parser.add_argument("--project-dir", required=True, help="Directory containing settings.json")
    parser.add_argument("--iso-path", required=True, help="Path to the source ISO file")
    return parser.parse_args()


def copy_iso_to_container(container, host_path, container_dest_path):
    """Copy the ISO with `docker cp`, which streams the file instead of loading it into memory."""
    try:
        logging.info(f"Copying ISO: {os.path.basename(host_path)}")
        if not os.path.exists(host_path):
            raise FileNotFoundError(f"ISO file not found: {host_path}")

        creationflags = getattr(subprocess, "CREATE_NO_WINDOW", 0)
        result = subprocess.run(
            ["docker", "cp", host_path, f"{container.id}:{container_dest_path}"],
            capture_output=True,
            text=True,
            creationflags=creationflags,
        )
        if result.returncode != 0:
            raise RuntimeError(result.stderr.strip() or f"docker cp exited with {result.returncode}")

        logging.info("ISO copy completed successfully")
        return True
    except Exception as e:
        logging.error("Failed to copy ISO: %s", str(e))
        logging.debug("Detailed error:", exc_info=True)
        return False


def copy_scripts_to_container(container, project_dir):
    tarstream = io.BytesIO()
    with tarfile.open(fileobj=tarstream, mode="w") as tar:
        for script in SCRIPT_NAMES:
            info = tar.gettarinfo(os.path.join(project_dir, script), arcname=script)
            info.mode = 0o755
            with open(os.path.join(project_dir, script), "rb") as f:
                tar.addfile(info, f)
    tarstream.seek(0)
    container.put_archive("/", tarstream)


def run_in_container(client, container, cmd, label):
    """Run cmd in the container, log its output line by line and return the exit code."""
    exec_id = client.api.exec_create(container.id, cmd)["Id"]
    pending = ""
    for chunk in client.api.exec_start(exec_id, stream=True):
        pending += chunk.decode("utf-8", errors="replace")
        *lines, pending = pending.split("\n")
        for line in lines:
            logging.debug(f"[{label}] {line.rstrip()}")
    if pending:
        logging.debug(f"[{label}] {pending.rstrip()}")
    return client.api.exec_inspect(exec_id)["ExitCode"]


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
            privileged=True,
        )
        container.reload()
        if container.status != "running":
            raise RuntimeError(f"Container failed to start. Status: {container.status}")

        container_id_path = os.path.join(args.project_dir, "container_id.txt")
        with open(container_id_path, "w", encoding="utf-8") as f:
            f.write(container.id)
        logging.info(f"Container ID saved to {container_id_path}: {container.id}")

        if not copy_iso_to_container(container, args.iso_path, "/base.iso"):
            raise RuntimeError("Failed to copy ISO to container")

        logging.info("Copying scripts to container")
        copy_scripts_to_container(container, args.project_dir)

        logging.info("Running setup_output.sh")
        if run_in_container(client, container, "/bin/bash /setup_output.sh", "SETUP") != 0:
            raise RuntimeError("setup_output.sh failed")

        logging.info("Running setup_chroot.sh")
        if run_in_container(client, container, "/bin/bash /setup_chroot.sh", "CHROOT") != 0:
            raise RuntimeError("setup_chroot.sh failed")

        success = True
        logging.info("=" * 50)
        logging.info("CONTAINER READY FOR CUSTOMISATION")
        logging.info("=" * 50)
        logging.info(f"Setup duration: {datetime.now() - start_time}")
        logging.info(f"Log file: {log_file}")

    except Exception as e:
        logging.error("=" * 50)
        logging.error("SETUP FAILED")
        logging.error("=" * 50)
        logging.error(f"Error: {str(e)}")
        logging.error(f"Duration: {datetime.now() - start_time}")
        logging.error(f"Log file: {log_file}")
        logging.debug("Detailed error trace:", exc_info=True)

    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()
