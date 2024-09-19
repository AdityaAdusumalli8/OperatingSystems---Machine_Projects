# mp1.s - Your solution goes here
#	
      .section .data                    # Data section (optional for global data)
      .extern skyline_beacon             # Declare the external global variable

      .extern skyline_stars
      .extern skyline_win_list

      .global skyline_star_cnt
      .type   skyline_star_cnt, @object

      .global skyline_star_cnt
      .type   skyline_star_cnt, @object

      .text
      .global start_beacon
      .type   start_beacon, @function

      
      .equ SKYLINE_STARS_MAX, 1000
      .equ SKYLINE_WIDTH, 640
      .equ SKYLINE_HEIGHT, 480
      


      .global remove_star
      .type remove_star, @function 

      .global add_star
      .type add_star, @function

      .global draw_star
      .type draw_star, @function

      .global remove_window
      .type remove_window, @function

      .global draw_window
      .type draw_window, @function
      
      
      
      .global add_window
      .type add_window, @function

      
      

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
      la t0, skyline_star_cnt       # Load the address of global var skyline_star_cnt
      lh t2, 0(t0)                        # Load the 16bit count into t2
      li t1, SKYLINE_STARS_MAX      # Load the immediate value of 1000 into t1 (max # of stars)
      
      # Check for space in array
      bge t2,t1,add_star_exit       # If current star count is bigger or equal to max, exit
      
      # Calculate Address of the new star in array if space exists
      la t3, skyline_stars    # Load base address of array into t3
      li t5, 8
      mul t2, t2, t5                # Multiply star count by 7 (bytes in struct) memory is byte-addressable
      add t3,t3,t2            # Add this offset to base address of array to get new location
      
      sh a0, 0(t3)      # Store x-coord 
      sh a1, 2(t3)      # Store y-coord 
      sh a2, 6(t3)      # Store color
      
      lh t2, 0(t0)      # Load star count back into t2
      addi t2,t2,1      # Increment count
      sh t2, 0(t0)      # Store star count back into memory address
add_star_exit:
      ret                     # Return
      
#
      
remove_star:
      la t0, skyline_star_cnt       # Load address of count into skyline_star_cnt
      lh t1,0(t0)                         # Load star count into t1
      beqz t1, remove_star_exit     # If star count is 0, nothing to remove and exit
      
      la t2, skyline_stars          # Load array base address into t2
      li t3, 0                            # Load 0 to t3 as index
      
remove_star_loop:
      bge t3,t1, remove_star_exit         # If index >= star count, exit loop
      li t4, 8                      # Calculate address of current star
      mul t5, t3, t4                
      add t5, t2,t5
      
      lh t4, 0(t5)            # Load x value into t4
      lh t5, 2(t5)            # Load y value into t5
      
      bne t4, a0, remove_star_next  # Compare current x value with a0; if not equal go to next
      bne t5, a1, remove_star_next  # Compare current y value with a1; if not equal go to next
      
      # Star found
      addi t1,t1,-1           # If x and y are equal to arguments, decrement star count
      beq t3,t1, remove_star_update       # If star found is last one, no swap

      li t4,8           # Calculate offset for last star
      mul t4, t1, t4
      add t5 , t2, t4   # Calculate address of last star
      
      li t4,7
      mul t3 , t3, t4
      add t3, t2, t3          # Calculate address of current star

      # Copy x,y,color
      lh t4, 0(t5)
      sh t4, 0(t3)
      
      lh t4, 2(t5)
      sh t4, 2(t3)
      
      lh t4, 6(t5)
      sh t4, 6(t3)
      
remove_star_update:
      # Store updated star count to memory
      sh t1, 0(t0)
remove_star_exit:
      ret
remove_star_next:
      add t3,t3,1
      j remove_star_loop

      

#

