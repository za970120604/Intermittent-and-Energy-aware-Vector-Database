#include "driverlib.h"
#include <stdlib.h>
#include <string.h>
#include "utils.h"
#include "checkpoint.h"
#include "my_timer.h"
#include "debug.h"
#include "config.h"
#include "vectors_config.h"
#include "insert_ops.h"
#include "delete_ops.h"
#include "query_ops.h"

extern BREAKPOINT_TYPE breakpoint;
extern Statistics stats;
extern PHASE phase;

extern uint32_t stat_timer_1_start;
extern uint32_t stat_timer_1_end;
extern uint32_t stat_timer_2_start;
extern uint32_t stat_timer_2_end;
extern uint16_t checkpoint_countdown;

extern uint8_t ucHeap[configTOTAL_HEAP_SIZE];
extern void snapshotReg();
extern void restoreReg();
extern uint32_t replay_head;

#pragma PERSISTENT(last_phase)
PHASE last_phase = PHASE_NONE;

#pragma PERSISTENT(snapshot)
Snapshot snapshot[SNAPSHOT_SLOT_COUNT] = {0};

#pragma PERSISTENT(snapshot_head)
uint8_t snapshot_head = 0;

void *snapshot_reg;

#pragma PERSISTENT(rtc_calendar_param)
// Calendar rtc_calendar_param = {.Seconds = 0x59,
//                                .Minutes = 0x0,
//                                .Hours = 0x0,
//                                .DayOfWeek = 0x0,
//                                .DayOfMonth = 0x0,
//                                .Month = 0x0,
//                                .Year = 0x0
//                                };
Calendar rtc_calendar_param = {.Seconds = 0x52,
                               .Minutes = 0x0,
                               .Hours = 0x0,
                               .DayOfWeek = 0x0,
                               .DayOfMonth = 0x0,
                               .Month = 0x0,
                               .Year = 0x0
                               };

Calendar rtc_calendar_param1 = {.Seconds = 0x59,
                               .Minutes = 0x0,
                               .Hours = 0x0,
                               .DayOfWeek = 0x0,
                               .DayOfMonth = 0x0,
                               .Month = 0x0,
                               .Year = 0x0
                               };


#pragma PERSISTENT(restart_count_this_query);
uint32_t restart_count_this_query = 0;
#pragma PERSISTENT(distinct_completed_count)
uint32_t distinct_completed_count = 0;
#pragma PERSISTENT(distinct_miss_count)
uint32_t distinct_miss_count = 0;

#if EXPERIMENT == EXP_BASELINE
#pragma PERSISTENT(in_energy_wait)
uint8_t in_energy_wait = 0;
#pragma PERSISTENT(wait_for_energy_count)
uint32_t wait_for_energy_count = 0;
#endif

