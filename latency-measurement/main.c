#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include "vtimer.h"
#include "lpc23xx.h"
#include "lpc2387.h"
#include <math.h>

// copied from lpc2378-timer3.c
void  benchmark_init(void)
{
    PCLKSEL1 = (PCLKSEL1 & ~(BIT14|BIT15)) | (1 << 14); // CCLK to PCLK divider
    PCONP |= PCTIM3;
    T3TCR = 0;                                          // disable timer
    T3MCR = 0;                                          // disable interrupt
    T3CCR = 0;                                          // capture is disabled.
    T3EMR = 0;                                          // no external match output.
    T3PR = 0;                                           // set prescaler
    T3TC = 0;                                           // reset counter
}

inline void  benchmark_reset_start(void)
{
    T3TCR = 0;                                              // disable timer
    T3TC = 0;                                               // reset counter
    T3TCR = BIT0;
}

inline unsigned int  benchmark_read_stop(void)
{
    T3TCR = 0;                                              // disable timer
    return T3TC;
}

void sprint_double(char *buffer, double x, int precision) {
    long integral_part = (long) x;
    long exponent = (long) pow(10, precision);
    long decimal_part = (long) ((x - integral_part) * exponent);
    sprintf(buffer, "%ld.%ld", integral_part, decimal_part);
}

// This only works on the MSBA2
unsigned long get_clock_frequency(void) {
    assert(CLKSRCSEL == 0x001); // Make sure the main oscillator w/ 16MHz is used

    unsigned short pll_multiplier = PLLSTAT & 0xFFFF;
    pll_multiplier &= ~(1 << 15);
    pll_multiplier += 1; // Value stored in register is one less than multiplier

    unsigned char pre_divider = PLLSTAT;
    pre_divider = (pre_divider >> 16) & 0xFF;
    pre_divider += 1; // Value stored in register is one less than divider

    unsigned short clock_divider = CCLKCFG + 1; // Value stored in register is one less than divider

    // These values, based on the formula from the lpc23xx user manual
    //      "F_CCO = (2 * multiplier * input frequency) / pre_divider" and
    //      "CCLK (cpu clock) = F_CCO / clock_divider"
    // lead to an operating frequency of 72MHz
    assert(pre_divider == 1);
    assert(pll_multiplier == 9);
    assert(clock_divider == 4);

    return 72000000; // 72MHz
}

int main(void)
{
    printf("You are running RIOT on a(n) %s board.\n", RIOT_BOARD);
    printf("This board features a(n) %s MCU.\n", RIOT_MCU);

    timex_t sleep = timex_set(1, 0);

    // This is for native
    //uint64_t cycles_used = 0;
    // while(true) {
    //     asm volatile (
    //         "mfence\n\t"
    //         "rdtsc\n\t"
    //         : "=A" (cycles_used)
    //     );

    //     printf("Cycles used: %llu\n", cycles_used);
    //     vtimer_sleep(sleep);
    // }

    unsigned long clk = get_clock_frequency();
    printf("Clock frequency: %lu\n", clk);


    benchmark_init();
    unsigned int cycles;
    char buffer[128];

    //T3PR = clk / 1000000; // Set pre-scaler for microsecond (10^-6) accuracy
    int foo = 1;

    while(true) {
        benchmark_reset_start();
        foo *= 2;
        //printf("wtf\n");
        vtimer_sleep(sleep);
        cycles = benchmark_read_stop();


        double microseconds_taken = cycles * 1.0 / (clk/1000000.0);
        sprint_double(buffer, microseconds_taken, 2);
        printf("Cycles: %u, Result: %sµs, Iteration: %d\n", cycles, buffer, foo);
        vtimer_sleep(sleep);
    }

    return 0;
}
