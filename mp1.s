# mp1.s - Your solution goes here
#
        .section .data
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

        .global add_star
        .type add_star, @function

        .global remove_star
        .type remove_star, @function 


        .global draw_star
        .type draw_star, @function
        
        .global add_window
        .type add_window, @function
          
        
        .global remove_window
        .type remove_window, @function

        .global draw_window
        .type draw_window, @function
        
        
        .global draw_beacon
        .type draw_beacon, @function
        

        

        

       
          
          
# void start_beacon(const uint16_t * img, ... uint16_t ontime)
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

        
        
# This function adds a star of specified color at position (x,y) and updates the star count. The star is 
# added to the array and if there is no room, the request is ignored.
# Arguments: a0 - x coord, a1 - y coord, a2 = color
add_star:
        la t0, skyline_star_cnt         # Load the address of global var skyline_star_cnt
        lh t2, 0(t0)                     # Load the 16bit count into t2
        li t1, SKYLINE_STARS_MAX          # Load the immediate value of 1000 into t1 (max # of stars)
          
        # Check for space in array
        bge t2,t1,add_star_exit         
          
        # Calculate Address of the new star in array if space exists

        la t3, skyline_stars    # Load base address of array into t3
        li t5, 8
        mul t4, t2, t5          # Multiply star count by 8 (bytes in struct) to get offset 
        add t3,t3,t4            # Add this offset to base address of array to get new location for star
          
        sh a0, 0(t3)          # Store x-coord of new star
        sh a1, 2(t3)          # Store y-coord of new star
        sh a2, 6(t3)          # Store color of new star (padding)
          
        addi t2,t2,1          # Increment stae count
        sh t2, 0(t0)      # Store star count back into memory address
add_star_exit:
        ret     # Return


# This function removes a star of specified position (x,y) from array and updates the star count. 
# If star doesn't exist, the request is ignored. Replace the removed star with the last star in array. 
# Arguments: a0 - x coord, a1 - y coord
remove_star:
        la t0, skyline_star_cnt       # Load the address of global var skyline_star_cnt
        lh t1,0(t0)                   # Load star count into t1
        beqz t1, remove_star_exit     # If star count is 0, array is empty so nothing to remove and exit
          
        la t2, skyline_stars    # Load array base address into t2
        li t3, 0        # Load 0 to t3 as index for loop
          
remove_star_loop:
        bge t3,t1, remove_star_exit   # If index >= star count, exit loop as we are outside of array
          
        # Calculate address of current star
        li t4, 8      # Load size of struct
        mul t5, t3, t4    # Multiply size of struct with index number to get exact offset for star location  
        add t5, t2,t5   # Access the exact address of current star by adding offset and array base address
          
        lh t4, 0(t5)    # Load x value of current star into t4
        lh t5, 2(t5)    # Load y value of current star into t5
          
        bne t4, a0, remove_star_next  # Compare current x value with a0; if not equal go to next star
        bne t5, a1, remove_star_next    # Compare current y value with a1; if not equal go to next star
          
        # Star found: 
        addi t1,t1,-1                  # If x and y are equal to arguments, decrement star count
        bne t3,t1, remove_star_update    # If current index is not equal to star count, we are removing non-last star so swap

        # If last star, no swap and Store updated star count to memory
        sh t1, 0(t0)
        j remove_star_exit

remove_star_update:
        # If removing non-last star, we must replace with last star to maintain contiguity. Get address of last star:
        li t4,8                  
        mul t4, t1, t4
        add t5 , t2, t4     # Calculate address of last star by adding offset with the base address of array
        
        # Get address of current star that is getting removed
        li t4,8
        mul t3 , t3, t4
        add t3, t2, t3      # Calculate address of last star by adding offset with the base address of array
        

        # Copy x,y,color from the last star to the current star that is getting removed. 
        lh t4, 0(t5)
        sh t4, 0(t3)
          
        lh t4, 2(t5)
        sh t4, 2(t3)
          
        lh t4, 6(t5)
        sh t4, 6(t3)

        # Store updated star count to memory
        sh t1, 0(t0)

remove_star_exit:
        ret
