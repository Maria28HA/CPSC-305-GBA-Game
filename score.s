.global increment_score

.data
score:      .word   0           @ Define score variable

.text
increment_score:
    @ Load the current value of score into a register
    ldr r0, =score
    ldr r1, [r0]

    @ Increment the value of score
    add r1, r1, #1

    @ Store the updated value back into score
    str r1, [r0]

    @ Return
    mov pc, lr

