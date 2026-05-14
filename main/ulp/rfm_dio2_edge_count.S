// Simple assembler program for the ESP32-S3 ULP to count edges

#include "soc/rtc_cntl_reg.h"
#include "soc/rtc_io_reg.h"
#include "soc/soc_ulp.h"

    .bss
    
    .global bit_count
bit_count:
    .long 0

    .text

    .global entry
entry:
    stage_rst           // Reset counter
    move r1, 0          // Edge counter
    move r2, 0          // Level state

read:                   // (36 cycles)
                        // (8) Read GPIO low bank
    READ_RTC_REG(RTC_GPIO_IN_REG, RTC_GPIO_IN_NEXT_S, 16)
    rsh r0, r0, 11      // (6) Shift to GPIO
    and r0, r0, 1       // (6) Is GPIO HIGH?
    add r3, r0, r2      // (6) Edge (r3 == 1) or same (r3 == 0 || 2)
    move r2, r0         // (6) Save level to r2
    and r3, r3, 1       // (6) Is it an edge (1)?     
    jump loop, eq       // (4) If not an edge, loop

edge:
    add r1, r1, 1       // Inc bit count
    move r3, bit_count  // Import variable
    st r1, r3, 0        // Write bit count to variable

loop:                   // (20 cycles)
    wait 320            // (6) pause
    stage_inc 1         // (6) increment counter
    jumps exit, 250, GE // (4) break()
    jump read           // (4) Read again

exit:
    // Cancel sleep
    WRITE_RTC_FIELD(RTC_CNTL_ULP_CP_TIMER_REG, RTC_CNTL_ULP_CP_SLP_TIMER_EN, 0)
    halt                    // finish up