savedcmd_arch/x86/boot/startup/built-in.a := rm -f arch/x86/boot/startup/built-in.a;  printf "arch/x86/boot/startup/%s " gdt_idt.o map_kernel.o | xargs ar cDPrST arch/x86/boot/startup/built-in.a