remove_star_next:
        # Iterate to the next star in array
        add t3,t3,1
        j remove_star_loop


# This function draws a star to the frame buffer. We must use the memory offset equation to calculate the 
# exact offset for the pixel.
# Arguments: a0 - pointer to frame buffer, a1 - pointer to star struct
draw_star:

        # Load the star's fields
        lh t0, 0(a1)  # x-coord
        lh t1, 2(a1)          # y-coord
        lb t2, 4(a1)          # diameter
        lh t3, 6(a1)          # color

        # Calculate pixel coordinates
          
        li t5, SKYLINE_WIDTH  # Store the frame buffer width
        mul t5, t5, t1        # Mutliply the width with the y-coord and store in t5
        add t5, t5, t0        # Add x coord with the above term
        li t4, 2
        mul t5,t4,t5           # Multiply value by 2 as each pixel contains color data that is 2 bytes

        # Calculate the address in frame buffer and store color
        add a6, t5, a0        
        sh t3, 0(a6)

        ret
          
# This function adds a window of specified color, size, position, to window list. 
# Arguments: a0 - x coord, a1 - y coord, a3 - window width, a4 - window height, a5 = color
add_window:
        # Save return adress
        addi sp,sp, -28
        sd ra, 0(sp)
        # Save arguments into temp registers before the malloc
        mv t1, a0
        mv t2, a1
        mv t3, a2
        mv t4, a3
        mv t5, a4

        sw t1, 8(sp)
        sw t2, 12(sp)
        sw t3, 16(sp)
        sw t4, 20(sp)
        sw t5, 24(sp)

        # Allocate memory for the window
        li a0,16                # 16 byte structure
        call malloc
          
        # If allocation failed, then exit
        bnez a0, add_window_continue

        ld ra, 0(sp)
        addi sp,sp,28
        ret

add_window_continue:

        lwu t1, 8(sp)
        lwu t2, 12(sp)
        lwu t3, 16(sp)
        lwu t4, 20(sp)
        lwu t5, 24(sp)
        
        # Store window data into new struct
        sh t1, 8(a0)
        sh t2, 10(a0)
        sb t3, 12(a0)
        sb t4, 13(a0)
        sh t5, 14(a0)
          
        # Add window to linkedlist
        la t0, skyline_win_list       # Load base address of linkedlist
        ld t1, 0(t0)                  # Load base address of current first window to t1
        sd t1, 0(a0)                  # Set the next pointer of new window to current first window address 
        sd a0, 0(t0)                  # Update the head of list to new window

        ld ra, 0(sp)
        addi sp,sp,28
        ret
          
# This function removes a window of specified position if exists. If window exists, remove from linked list and free. 
# Arguments: a0 - x coord, a1 - y coord
remove_window:
# Save return address 
        addi sp, sp, -8
        sd ra, 0(sp)

        la t0, skyline_win_list       # Load address of window linked list into t0
        ld t1, 0(t0)                    # Load head of list into t1
        beqz t1, remove_window_exit   # If head of list is emtpy then exit
          
        li t2, 0      # Initialize a previous pointer
remove_window_loop:
        beqz t1, remove_window_exit # If head of linked list is empty then we exit
        lh t3, 8(t1)                    # Load x-coord of current window into t3
        lh t4, 10(t1)                  # Load y-coord of current window into t3
          
        # Compare x and y with function arguments. If x and y dont match, then go to next window. 
        bne t3,a0, check_next_window
        bne t4, a1, check_next_window
          
        # If matching window is found, then remove it from the linked list. 
        ld t5, 0(t1)
        bnez t2, remove_window_continue

        # Matching window is head:
        sd t5,0(t0)                     
        mv a0,t1
        call free
        j remove_window_exit

remove_window_continue:
        # Set the next pointer of prev window to the next of current window
        sd t5, 0(t2)          
        mv a0,t1
        call free
        j remove_window_exit

# Iterate to next window
check_next_window:
        mv t2,t1
        ld t1, 0(t1)
        j remove_window_loop

remove_window_exit:
        ld ra, 0(sp)
        addi sp,sp,8
        ret

