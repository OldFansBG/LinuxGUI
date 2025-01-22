#!/bin/bash

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

# Check if a command is provided
if [ -n "$1" ]; then
    # Execute the command and then start an interactive shell
    exec chroot squashfs-root /bin/bash -c "$1; exec /bin/bash"
else
    # Start an interactive shell
    exec chroot squashfs-root /bin/bash
fi