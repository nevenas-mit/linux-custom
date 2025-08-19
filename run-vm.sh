#!/bin/bash
set -euo pipefail

# --- Paths (adjust if you moved things) ---
QEMU="/mydata/QEMU_nevena/QEMU/build/qemu-system-x86_64"
DISK="/mydata/ubuntu2204-server-cloudimg-amd64.qcow2"
KERNEL="/mydata/linux/arch/x86/boot/bzImage"

# --- VM resources ---
RAM_MB=16384
VCPUS=1

# --- Preflight checks ---
[[ -x "$QEMU" ]] || { echo "ERROR: QEMU not found/executable at: $QEMU"; exit 1; }
[[ -f "$DISK" ]] || { echo "ERROR: Disk image not found: $DISK"; exit 1; }
[[ -f "$KERNEL" ]] || { echo "ERROR: Kernel bzImage not found: $KERNEL"; exit 1; }

# --- Pick accelerator: prefer KVM if present in this QEMU build and /dev/kvm exists ---
ACCEL="tcg"
if [[ -e /dev/kvm ]] && "$QEMU" -accel help 2>/dev/null | grep -q '\bkvm\b'; then
  ACCEL="kvm"
fi
echo "Using accelerator: $ACCEL"

# --- Launch ---
exec "$QEMU" \
  -enable-kvm \
  -cpu host \
  -smp "$VCPUS" -m "$RAM_MB" \
  -nographic \
  -drive file="$DISK",if=virtio,format=qcow2 \
  -drive file="/mydata/cloud/seed.img",if=virtio,format=raw,readonly=on \
  -device virtio-net-pci,netdev=net0 \
  -netdev user,id=net0,hostfwd=tcp::2223-:22 \
  -append "console=ttyS0 root=/dev/vda1 loglevel=8 \
  systemd.mask=snapd.service systemd.mask=snapd.seeded.service \
  systemd.mask=snap.lxd.service" \
  -kernel "$KERNEL"

  
