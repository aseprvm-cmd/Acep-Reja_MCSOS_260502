set confirm off
set pagination off
file build/kernel.elf
target remote localhost:1234
break kernel_main
break vmm_map_page
break x86_64_trap_dispatch
continue
