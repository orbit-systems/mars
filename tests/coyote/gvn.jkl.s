.section text
.sectionflag code

.align 4
gvn:
.global gvn
    mov t0, long [a0]
    mov t1, long [a0]
    add t0, t0, t1
    mov t1, long [a0]
    mov t2, long [a0]
    add t1, t1, t2
    add t0, t0, t1
    mov t1, long [a0]
    mov t2, long [a0]
    add t1, t1, t2
    add t0, t0, t1
    mov t1, long [a0]
    mov t2, long [a0]
    add t1, t1, t2
    add t0, t0, t1
    mov t1, long [a0]
    mov t2, long [a0]
    add t1, t1, t2
    add a3, t0, t1
    ret

.section data

.section rodata

.section bss
.sectionflag zeroed

