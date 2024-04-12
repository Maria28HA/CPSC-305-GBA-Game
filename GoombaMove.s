@Goomba Movement

/* function that makes the Goomba move left and right */
.global GoombaMove
.equ SCREEN_WIDTH, 240

GoombaMove:
    stmfd sp!, {r4-r7, lr}  @ Save registers

    @ Load parameters
    ldr r4, [r0]      @ Load x-coordinate of Goomba
    ldr r5, [r1]      @ Load direction of Goomba

    @ Adjust x-coordinate based on direction
    add r4, r4, r5    @ Increment/decrement x-coordinate based on direction

    @ Check boundaries
    cmp r4, #0        @ Check if x-coordinate is less than or equal to 0 (left boundary)
    blt change_direction_left

    ldr r6, =SCREEN_WIDTH    @ Load SCREEN_WIDTH into register r6
    sub r6, r6, #32          @ Subtract 32 from SCREEN_WIDTH
    cmp r4, r6               @ Compare x-coordinate with SCREEN_WIDTH - 32

continue:
    @ Update Goomba's x-coordinate
    str r4, [r0]

    ldmfd sp!, {r4-r7, pc}  @ Restore registers and return

change_direction_left:
    mov r5, #1        @ Change direction to right
    b continue

change_direction_right:
    mov r5, #-1       @ Change direction to left
    b continue

