#include "my_timer.h"
#include "FreeRTOS.h"
#include "driverlib.h"

#pragma PERSISTENT(elapsed_tick)
uint32_t elapsed_tick = 0;

//#pragma PERSISTENT(elapsed_tick_high)
//uint32_t elapsed_tick_high = 0;
#pragma PERSISTENT(cumulative_active_ticks)
uint64_t cumulative_active_ticks = 0;

#pragma PERSISTENT(segment_start_tick)
uint32_t segment_start_tick = 0;

// #define HIGH_RES_CLK TIMER_A_CLOCKSOURCE_SMCLK
// #define LOW_RES_CLK TIMER_A_CLOCKSOURCE_ACLK
// #define TIMER_CLK_SOURCE LOW_RES_CLK

void low_res_timer_init()
{
    Timer_A_initContinuousModeParam initContParam = {0};
//    initContParam.clockSource = TIMER_A_CLOCKSOURCE_ACLK;
    initContParam.clockSource = TIMER_A_CLOCKSOURCE_SMCLK;
    initContParam.clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_1;
    initContParam.timerInterruptEnable_TAIE = TIMER_A_TAIE_INTERRUPT_ENABLE;
    initContParam.timerClear = TIMER_A_DO_CLEAR;
    initContParam.startTimer = false;
    Timer_A_initContinuousMode(TIMER_A2_BASE, &initContParam);

    Timer_A_enableInterrupt(TIMER_A2_BASE);
}

//void high_res_timer_init()
//{
//    Timer_A_initContinuousModeParam initContParam = {0};
//    initContParam.clockSource = TIMER_A_CLOCKSOURCE_SMCLK;
//    initContParam.clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_2;
////    initContParam.timerInterruptEnable_TAIE = TIMER_A_TAIE_INTERRUPT_DISABLE;
//    initContParam.timerInterruptEnable_TAIE = TIMER_A_TAIE_INTERRUPT_ENABLE;
//
//    initContParam.timerClear = TIMER_A_DO_CLEAR;
//    initContParam.startTimer = false;
//    Timer_A_initContinuousMode(TIMER_A1_BASE, &initContParam);
//
////    Timer_A_disableInterrupt(TIMER_A1_BASE);
//    Timer_A_enableInterrupt(TIMER_A1_BASE);
//}

double get_elapsed_time(uint32_t start, uint32_t end, TIMER_TYPE type)
{
    if (end < start)
        return 0;
    
//    return (double)(end - start) / (type == HIGH_RES_CLK ? CS_getSMCLK() >> 1 : CS_getACLK());
//    return (double)(end - start) / (type == HIGH_RES_CLK ? CS_getSMCLK() >> 1 : CS_getSMCLK());
    return (double)(end - start) / CS_getSMCLK();
}

void low_res_timer_start()
{
    Timer_A_clear(TIMER_A2_BASE);
    Timer_A_startCounter(TIMER_A2_BASE, TIMER_A_CONTINUOUS_MODE);
    // Every boot lands here (cold start or post-restore) right after the
    // hardware register is reset -- mark "now" as the start of this boot's
    // first segment so bank_active_segment() measures only real elapsed
    // time, never a stale leftover value from before this reboot.
    segment_start_tick = get_current_tick(LOW_RES_CLK);
}

void bank_active_segment()
{
    uint32_t now = get_current_tick(LOW_RES_CLK);
    cumulative_active_ticks += (uint64_t)(now - segment_start_tick);
    segment_start_tick = now;
}

//void high_res_timer_start()
//{
//    Timer_A_clear(TIMER_A1_BASE);
//    Timer_A_startCounter(TIMER_A1_BASE, TIMER_A_CONTINUOUS_MODE);
//}

//uint32_t get_current_tick(TIMER_TYPE type)
//{
//    return (type == HIGH_RES_CLK ? (uint32_t)Timer_A_getCounterValue(TIMER_A1_BASE) + elapsed_tick_high : (uint32_t)Timer_A_getCounterValue(TIMER_A2_BASE) + elapsed_tick);
//}

uint32_t get_current_tick(TIMER_TYPE type) {
    uint32_t high1, high2, cnt;
//    if (type == LOW_RES_CLK) {
//        do {
//            high1 = elapsed_tick;
//            cnt   = (uint32_t)Timer_A_getCounterValue(TIMER_A2_BASE);
//            high2 = elapsed_tick;
//        } while (high1 != high2);
//        return cnt + high1;
//    }
//    else {
//        do {
//            high1 = elapsed_tick_high;
//            cnt   = (uint32_t)Timer_A_getCounterValue(TIMER_A1_BASE);
//            high2 = elapsed_tick_high;
//        } while (high1 != high2);
//        return cnt + high1;
//    }
    (void)type; // only LOW_RES_CLK exists now
    do {
        high1 = elapsed_tick;
        cnt   = (uint32_t)Timer_A_getCounterValue(TIMER_A2_BASE);
        high2 = elapsed_tick;
    } while (high1 != high2);
    return cnt + high1;
}

void low_res_timer_pause()
{
    Timer_A_stop(TIMER_A2_BASE);
}

void low_res_timer_resume()
{
    Timer_A_startCounter(TIMER_A2_BASE, TIMER_A_CONTINUOUS_MODE);
}

#pragma vector=TIMER2_A1_VECTOR
__interrupt void A2_ISR( void )
{
    if (TA2IV == 0xE)
        elapsed_tick += 0x10000;

    // TA2CTL &= ~TAIFG;

//  __bic_SR_register_on_exit( SCG1 + SCG0 + OSCOFF + CPUOFF );
}

#pragma vector=TIMER1_A1_VECTOR
__interrupt void A1_ISR(void)
{
    TA1CTL &= ~TAIFG;
}
