savedcmd_drivers/pci/pcie/built-in.a := rm -f drivers/pci/pcie/built-in.a;  printf "drivers/pci/pcie/%s " portdrv.o rcec.o bwctrl.o aspm.o pme.o | xargs ar cDPrST drivers/pci/pcie/built-in.a
