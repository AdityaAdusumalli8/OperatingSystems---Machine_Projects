# mp1.s - Your solution goes here
#
        .section .data                    # Data section (optional for global data)
        .extern skyline_beacon             # Declare the external global variable

        .global skyline_star_cnd
        .type   skyline_star_cnd, @object

        .text
        .global start_beacon
        .type   start_beacon, @function

start_beacon:

        la t0, skyline_beacon             # Load address of skyline_beacon into t0 (t0 is 64-bit)

        # Store the function arguments into the struct fields
        sd a0, 0(t0)                      # Store img (a0, 64-bit) at offset 0 (8 bytes)

        sh a1, 8(t0)                      # Store x (a1, 16-bit) at offset 8 (after img pointer)

        sh a2, 10(t0)                     # Store y (a2, 16-bit) at offset 10

        sb a3, 12(t0)                     # Store dia (a3, 8-bit) at offset 12

        sh a4, 14(t0)                     # Store period (a4, 16-bit) at offset 14

        sh a5, 16(t0)                     # Store ontime (a5, 16-bit) at offset 16

        ret                               # Return to caller

        .end
        
#
	.section .text
	.global add_star
	.type add_star, @function
	
add_star:
	la t0, skyline_star_cnt
	li t1, SKYLINE_STARS_MAX
	lh t2, 0(t0)
	
	bge t2,t1,add_star_exit
	
	la t3, skyline_stars
	slli t2,t2,4
	add t3,t3,t2
	
	sh a0, 0(t3)
	sh a1, 2(t3)
	sh a2, 6(t3)
	
	lh t4, 0(t0)
	addi t4,t4,1
	sh t4, 0(t0)
add_star_exit:
	ret
	
#
	.section .text
	.global remove_star
	.type remove_star, @function
remove_star:
	la t0, skyline_star_cnt
	lh t1,0(t0)
	beqz t1, remove_star_exit
	
	la t2, skyline_stars
	li t3, 0
	
remove_star_loop:
	bge t3,t1, remove_star_exit
	slii t4,t3, 4
	add t5, t2,t4
	
	lh t6, 0(t5)
	lh t7, 2(t5)
	
	bne t6, a0, remove_star_next
	bne t7, a1, remove_star_next
	
	addi t1,t1,-1
	sh t1,0(t0)
	
	beq t3,t1, remove_star_exit
	
	slli t4,t1,4
	add t6,t2,t4
	
	lh t7, 0(t6)
	sh t7, 0(t5)
	
	lh t7, 2(t6)
	sh t7, 2(t5)
	
	lh t7, 6(t6)
	sh t7, 6(t5)
	
	j remove_star_exit
	
remove_star_next:
	add t3,t3,1
	j remove_star_loop
remove_star_exit:
	ret
	
#
	.section .text
	.global draw_star
	.type draw_star, @function
draw_star:
	lh t0, 0(a1)
	lh t1, 2(a1)
	lb t2, 4(a1)
	lh t3, 6(a1)
	
	li t4, 640
	mul t5, t1,t4
	
	add t5,t5,t0
	slli t5,t5,1
	
	add t6,a0,t5
	li t7,0
draw_star_row:
	bge t7,t2, draw_star_exit
	li  t8, 0
draw_star_col:
	bge t8, t2, next_row
	sh t3, 0(t6)
	addi t6,t6,2
	
	addi t8,t8,1
	j draw_star_col

next_row:
	add t6,t6,t4
	addi t7,t7,1
	j draw_star_row
draw_star_exit:
	ret
	

	