void checkpoint(CHECKPOINT_TYPE type)
{
    ++stats.count[PHASE_CHECKPOINT];
    bank_active_segment();

    Snapshot *open_snapshot = &snapshot[snapshot_head];
    open_snapshot->status = COMMIT_INCOMPLETE;
    open_snapshot->type = type;

    /*******************************************/
    /*              Storage space              */
    /*******************************************/

    portENTER_CRITICAL();
    open_snapshot->replay_head = replay_head;
    open_snapshot->saved_n_query  = n_query;
    open_snapshot->saved_n_hops   = n_hops;
    open_snapshot->saved_n_addL_new = n_addL_new;
    open_snapshot->saved_n_addL_dup = n_addL_dup;
    open_snapshot->saved_tick_nbr_io = tick_nbr_io;
    open_snapshot->saved_tick_nbr_loop = tick_nbr_loop;
    open_snapshot->saved_tick_prune = tick_prune;
    open_snapshot->saved_tick_hc_new = tick_hc_new;
    open_snapshot->saved_tick_hc_dup = tick_hc_dup;
    open_snapshot->saved_tick_get_closest = tick_get_closest;
    open_snapshot->saved_tick_compute_table = tick_compute_table;
    open_snapshot->saved_tick_searchlist_expand = tick_searchlist_expand;
    open_snapshot->saved_tick_rerank_answer = tick_rerank_answer;
    open_snapshot->saved_tick_rerank_io = tick_rerank_io;
    open_snapshot->saved_tick_rerank_cmp = tick_rerank_cmp;
    open_snapshot->saved_tick_rerank_sort = tick_rerank_sort;

#if SPI_DEBUG_MODE == SPI_DEBUG_MODE_ON
    open_snapshot->saved_tick_pqcode_page_read_cmd = tick_pqcode_page_read_cmd;
    open_snapshot->saved_tick_pqcode_page_read_poll_for_oip = tick_pqcode_page_read_poll_for_oip;
    open_snapshot->saved_tick_pqcode_cache_read = tick_pqcode_cache_read;
    open_snapshot->saved_tick_pqtable_lookup = tick_pqtable_lookup;
    open_snapshot->saved_tick_sorted_insert = tick_sorted_insert;
    open_snapshot->saved_tick_other_cmp = tick_other_cmp;
    open_snapshot->saved_tick_dma_register = tick_dma_register;
    open_snapshot->saved_tick_dma_wait = tick_dma_wait;
#endif
    portEXIT_CRITICAL();

    /*******************************************/
    /*              Program space              */
    /*******************************************/

    snapshot_reg = &open_snapshot->registers;

    /* Backup SRAM and registers */
    portENTER_CRITICAL();
    DMA_transfer((uint8_t *)__bss__, (uint8_t *)open_snapshot->bss, BSS_SIZE);
    DMA_transfer((uint8_t *)__data__, (uint8_t *)open_snapshot->data, DATA_SIZE);
    DMA_transfer((uint8_t *)ucHeap, (uint8_t *)open_snapshot->heap, UCHEAP_SIZE);
    snapshotReg();
    portEXIT_CRITICAL();

    /*******************************************/
    /*             Clear Undo Log              */
    /*******************************************/

    if (phase == PHASE_RESTORE) // from restore
        return;

    /* Commit completion flag & update snapshot head */

    portENTER_CRITICAL();
    snapshot_head ^= 1;
    open_snapshot->status = COMMIT_COMPLETE;
    portEXIT_CRITICAL();
}

void restore() {
    ++stats.count[PHASE_RESTORE];

    Snapshot *complete_snapshot = &snapshot[snapshot_head^1];

    if (complete_snapshot->status == COMMIT_NULL)
    {
        stat_timer_1_end = get_current_tick(LOW_RES_CLK);
        stats.elapsed_time[PHASE_RESTORE] += get_elapsed_time(stat_timer_1_start, stat_timer_1_end, LOW_RES_CLK);
        return;
    }

    if (complete_snapshot->status == COMMIT_INCOMPLETE) {
        SET_BREAKPOINT(BP_ERROR_CKPT);
    }
#if EXPERIMENT == EXP_BASELINE
    if (!in_energy_wait) {
        ++restart_count_this_query;
    }
#else
        ++restart_count_this_query;
#endif

    snapshot_reg = &complete_snapshot->registers;

    /*******************************************/
    /*              Storage space              */
    /*******************************************/
    portENTER_CRITICAL();
    replay_head = complete_snapshot->replay_head;
    n_query  = complete_snapshot->saved_n_query;
    n_hops   = complete_snapshot->saved_n_hops;
    n_addL_new = complete_snapshot->saved_n_addL_new;
    n_addL_dup = complete_snapshot->saved_n_addL_dup;
    tick_nbr_io = complete_snapshot->saved_tick_nbr_io;
    tick_nbr_loop = complete_snapshot->saved_tick_nbr_loop;
    tick_prune = complete_snapshot->saved_tick_prune;
    tick_hc_new = complete_snapshot->saved_tick_hc_new;
    tick_hc_dup = complete_snapshot->saved_tick_hc_dup;
    tick_get_closest = complete_snapshot->saved_tick_get_closest;
    tick_compute_table = complete_snapshot->saved_tick_compute_table;
    tick_searchlist_expand = complete_snapshot->saved_tick_searchlist_expand;
    tick_rerank_answer = complete_snapshot->saved_tick_rerank_answer;
    tick_rerank_io = complete_snapshot->saved_tick_rerank_io;
    tick_rerank_cmp = complete_snapshot->saved_tick_rerank_cmp;
    tick_rerank_sort = complete_snapshot->saved_tick_rerank_sort;

#if SPI_DEBUG_MODE == SPI_DEBUG_MODE_ON
    tick_pqcode_page_read_cmd = complete_snapshot->saved_tick_pqcode_page_read_cmd;
    tick_pqcode_page_read_poll_for_oip = complete_snapshot->saved_tick_pqcode_page_read_poll_for_oip;
    tick_pqcode_cache_read = complete_snapshot->saved_tick_pqcode_cache_read;
    tick_pqtable_lookup = complete_snapshot->saved_tick_pqtable_lookup;
    tick_sorted_insert = complete_snapshot->saved_tick_sorted_insert;
    tick_other_cmp = complete_snapshot->saved_tick_other_cmp;
    tick_dma_register = complete_snapshot->saved_tick_dma_register;
    tick_dma_wait = complete_snapshot->saved_tick_dma_wait;
#endif
    portEXIT_CRITICAL();

    /*******************************************/
    /*              Program space              */
    /*******************************************/
    portENTER_CRITICAL();

    /* Restore SRAM and registers */
    DMA_transfer((uint8_t *)complete_snapshot->bss, (uint8_t *)__bss__, BSS_SIZE);
    DMA_transfer((uint8_t *)complete_snapshot->data, (uint8_t *)__data__, DATA_SIZE);
    DMA_transfer((uint8_t *)complete_snapshot->heap, (uint8_t *)ucHeap, UCHEAP_SIZE);

    stat_timer_1_end = get_current_tick(LOW_RES_CLK);
    double elapsed_time = get_elapsed_time(stat_timer_1_start, stat_timer_1_end, LOW_RES_CLK);
    stats.elapsed_time[PHASE_RESTORE] += elapsed_time;

#ifdef STAT_TIME_MIN_MAX
    stats.min_time[PHASE_RESTORE] = min(stats.min_time[PHASE_RESTORE], elapsed_time);
    stats.max_time[PHASE_RESTORE] = max(stats.max_time[PHASE_RESTORE], elapsed_time);
#endif

    restoreReg();

    /* Never reach here */
    portEXIT_CRITICAL();
}

