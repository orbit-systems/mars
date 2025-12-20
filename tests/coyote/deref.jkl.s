.section text
.sectionflag code

.align 4
foo:
.global foo
    mov long [a0], 4
    addi t0, zero, 5
    mov t1, long [a0]
    sub t0, t0, t1
    add t0, t0, t0
    addi a3, t0, 6
    mov long [a3], a0
    ret

.section data

.section rodata

.section bss
.sectionflag zeroed

