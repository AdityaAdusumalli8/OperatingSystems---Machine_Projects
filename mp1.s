# mp1.s - Your solution goes here
#
        .section .data                    # Data section (optional for global data)
        .extern skyline_beacon             # Declare the external global variable

        .global skyline_star_cnd
        .type   skyline_star_cnd, @object

        .text
        .global start_beacon
        .type   start_beacon, @function

		.global add_star
		.type add_star, @function
		.equ SKYLINE_STARS_MAX, 1000

		.global remove_star
		.type remove_star, @function



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

        
        
#

add_star:
	la t0, skyline_star_cnt		# Load the address of global var skyline_star_cnt
	lh t2, 0(t0)				# Load the 16bit count into t2
	li t1, SKYLINE_STARS_MAX	# Load the immediate value of 1000 into t1 (max # of stars)
	
	# Check for space in array
	bge t2,t1,add_star_exit		# If current star count is bigger or equal to max, exit
	
	# Calculate Address of the new star in array if space exists
	la t3, skyline_stars	# Load base address of array into t3
	li t5, 7
	mul t2, t2, t5			# Multiply star count by 7 (bytes in struct) memory is byte-addressable
	add t3,t3,t2		# Add this offset to base address of array to get new location
	
	sh a0, 0(t3)	# Store x-coord 
	sh a1, 2(t3)	# Store y-coord 
	sh a2, 5(t3)	# Store color
	
	lh t2, 0(t0)	# Load star count back into t2
	addi t2,t2,1	# Increment count
	sh t2, 0(t0)	# Store star count back into memory address
add_star_exit:
	ret				# Return
	
#
	
remove_star:
	la t0, skyline_star_cnt		# Load address of count into skyline_star_cnt
	lh t1,0(t0)					# Load star count into t1
	beqz t1, remove_star_exit	# If star count is 0, nothing to remove and exit
	
	la t2, skyline_stars		# Load array base address into t2
	li t3, 0					# Load 0 to t3 as index
	
remove_star_loop:
	bge t3,t1, remove_star_exit		# If index >= star count, exit loop
	li t4, 7				# Calculate address of current star
	mul t5, t3, t4			
	add t5, t2,t5
	
	lh t4, 0(t5)		# Load x value into t4
	lh t5, 2(t5)		# Load y value into t5
	
	bne t4, a0, remove_star_next 	# Compare current x value with a0; if not equal go to next
	bne t5, a1, remove_star_next	# Compare current y value with a1; if not equal go to next
	
	# Star found
	addi t1,t1,-1		# If x and y are equal to arguments, decrement star count
	beq t3,t1, remove_star_update 	# If star found is last one, no swap

	li t4,7		# Calculate offset for last star
	mul t4, t1, t4
	add t5 , t2, t4 	# Calculate address of last star
	
	li t4,7
	mul t3 , t3, t4
	add t3, t2, t3		# Calculate address of current star

	# Copy x,y,color
	lh t4, 0(t5)
	sh t4, 0(t3)
	
	lh t4, 2(t5)
	sh t4, 2(t3)
	
	lh t4, 5(t5)
	sh t4, 5(t3)
	
remove_star_update:
	# Store updated star count to memory
	sh t1, 0(t0)
remove_star_exit:
	ret
remove_star_next:
	add t3,t3,1
	j remove_star_loop

	

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
	
#
	.section .text
	.global add_window
	.type add_window, @function
	
add_window:
	li a0,16
	call malloc
	
	beqz a0, add_window_exit
	
	sh a1, 8(a0)
	sh a2, 10(a0)
	sb a3, 12(a0)
	sb a4, 13(a0)
	sh a5, 14(a0)
	
	la t0, skyline_win_list
	ld t1, 0(t0)
	sd t1, 0(a0)
	
	sd a0, 0(t0)
add_window_exit:
	ret
	
#
	.section .text
	.global remove_window
	.type remove_window, @function
remove_window:
	la t0, skyline_win_list
	ld t1, 0(t0)
	beqz t1, remove_window_exit
	
	li t2, 0
remove_window_loop:
	beqz t1, remove_window_exit
	lh t3, 8(t1)
	lh t4, 10(t1)
	
	bne t3,a0, check_next_window
	bne t4, a1, check_next_window
	
	beqz t2, remove_head
	
	ld t5, 0(t1)
	sd t5, 0(t2)
	j free_memory
	
remove_head:
	ld t5, 0(t1)
	sd t5, 0(t0)
	j free_memory
	
check_next_window:
	addi t2,t1, 0
	ld t1, 0(t1)
	j remove_window_loop

free_memory:
	mv a0,t1
	call free
remove_window_exit:
	ret

#
	.section .text
	.global draw_window
	.type draw_window, @function
draw_window:
	lh t0, 8(a1)
	lh t1, 10(a1)
	lb t2, 12(a1)
	lb t3, 13(a1)
	lh t4, 14(a1)
	
	li t5, SKYLINE_WIDTH
	li t6, SKYLINE_HEIGHT
	
	li t7, 0
draw_window_row:
	bge t7,t3, draw_window_exit
	li t8,0
draw_window_col:
	bge t8,t2,next_row
	add t9,t0,t8
	add t10, t1, t7
	
	bgeu t9,t5,next_pixel
	bgeu t10,t6, next_pixel
	mul t11, t10, t5
	add t11, t11, t9
	slli t11, t11, 1
	
	add t12, a0, t11
	sh t4, 0(t12)
	
next_pixel:
	addi t8,t8, 1
	j draw_window_col

next_row:
	addi t7,t7,1
	j draw_window_row
draw_window_exit:
	ret
	
#
	.section .text
	.global draw_beacon
	.type draw_beacon, @function
	
draw_function:
	lh t0, 14(a2)
	lh t1, 16(a2)
	
	remu t2,a1,t0
	
	bgeu t2,t1, draw_beacon_exit
	
	lh t3, 8(a2)
	lh t4, 10(a2)
	lb t5, 12(a2)
	
	ld t6, 0(a2)
	
	li t7, SKYLINE_WIDTH
	li t8, SKYLINE_WIDTH
	
	li t9, 0
	li t10,0
draw_beacon_row:
	bge t9,t5, draw_beacon_exit
	li t10, 0
draw_beacon_col:
	bge t10, t5, next_row
	add t11, t3,t10
	add t12, t4,t9
	
	bgeu t11, t7, skip_pixel
	bgeu t12, t8, skip_pixel
	
	mul t13,t12, t7
	add t13, t13,t11
	slli t13,t13,1
	
	lhu t14, 0(t6)
	addi t6,t6,2
	
	add t15,a0, t13
	sh t14, 0(t15)
skip_pixel:
	addi t10,t10, 1
	j draw_beacon_col
next_row:
	addi t9,t9,1
	j draw_beacon_row
draw_beacon_exit:
	ret

	.end

	
