# ESP32 ULP RFM69HCW Example

Proof of concept to show how the ESP32 ULP-FSM can perform with signal processing.

In this example, the ESP32 ULP-FSM 17.5Mhz clock is tied to a RFM69HCW radio which can then read signals from a climate sensor.

Hardest part was figuring out is the timing, whereby pauses are done with `WAIT` and the cycles. 

Some assembler examples from various stages in the discovery process have been left for those wanting to develop something similar (count edges, count pulse lengths).

Key findings:
- ULP and its FSM mode need to has to be enabled in the ESP-IDF SDK Configuration:
```
CONFIG_ULP_COPROC_ENABLED=y
CONFIG_ULP_COPROC_TYPE_FSM=y
```
- For ULP storage there is:
    - Four 16-bit registers: `r0`, `r1`, `r2` & `r3`
    - An 8-bit `stage_cnt` counter (can only be used with the `JUMPS` instruction)
    - Default of 4KiB `RTC_SLOW_MEM` for using the `ST` store and `LD` load instructions. 
- Precise ULP FSM cycle timings at 17.5MHz result in the following timings:

| 16-bit Cycles  | Time Elapsed       |
|----------------|--------------------|
| 175 cycles     | 10 microseconds    |
| 17,500 cycles  | 1.0 millisecond    |
| 52,500 cycles  | 3.0 milliseconds   |
| 64,750 cycles  | 3.7 milliseconds   |
| 65,535 cycles  | 3.745 milliseconds |

- Example of a 2 x 16-bit element array. It is actually 32-bit elements, but the first two bytes contain the instruction and data passed:
```
        .global array
array:  .long 0 
        .long 0
```
It can then be used in the C/C++ and ULP assembler as follows:

| C/C++                       | ULP Assembler (Load)     | ULP Assembler (Store)   |
|-----------------------------|--------------------------|-------------------------|
| `(uint16_t)(&ulp_value)[0]` | `ld Rdst, Rsrc, 0`       | `st Rsrc, Rdst, 0`      |
| `(uint16_t)(&ulp_value)[1]` | `ld Rdst, Rsrc, 4`       | `st Rsrc, Rdst, 4`      |