void shutdown()
{
    portENTER_CRITICAL();

#if EXPERIMENT == EXP_POWER_EVENT
    RTC_C_disableInterrupt(RTC_C_BASE, RTC_C_TIME_EVENT_INTERRUPT + RTC_C_CLOCK_ALARM_INTERRUPT + RTC_C_CLOCK_READ_READY_INTERRUPT + RTC_C_OSCILLATOR_FAULT_INTERRUPT);
    RTC_C_clearInterrupt(RTC_C_BASE, RTC_C_TIME_EVENT_INTERRUPT + RTC_C_CLOCK_ALARM_INTERRUPT + RTC_C_CLOCK_READ_READY_INTERRUPT + RTC_C_OSCILLATOR_FAULT_INTERRUPT);
    RTC_C_initCalendar(RTC_C_BASE, &rtc_calendar_param, RTC_C_FORMAT_BCD);
    RTC_C_setCalendarEvent(RTC_C_BASE, RTC_C_CALENDAREVENT_MINUTECHANGE);
    RTC_C_enableInterrupt(RTC_C_BASE, RTC_C_TIME_EVENT_INTERRUPT);
#endif

    PMM_turnOffRegulator();
    PMM_disableSVSH();

#if EXPERIMENT == EXP_POWER_EVENT
    RTC_C_startClock(RTC_C_BASE);
#endif
    LPM4;

    portEXIT_CRITICAL();
}

