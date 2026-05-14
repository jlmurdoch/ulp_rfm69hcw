// Simple assembler program for the ESP32-S3 ULP to count pulse times into buckets

#include "soc/rtc_cntl_reg.h"
#include "soc/rtc_io_reg.h"
#include "soc/soc_ulp.h"

    .bss
    
    .global value_p
value_p:  
    .long 0

    .global value_0
value_0:  
    .long 0

    .global value_1
value_1:  
    .long 0

    .global value_s
value_s:  
    .long 0
    
    .text

    .global entry
entry:                      // (36 cycles)
                            // (8) Read GPIO low bank
    READ_RTC_REG(RTC_GPIO_IN_REG, RTC_GPIO_IN_NEXT_S, 16)
    rsh r0, r0, 11          // (6) Shift to GPIO
    and r0, r0, 1           // (6)  Is GPIO HIGH?
    add r3, r0, r2          // (6) Edge (r3 == 1) or same (r3 == 0 || 2)
    move r2, r0             // (6) Save level to r2
    and r3, r3, 1           // (6) Is it an edge (1)?     
    jump loop, eq           // (4) If not an edge, loop

edge:
    jumps bit_p, 30, lt     // 500 = pre
    jumps bit_0, 60, lt     // 1000 = zero
    jumps bit_1, 120, lt    // 2000 = one
    jumps bit_s, 240, lt    // 4000 = sync
    jump exit

loop:                       // (20 cycles)
    wait 320                // (6) wait
    stage_inc 1             // (6) counter++
    jumps exit, 240, GE     // (4) break()
    jump entry              // (4) Read again

bit_p:
    stage_rst               // Reset counter
    move r0, value_p        // Import variable
    ld r1, r0, 0            // Load bit count
    add r1, r1, 1           // Increment bit count
    move r0, value_p        // Import variable
    st r1, r0, 0            // Write bit count to variable
    jump loop

bit_0:
    stage_rst               // Reset counter 
    move r0, value_0        // Import variable
    ld r1, r0, 0            // Load bit count
    add r1, r1, 1           // Increment bit count
    move r0, value_0        // Import variable
    st r1, r0, 1            // Write bit count to variable
    jump loop

bit_1:
    stage_rst               // Reset counter
    move r0, value_1        // Import variable
    ld r1, r0, 0            // Load bit count
    add r1, r1, 1           // Increment bit count
    move r0, value_1        // Import variable
    st r1, r0, 0            // Write bit count to variable
    jump loop

bit_s:
    stage_rst               // Reset counter
    move r0, value_s        // Import variable
    ld r1, r0, 0            // Load bit count
    add r1, r1, 1           // Increment bit count
    move r0, value_s        // Import variable
    st r1, r0, 0            // Write bit count to variable
    jump loop

exit:
    // Cancel sleep
    WRITE_RTC_FIELD(RTC_CNTL_ULP_CP_TIMER_REG, RTC_CNTL_ULP_CP_SLP_TIMER_EN, 0)
    halt                    // finish up