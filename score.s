.global increment_score

.text
increment_score:
    @ Load the current value of score into a register
    ldr r1, =score      @ Load the address of score into r1
    ldr r0, [r1]        @ Load the value of score into r0

    @ Increment the value of score
    add r0, r0, #1

    @ Store the updated value back into score
    str r0, [r1]

    @ Return
    mov pc, lr