#if EXPERIMENT == EXP_BASELINE
void arm_rtc_wakeup_ms(uint32_t off_ms) {
    uint64_t wake_ticks64;
    uint32_t wake_ticks;
    uint32_t preload;

    wake_ticks64 = ((uint64_t)off_ms * 32768UL + 999ULL) / 1000ULL;
    if (wake_ticks64 < 1ULL) {
        wake_ticks64 = 1ULL;
    }
    if (wake_ticks64 > 0xFFFFFFFFULL) {
        wake_ticks64 = 0xFFFFFFFFULL;
    }
    wake_ticks = (uint32_t)wake_ticks64;
    preload = (uint32_t)(0UL - wake_ticks);

    RTC_C_holdClock(RTC_C_BASE);
    RTC_C_disableInterrupt(RTC_C_BASE, RTC_C_TIME_EVENT_INTERRUPT);
    RTC_C_clearInterrupt(RTC_C_BASE, RTC_C_TIME_EVENT_INTERRUPT);

    RTCCTL0_H = RTCKEY_H;
    RTC_C_initCounter(RTC_C_BASE, RTC_C_CLOCKSELECT_32KHZ_OSC, RTC_C_COUNTERSIZE_32BIT);
    RTC_C_setCounterValue(RTC_C_BASE, preload);
    RTCCTL0_H = 0x00;

    RTC_C_clearInterrupt(RTC_C_BASE, RTC_C_TIME_EVENT_INTERRUPT);
    RTC_C_enableInterrupt(RTC_C_BASE, RTC_C_TIME_EVENT_INTERRUPT);
}
void wait_for_energy(void) {
    portENTER_CRITICAL();
    in_energy_wait = 1;
    ++wait_for_energy_count;

#if ENERGY_WAIT_METHOD == ENERGY_WAIT_METHOD_CALENDAR
    RTC_C_disableInterrupt(RTC_C_BASE, RTC_C_TIME_EVENT_INTERRUPT + RTC_C_CLOCK_ALARM_INTERRUPT + RTC_C_CLOCK_READ_READY_INTERRUPT + RTC_C_OSCILLATOR_FAULT_INTERRUPT);
    RTC_C_clearInterrupt(RTC_C_BASE, RTC_C_TIME_EVENT_INTERRUPT + RTC_C_CLOCK_ALARM_INTERRUPT + RTC_C_CLOCK_READ_READY_INTERRUPT + RTC_C_OSCILLATOR_FAULT_INTERRUPT);
    RTC_C_initCalendar(RTC_C_BASE, &rtc_calendar_param1, RTC_C_FORMAT_BCD);
    RTC_C_setCalendarEvent(RTC_C_BASE, RTC_C_CALENDAREVENT_MINUTECHANGE);
    RTC_C_enableInterrupt(RTC_C_BASE, RTC_C_TIME_EVENT_INTERRUPT);
#elif ENERGY_WAIT_METHOD == ENERGY_WAIT_METHOD_RT1PS
    RTC_C_disableInterrupt(RTC_C_BASE, RTC_C_TIME_EVENT_INTERRUPT + RTC_C_CLOCK_ALARM_INTERRUPT +
        RTC_C_CLOCK_READ_READY_INTERRUPT + RTC_C_OSCILLATOR_FAULT_INTERRUPT +
        RTC_C_PRESCALE_TIMER0_INTERRUPT + RTC_C_PRESCALE_TIMER1_INTERRUPT);
    RTC_C_clearInterrupt(RTC_C_BASE, RTC_C_TIME_EVENT_INTERRUPT + RTC_C_CLOCK_ALARM_INTERRUPT +
        RTC_C_CLOCK_READ_READY_INTERRUPT + RTC_C_OSCILLATOR_FAULT_INTERRUPT +
        RTC_C_PRESCALE_TIMER0_INTERRUPT + RTC_C_PRESCALE_TIMER1_INTERRUPT);

    RTCCTL0_H = RTCKEY_H;
    RTC_C_initCounter(RTC_C_BASE, RTC_C_CLOCKSELECT_32KHZ_OSC, RTC_C_COUNTERSIZE_16BIT);

    RTCCTL13 &= ~(RTCMODE);

    RTC_C_initCounterPrescale(RTC_C_BASE, RTC_C_PRESCALE_1, RTC_C_PSCLOCKSELECT_ACLK, RTC_C_PSDIVIDER_256);
    RTC_C_definePrescaleEvent(RTC_C_BASE, RTC_C_PRESCALE_1, RTC_C_PSEVENTDIVIDER_32);
    RTC_C_startCounterPrescale(RTC_C_BASE, RTC_C_PRESCALE_1);

    RTCCTL0_H = 0x00;
    RTC_C_enableInterrupt(RTC_C_BASE, RTC_C_PRESCALE_TIMER1_INTERRUPT);

    debug_rtcctl13_after_init = RTCCTL13;
    debug_rtcps1ctl_after_init = RTCPS1CTL;
#else
    arm_rtc_wakeup_ms(RECHARGE_TIME_MS);
#endif

    PMM_turnOffRegulator();
    PMM_disableSVSH();
    RTC_C_startClock(RTC_C_BASE);
    LPM4;
    portEXIT_CRITICAL();
}
#endif

