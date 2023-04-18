.GLOBL damagePlayer
damagePlayer:
    swp     r4, r4, [sp]			 @ pop last input argument from stack and put it in r4
    stmfd   sp!, {r4-r11, lr}     @ save content of r4-r11 and link register into the sp register
    @ Start your assembly code here
    @ Also, think about breaking the
    @ code into sub-macros you can branch
    @ to/from, and comment each line with
    @ the corresponding C instruction

	ldr r1,[r0,#0] @ load value at address r0 (&player_hp) to r1 (current hp)
	sub r1,#1 @ current hp - 1
	str r1,[r0,#0] @ str value at r1 (current hp - 1) back to &player_hp

    @ Here, you can use r4-r11 as temporary variables for the algorithm
    @ Exit from function
    ldmfd   sp!, {r4-r11, lr}     @ Recover past state of r4-r11 and link register from sp register
    swp     r4, r4, [sp]			 @ Restore state of r4
    mov     pc, lr					 @ Branch back to lr (go back to C code that called this function)
