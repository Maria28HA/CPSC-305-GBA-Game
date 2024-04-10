@Goomba Movement

/* function that makes the Goomba move left and right */
goomba:
   @ Need to import the binary data of the Goomba sprite

goomba_x:
   @ Variable to store goomba's x-coordinate
goomba_direction:
   @ Variable to store goomba's direction (1 for right, 0 for left)

.global main

main:
  @ Setup
  mov r0, #100        @ Initial x-coordinate of the sprite
  mov r1, #100        @ Initial y-coordinate of the sprite
  mov r2, #1          @ Movement direction (1 for right, -1 for left)
  mov r3, #1          @ Speed of movement

loop:
  @ Move the goomba
  add r0, r0, r2, LSL #3    @ Adjust x-coordinate based on direction and speed (assuming 8x8 sprite)
  
  @ Check boundaries
  cmp r0, #20        @ Right boundary
  bge change_direction
  
  cmp r0, #20         @ Left boundary
  ble change_direction

continue:
  b loop

change_direction:
  neg r2, r2          @ Change direction
  b continue

