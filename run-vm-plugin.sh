#!/bin/bash

port=$((2221 + 0))

/mydata/QEMU/build/qemu-system-x86_64 \
-smp 1 -m 16384 \
-nographic \
-drive file=/mydata/ubuntu-22.04-server-cloudimg-amd64.qcow2,if=virtio,format=qcow2 \
-drive file="/mydata/cloud/seed.img",if=virtio,format=raw,readonly=on \
-kernel /mydata/linux/arch/x86/boot/bzImage \
-append "nokaslr console=ttyS0 root=/dev/vda1 loglevel=8 \
  systemd.mask=snapd.service systemd.mask=snapd.seeded.service \
  systemd.mask=snap.lxd.service" \
-device virtio-net-pci,netdev=net0 \
-netdev user,id=net0,hostfwd=tcp::$port-:22 \
-icount 0 \
-plugin /mydata/QEMU/api/libtrace_dump.so,arg="filename=test_out_0_compressed" 