void CHECKPOINT(CHECKPOINT_TYPE type)
{
    phase = PHASE_CHECKPOINT;
//    checkpoint_countdown = CHECKPOINT_INTERVAL;

    portENTER_CRITICAL();
//    high_res_timer_start();
    stat_timer_2_start = get_current_tick(LOW_RES_CLK);
    checkpoint(type);
    if (phase == PHASE_CHECKPOINT)
    {
        stat_timer_2_end = get_current_tick(LOW_RES_CLK);
        double elapsed_time = get_elapsed_time(stat_timer_2_start, stat_timer_2_end, LOW_RES_CLK);
        stats.elapsed_time[PHASE_CHECKPOINT] += elapsed_time;
#ifdef STAT_TIME_MIN_MAX
        stats.min_time[PHASE_CHECKPOINT] = min(stats.min_time[PHASE_CHECKPOINT], elapsed_time);
        stats.max_time[PHASE_CHECKPOINT] = max(stats.max_time[PHASE_CHECKPOINT], elapsed_time);
#endif
    }
    portEXIT_CRITICAL();
}

void RESTORE()
{
    stats.power_event_phase[phase]++;
    phase = PHASE_RESTORE;

    stat_timer_1_start = get_current_tick(LOW_RES_CLK);
    restore();
}

#pragma vector=RTC_VECTOR
__interrupt void RTC_ISR( void )
{
//#if ENERGY_WAIT_METHOD == ENERGY_WAIT_METHOD_CALENDAR
    RTC_C_clearInterrupt(RTC_C_BASE, RTC_C_TIME_EVENT_INTERRUPT + RTC_C_CLOCK_ALARM_INTERRUPT + RTC_C_CLOCK_READ_READY_INTERRUPT + RTC_C_OSCILLATOR_FAULT_INTERRUPT);
//    ++rtc_isr_fire_count;
//#else
//    RTC_C_clearInterrupt(RTC_C_BASE, RTC_C_TIME_EVENT_INTERRUPT + RTC_C_CLOCK_ALARM_INTERRUPT + RTC_C_CLOCK_READ_READY_INTERRUPT + RTC_C_OSCILLATOR_FAULT_INTERRUPT + RTC_C_PRESCALE_TIMER0_INTERRUPT + RTC_C_PRESCALE_TIMER1_INTERRUPT);
//#endif
}

// For time based power event experiment
void setup_power_event_timer()
{
    Timer_A_initUpModeParam initUpParam = {0};
    initUpParam.clockSource = TIMER_A_CLOCKSOURCE_ACLK;

    /* To configure the power event interval, compute and set these two values down below */
    initUpParam.clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_64;
    initUpParam.timerPeriod = (CS_getACLK() >> 6) * POWER_EVENT_INTERVAL;

    initUpParam.timerInterruptEnable_TAIE = TIMER_A_TAIE_INTERRUPT_ENABLE;
    initUpParam.captureCompareInterruptEnable_CCR0_CCIE = TIMER_A_CCIE_CCR0_INTERRUPT_DISABLE;
    initUpParam.timerClear = TIMER_A_DO_CLEAR;
    initUpParam.startTimer = false;
    Timer_A_initUpMode(TIMER_A3_BASE, &initUpParam);

    Timer_A_clear(TIMER_A3_BASE);
    Timer_A_startCounter(TIMER_A3_BASE, TIMER_A_UP_MODE);
}

void disable_power_event_timer()
{
    Timer_A_stop(TIMER_A3_BASE);
}

#pragma vector=TIMER3_A1_VECTOR
__interrupt void A3_ISR( void )
{
    TA3CTL &= ~TAIFG;

    last_phase = phase;
    stats.power_event_phase[last_phase]++;
    shutdown();

    __bic_SR_register_on_exit( SCG1 + SCG0 + OSCOFF + CPUOFF );
}


