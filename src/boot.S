.text
.global _start
.type _start, @function
.extern kmain
.extern __stack_top
_start:
    cli
    leaq __stack_top(%rip), %rsp
    andq $-16, %rsp
    call kmain
1:
    hlt
    jmp 1b
.size _start, . - _start
