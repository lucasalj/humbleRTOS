            .cdecls C,LIST,"msp430f5529.h","humble_rtos.h"
.text
    .align 2
    .global TA0IV
    .global humble_rtos_schedule_next
    .global run_task
    .global enable_interrupts
    .global disable_interrupts

    .def quantum_isr

enable_interrupts:  .asmfunc
    NOP
    BIS #0x0008, SR
    NOP
    RETA
    .endasmfunc

disable_interrupts: .asmfunc
    NOP
    BIC #0x0008, SR
    NOP
    RETA
    .endasmfunc

humble_rtos_scheduler_launch:   .asmfunc
    MOVA    #run_task,  R4
    MOVA    @R4,        R5
    MOVA    @R5,        SP
    POPM.A  #12,        R15
    NOP
    BIS #0x0008, SR
    NOP
    RETA
    .endasmfunc

quantum_isr:      .asmfunc
    PUSHM.A     #12,        R15
    MOVA        #run_task,  R4
    MOVA        @R4,        R5
    MOVA        SP,         0(R5)
    CALLA       #humble_rtos_schedule_next
    MOVA        @R4,        R5
    MOVA        @R5,        SP
    MOVA        #TA0IV,     R4
    MOVA        @R4,        R5
    POPM.A      #12,        R15
    NOP
    BIS #0x0008, SR
    NOP
    RETA
    .endasmfunc
    .end
