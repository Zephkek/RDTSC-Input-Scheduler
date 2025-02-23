;
; Mohamed Maatallah <hotelsmaatallahrecemail@gmail.com>
; This file implements a busy-wait routine using RDTSCP for high precision delays.
; It subtracts measured syscall overhead and jitter so that delays are as accurate as possible.
; calculates: delay_cycles = (ns * cyclesPerNs) - (syscallOverhead + jitter)
; busy-waits until current tsc >= target tsc.

EXTERN gCyclesPerNanosecond:QWORD
EXTERN gSyscallOverheadCycles:QWORD
EXTERN gJitterCompensationCycles:QWORD

.code
PUBLIC RDTSCInputSchedulerWait

RDTSCInputSchedulerWait PROC
    ; get cycles-per-ns; bail if zero
    mov     r8, qword ptr [gCyclesPerNanosecond]
    test    r8, r8
    jz      RDTSCInputSchedulerWait_SafeReturn

    ; compute total cycles needed = ns * cycles-per-ns
    mov     rax, rcx
    mul     r8                ; now rax = rcx * gCyclesPerNanosecond

    ; subtract measured overhead+jitter
    mov     r11, qword ptr [gSyscallOverheadCycles]
    add     r11, qword ptr [gJitterCompensationCycles]
    cmp     rax, r11
    jb      RDTSCInputSchedulerWait_SafeReturn
    sub     rax, r11
    mov     r9, rax          ; effective cycles to wait

    ; get current tsc (rdtscp serializes)
    rdtscp                   ; rax = low, rdx = high
    lfence                   ; mfence isn't necessary for rdtscp
    shl     rdx, 32
    or      rax, rdx         ; combine into full 64-bit tsc
    mov     r10, rax         ; starting tsc
    add     r10, r9          ; target tsc = start + delay

L1:
    ; check current tsc
    rdtscp
    lfence
    shl     rdx, 32
    or      rax, rdx
    cmp     rax, r10
    jae     L_exit

    ; double-check for extra accuracy
    rdtscp
    lfence
    shl     rdx, 32
    or      rax, rdx
    cmp     rax, r10
    jae     L_exit

    pause                   ; relax a bit
    jmp     L1

L_exit:
    ret

RDTSCInputSchedulerWait_SafeReturn:
    ret
RDTSCInputSchedulerWait ENDP
END