draw_star:

      # Load the star's fields
      lh t0, 0(a1)  # x-coord
      lh t1, 2(a1)      # y-coord
      lb t2, 4(a1)      # diameter
      lh t3, 6(a1)      # color

      # Calculate pixel coordinates
      
      li t5, SKYLINE_WIDTH    # Store the width
      mul t5, t5, t1                # Mutliply the width with the y-coord and store t5
      add t5, t5, t0                # Add x coord with other term
      li t4, 2
      mul t5,t4,t5                  # Mutliply by 2

      add a6, t5, a0                # Calculate the address in frame buffer and store color
      sh t3, 0(a6)

      ret
      
#
      
add_window:
      # Save arguments into temp registers before the malloc
      addi sp,sp, -8
      sd ra, 8(sp)
      mv t1, a0
      mv t2, a1
      mv t3, a2
      mv t4, a3
      mv t5, a4

      # Allocate memory for the window
      li a0,16
      call malloc
      
      # If allocation failed, then exit
      beqz a0, add_window_exit
      
      # Store window data into new struct
      sh t1, 8(a0)
      sh t2, 10(a0)
      sb t3, 12(a0)
      sb t4, 13(a0)
      sh t5, 14(a0)
      
      # Add window to linkedlist
      la t0, skyline_win_list       # Load address of linkedlist
      ld t1, 0(t0)                        # Load current first window to t1
      sd t1, 0(a0)                        # Set the next pointer of new window to current first window address 
      sd a0, 0(t0)                        # Update the head of list to new window
add_window_exit:
      ld ra, 8(sp)
      addi sp,sp,8
      ret
      
#
remove_window:
      la t0, skyline_win_list       # Load address of window linked list into t0
      ld t1, 0(t0)            # Load head of list into t1
      beqz t1, remove_window_exit         # If head of list is emtpy then exit
      
      li t2, 0                # Initialize a previous pointer
remove_window_loop:
      beqz t1, remove_window_exit # If head of linked list is empty then we exit
      lh t3, 8(t1)            # Load x-coord of current window into t3
      lh t4, 10(t1)           # Load y-coord of current window into t3
      
      # Compare x and y with function arguments. If x and y dont match, then go to next window. 
      bne t3,a0, check_next_window
      bne t4, a1, check_next_window
      
      # If matching window is found, then remove it from the linked list. 
      ld t5, 0(t1)
      beqz t2, remove_window_head

      # Set the next pointer of prev window to the next of current window
      sd t5, 0(t2)
      j free_memory_window
      
remove_window_head:
      sd t5,0(t0)
free_memory_window:
      mv a0,t1
      call free
      j remove_window_exit
check_next_window:
      mv t2,t1
      ld t1, 0(t1)
      j remove_window_loop

remove_window_exit:
      ret

#
      
draw_window:
      # Load window coord, size, color into temp
      lh t0, 8(a1)
      lh t1, 10(a1)
      lb t2, 12(a1)
      lb t3, 13(a1)
      lh t4, 14(a1)
      
      li t5, 0 # Initialize row counter

draw_window_row:
      bge t5,t3, draw_window_exit
      li a2,0     # Reset col counter
draw_window_col:
      bge a2,t2, draw_window_next_row

      # Calculate pixel coordinates
      add a4,t0,a2
      add a5, t1, t5
      
      # Ensure pixel is within window boundaries
      li a7, SKYLINE_WIDTH
      li a3, SKYLINE_HEIGHT
      bgeu a4,a7, draw_window_skip_pixel
      bgeu a5,a3, draw_window_skip_pixel

      # Calculate offset in frame buffer

      mul a6, a5, a7
      add a6, a6, a4
      slli a6, a6, 1
      
      # Set pixel color in the fram buffer
      add a6, a6, a0
      sh t4, 0(a6)
      
draw_window_skip_pixel:
      addi a2,a2, 1     # Increment column
      j draw_window_col

draw_window_next_row:
      addi t5,t5,1
      j draw_window_row
draw_window_exit:
      ret
      
#


      .end