# This function draws given window into frame buffer. Iterate pixels and set respective pixel to color in win.
# Skip pixels whose locations are outside the screen.
# Arguments: a0 - pointer to frame buffer, a1 - pointer to window struct        
draw_window:
        # Load window coord, size, color into temp
        lh t0, 8(a1)
        lh t1, 10(a1)
        lb t2, 12(a1)
        lb t3, 13(a1)
        lh t4, 14(a1)
          
        li t5, 0 # Initialize row counter

draw_window_row:
        blt t5,t3, draw_window_start             # If row counter is bigger than window height then exit
        ret
draw_window_start:
        li a2,0        # Column Counter for next row

draw_window_col:
        bge a2,t2, draw_window_next_row

        # Calculate pixel coordinates
        add a4,t0,a2
        add a5, t1, t5
          
        # Ensure pixel is within window boundaries
        li a7, SKYLINE_WIDTH
        li a3, SKYLINE_HEIGHT
        bltu a4,a7, draw_window_continue

        addi a2,a2, 1        # Increment column
        j draw_window_col

        bltu a5,a3, draw_window_continue
        addi a2,a2, 1        # Increment column
        j draw_window_col

draw_window_continue:
        # Calculate offset in frame buffer

        mul a6, a5, a7
        add a6, a6, a4
        slli a6, a6, 1
          
        # Add memory offset to base address in frame buffer and store the color at this location
        add a6, a6, a0
        sh t4, 0(a6)

        addi a2,a2, 1        # Increment column
        j draw_window_col

draw_window_next_row:
        addi t5,t5,1
        j draw_window_row



# This function draws beacon when it is meant to be on, otherwise it is not drawn. If it should be drawn, 
# iterate over color data of beacon , copying color data into fbuf at the offset corresponding to 
# color data's on-screen coordinates. Ensure beacon is not drawn outside window. Determine if beacon should
# be drawn by comparing t with period and ontime. 
# Arguments: a0 - pointer to frame buffer, a1 - time 't', a2 - pointer to beacon struct
draw_beacon:
        # Save return address
        addi sp,sp, -8
        sd ra, 0(sp)

        # Load beacon's arguments to temp registers
        ld t0, 0(a2)
        lh t1, 8(a2)
        lh t2, 10(a2)
        lb t3, 12(a2)
        lh t4, 14(a2)
        lh t5, 16(a2)

        # Determine whether beacon on/off
        remu a3, a1, t4

        # If time is less than ontime then continue, else exit
        bltu a3, t5, beacon_start_row   
        ld ra,0(sp)
        addi sp, sp, 8
        ret


beacon_start_row:
        # Initialize row counter for beacon drawing
        li a4, 0
draw_beacon_row:
        blt a4, t3, beacon_start_col

        # If col count is less than window width then continue, else exit
        ld ra,0(sp)
        addi sp, sp, 8
        ret

beacon_start_col:
        # Initialize col counter
        li a5,0
draw_beacon_col:
        bge a5, t3, draw_beacon_next_row

        # Calculate the pixel coordinates
        add a6, t1, a5
        add a7, t2, a4

        # Ensure that the pixel is within boundaries
        li t4, SKYLINE_HEIGHT
        li t5, SKYLINE_WIDTH

        # Only continue if pixel is within boundaries; if not, skip.
        bltu a6, t5, beacon_continue
        addi a5,a5,1
        j draw_beacon_col

        bltu a7, t4, beacon_continue
        addi a5,a5,1
        j draw_beacon_col

beacon_continue:
        # Calculate the offset for beacon within the frame buffer
        mul t5, t5, a7
        add t5, t5, a6
        slli t5,t5,1

        # Add base frame buffer address with the offset to get final address for beacon
        add t5, t5, a0

        # Set pixel color from beacon's pointer
        lh a7, 0(t0)
        sh a7, 0(t5)

        # Increment the image pointer
        addi t0,t0,2

        addi a5,a5,1
        j draw_beacon_col

# Increment row counter.
draw_beacon_next_row:
        addi a4,a4,1
        j draw_beacon_row




        .end
