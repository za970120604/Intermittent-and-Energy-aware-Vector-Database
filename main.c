/*
 * FreeRTOS V202107.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/******************************************************************************
 * This project provides two demo applications.  A simple blinky style project,
 * and a more comprehensive test and demo application.  The
 * CHECKPOINT_TEST setting (defined in this file) is used to
 * select between the two.  The simply blinky demo is implemented and described
 * in main_blinky.c.  The more comprehensive test and demo application is
 * implemented and described in main_full.c.
 *
 * This file implements the code that is not demo specific, including the
 * hardware setup and standard FreeRTOS hook functions.
 *
 * ENSURE TO READ THE DOCUMENTATION PAGE FOR THIS PORT AND DEMO APPLICATION ON
 * THE http://www.FreeRTOS.org WEB SITE FOR FULL INFORMATION ON USING THIS DEMO
 * APPLICATION, AND ITS ASSOCIATE FreeRTOS ARCHITECTURE PORT!
 *
 */

/* Scheduler include files. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* Standard demo includes, used so the tick hook can exercise some FreeRTOS
 functionality in an interrupt. */
#include "EventGroupsDemo.h"
#include "TaskNotify.h"
//#include "ParTest.h" /* LEDs - a historic name for "Parallel Port". */

/* TI includes. */
#include "driverlib.h"
#include <stdio.h>
#include <stdlib.h>

#include "checkpoint.h"
#include "my_timer.h"
#include "spi_nand.h"
#include "graphsearch.h"
#include "debug.h"
#include "config.h"
#include "adc.h"
#include "workload.h"
#include "DSPLib.h"
#include "framctl_a.h"
/*-----------------------------------------------------------*/

/*
 * Configure the hardware as necessary to run this demo.
 */
static int prvSetupHardware(void);

#if EXPERIMENT == EXP_GEN_WORKLOAD
extern void main_workload();
#else
extern void main_graphsearch(void);
#endif 

/* Prototypes for the standard FreeRTOS callback/hook functions implemented
 within this file. */
void vApplicationMallocFailedHook(void);
void vApplicationIdleHook(void);
void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName);
void vApplicationTickHook(void);

/* The heap is allocated here so the "persistent" qualifier can be used.  This
 requires configAPPLICATION_ALLOCATED_HEAP to be set to 1 in FreeRTOSConfig.h.
 See http://www.freertos.org/a00111.html for more information. */
#ifdef __ICC430__
	__persistent 					/* IAR version. */
#else
#pragma PERSISTENT( ucHeap ) 	/* CCS version. */
#endif
uint8_t ucHeap[configTOTAL_HEAP_SIZE] = { 0 };

extern DMA_initParam dma_param;

extern PHASE phase;

#pragma PERSISTENT(clear_all_blocks_flag)
uint8_t clear_all_blocks_flag = 0;

#pragma PERSISTENT(halt_flag)
uint8_t halt_flag = 1;

#pragma PERSISTENT(cycle_query_count)
uint32_t cycle_query_count = 0;
#pragma PERSISTENT(total_power_cycles)
uint32_t total_power_cycles = 0;
#pragma PERSISTENT(power_cycle_arr)
uint32_t power_cycle_arr[2000] = {0};

typedef enum {
    CKPT_ENTRY = 0,          // t0¡Glow_res_timer
    CKPT_AFTER_LFXT,         // CS_turnOnLFXT()
    CKPT_AFTER_DMA_INIT,     // DMA_init()
    CKPT_AFTER_TICK_TIMER,   // vApplicationSetupTimerInterrupt()
    CKPT_AFTER_ADC_INIT,     // init_adc()
    CKPT_AFTER_SPI_GPIO,     // UCx_MODE SPI/GPIO
    CKPT_AFTER_NAND_INIT,    // spi_nand_init()
    CKPT_COUNT
} setup_ckpt_t;

#pragma PERSISTENT(hw_setup_ticks)
uint32_t hw_setup_ticks[CKPT_COUNT] = {0};
#pragma PERSISTENT(hw_setup_cum_ticks)
uint64_t hw_setup_cum_ticks[CKPT_COUNT - 1] = {0};
#pragma PERSISTENT(hw_setup_sample_count)
uint32_t hw_setup_sample_count = 0;

static void halt()
{
    P1DIR |= 0x0003; // Set P1.0 and P1.1 to output direction

    PM5CTL0 &= ~LOCKLPM5; // Disable the GPIO power-on default high-impedance mode
    // to activate previously configured port settings

    P1OUT = 0x0000;
    P5DIR &= ~BIT5;
    P5REN |= BIT5;

    P1OUT |= BIT0; // light up the LED

    while (1)
    {
        if (!(P5IN & BIT5))
        {
            P1OUT &= ~BIT0;
            halt_flag = 0;
            break;
        }
    }
}

/*-----------------------------------------------------------*/

int main(void)
{
    /* See http://www.FreeRTOS.org/MSP430FR5969_Free_RTOS_Demo.html */

    /* Configure the hardware ready to run the demo. */
    if (prvSetupHardware()) {
        SET_BREAKPOINT(BP_ERROR);
    }

#if EXPERIMENT == EXP_BASELINE
    if (SYSRSTIV != SYSRSTIV_LPM5WU) {
#endif

    if (clear_all_blocks_flag == 0) {
        phase = PHASE_CLEAR_BLOCKS;
        clear_all_blocks();
        clear_all_blocks_flag = 1;
    }

#if EXPERIMENT == EXP_BASELINE || EXPERIMENT == EXP_REPLAY
//     if (halt_flag == 1)
//         halt();
#endif

     if (total_power_cycles == 2000) {
         total_power_cycles = 0;
     }
     power_cycle_arr[total_power_cycles++] = cycle_query_count;
     cycle_query_count = 0;

#if EXPERIMENT == EXP_BASELINE
    }
#endif

#if EXPERIMENT == EXP_GEN_WORKLOAD
    main_workload();
#else
    RESTORE(); // Must be placed after HW setup since CLK, DMA, SPI need to be init.

    main_graphsearch();
#endif
    return 0;
}
/*-----------------------------------------------------------*/

void vApplicationMallocFailedHook(void)
{
    /* Called if a call to pvPortMalloc() fails because there is insufficient
     free memory available in the FreeRTOS heap.  pvPortMalloc() is called
     internally by FreeRTOS API functions that create tasks, queues, software
     timers, and semaphores.  The size of the FreeRTOS heap is set by the
     configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */

    /* Force an assert. */
    configASSERT(( volatile void * ) NULL);
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName)
{
    (void) pcTaskName;
    (void) pxTask;

    /* Run time stack overflow checking is performed if
     configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
     function is called if a stack overflow is detected.
     See http://www.freertos.org/Stacks-and-stack-overflow-checking.html */

    /* Force an assert. */
    configASSERT(( volatile void * ) NULL);
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook(void)
{
    __bis_SR_register( LPM4_bits + GIE);
    __no_operation();
}
/*-----------------------------------------------------------*/

void vApplicationTickHook(void)
{
    // #if( CHECKPOINT_TEST == 0 )
    // {
    // 	/* Call the periodic event group from ISR demo. */
    // 	vPeriodicEventGroupsProcessing();

    // 	/* Call the code that 'gives' a task notification from an ISR. */
    // 	xNotifyTaskFromISR();
    // }
    // #endif
}
/*-----------------------------------------------------------*/

/* The MSP430X port uses this callback function to configure its tick interrupt.
 This allows the application to choose the tick interrupt source.
 configTICK_VECTOR must also be set in FreeRTOSConfig.h to the correct
 interrupt vector for the chosen tick interrupt source.  This implementation of
 vApplicationSetupTimerInterrupt() generates the tick from timer A0, so in this
 case configTICK_VECTOR is set to TIMER0_A0_VECTOR. */
void vApplicationSetupTimerInterrupt(void)
{
    const unsigned short usACLK_Frequency_Hz = 32768;

    /* Ensure the timer is stopped. */
    TA0CTL = 0;

    /* Run the timer from the ACLK. */
    TA0CTL = TASSEL_1;

    /* Clear everything to start with. */
    TA0CTL |= TACLR;

    /* Set the compare match value according to the tick rate we want. */
    TA0CCR0 = usACLK_Frequency_Hz / configTICK_RATE_HZ;

    /* Enable the interrupts. */
    TA0CCTL0 = CCIE;

    /* Start up clean. */
    TA0CTL |= TACLR;

    /* Up mode. */
    TA0CTL |= MC_1;
}
/*-----------------------------------------------------------*/

static int prvSetupHardware(void)
{
    /* Stop Watchdog timer. */
    WDT_A_hold( __MSP430_BASEADDRESS_WDT_A__);

    /* Disable RTC */
    RTC_C_holdClock(RTC_C_BASE);

    /* Set PJ.4 and PJ.5 for LFXT. */
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_PJ,
                                               GPIO_PIN4 + GPIO_PIN5,
                                               GPIO_PRIMARY_MODULE_FUNCTION);

    /* Set DCO frequency to 8 MHz. */
    CS_setDCOFreq( CS_DCORSEL_0, CS_DCOFSEL_6);

    /* Set external clock frequency to 32.768 KHz. */
    CS_setExternalClockSource(32768, 0);

    /* Set ACLK = LFXT. */
    CS_initClockSignal( CS_ACLK, CS_LFXTCLK_SELECT, CS_CLOCK_DIVIDER_1);

    /* Set SMCLK = DCO with frequency divider of 1. */
    CS_initClockSignal( CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);

    /* Set MCLK = DCO with frequency divider of 1. */
    CS_initClockSignal( CS_MCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);

//    low_res_timer_init();
//    low_res_timer_start();
//    __enable_interrupt(); // make TA2 overflow ISR runnable
//    hw_setup_ticks[CKPT_ENTRY] = get_current_tick(LOW_RES_CLK);

    /* Start XT1 with no time out. */
    CS_turnOnLFXT(CS_LFXT_DRIVE_0);
//    hw_setup_ticks[CKPT_AFTER_LFXT] = get_current_tick(LOW_RES_CLK);

    /* Disable the GPIO power-on default high-impedance mode. */
    PMM_unlockLPM5();

// DMA channel assignment:
// UCB_MODE_UCB1 / UCA_MODE_UCA3 / default: CH0 = FRAM-to-FRAM snapshot
// UCB_MODE_UCB0 + SPI_MODE_DMA:  CH2 = FRAM-to-FRAM snapshot; CH0 = UCB0 RX; CH1 = UCB0 TX (set in spi.c)
// UCA_MODE_UCA3 + SPI_MODE_DMA:  CH0 = FRAM-to-FRAM snapshot; CH4 = UCA3 RX; CH5 = UCA3 TX (set in spi.c)
#if UCx_MODE == UCB_MODE_UCB0
    dma_param.channelSelect = DMA_CHANNEL_2;
#else
    dma_param.channelSelect = DMA_CHANNEL_0;
#endif

    dma_param.transferModeSelect = DMA_TRANSFER_BLOCK;
    dma_param.transferUnitSelect = DMA_SIZE_SRCWORD_DSTWORD;
    DMA_init(&dma_param);

    /* Setup Timer */
    low_res_timer_init();
    low_res_timer_start();
//    high_res_timer_init();
//    high_res_timer_start();

    /* Setup FreeRTOS tick timer */
//    hw_setup_ticks[CKPT_AFTER_DMA_INIT] = get_current_tick(LOW_RES_CLK);
    vApplicationSetupTimerInterrupt();
//    hw_setup_ticks[CKPT_AFTER_TICK_TIMER] = get_current_tick(LOW_RES_CLK);

#ifdef POWER_EVENT_ON
    setup_power_event_timer();
#endif

    init_adc();
//    hw_setup_ticks[CKPT_AFTER_ADC_INIT] = get_current_tick(LOW_RES_CLK);

#if SAMPLE_RATE > 0
    setup_sample_rate_timer();
#endif

    /* ----------------------------------------------------------------
     * SPI peripheral setup
     *
     * UCB_MODE_UCB1:
     *   eUSCI_B1 (P5.0/P5.1/P5.2)  -> NAND flash,  CS = P4.1
     *   eUSCI_B0 (P1.6/P1.7/P2.2)  -> SD card,     CS = P4.0
     *
     * UCB_MODE_UCB0 (DMA, needs rewire):
     *   eUSCI_B0 (P1.6/P1.7/P2.2)  -> NAND flash,  CS = P4.1
     *   SD card not used / not initialized
     *
     * UCA_MODE_UCA3 (DMA, no rewire needed):
     *   eUSCI_A3 (P6.0/P6.1/P6.2)  -> NAND flash, CS = P4.1
      ----------------------------------------------------------------*/

#if UCx_MODE == UCB_MODE_UCB1
    /* Setup SPI (eUSCI_B1) for Nand*/
    GPIO_setAsPeripheralModuleFunctionInputPin(
            GPIO_PORT_P5, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2,
            GPIO_PRIMARY_MODULE_FUNCTION);
    GPIO_setAsOutputPin(GPIO_PORT_P4, GPIO_PIN1);
    GPIO_setOutputHighOnPin(GPIO_PORT_P4, GPIO_PIN1);
    EUSCI_B_SPI_initMasterParam spi_init = {
            .selectClockSource = EUSCI_B_SPI_CLOCKSOURCE_SMCLK,
            .clockSourceFrequency = 8000000, .desiredSpiClock = 8000000,
            .msbFirst = EUSCI_B_SPI_MSB_FIRST, .clockPhase =
            EUSCI_B_SPI_PHASE_DATA_CAPTURED_ONFIRST_CHANGED_ON_NEXT,
            .clockPolarity = EUSCI_B_SPI_CLOCKPOLARITY_INACTIVITY_LOW,
            .spiMode = EUSCI_B_SPI_3PIN };
    EUSCI_B_SPI_initMaster(EUSCI_B1_BASE, &spi_init);
    EUSCI_B_SPI_enable(EUSCI_B1_BASE);
    EUSCI_B_SPI_clearInterrupt(EUSCI_B1_BASE, EUSCI_B_SPI_RECEIVE_INTERRUPT);

    /* Setup SD card (eUSCI_B0)*/
    // P1.6 = UCB0SIMO (MOSI), P1.7 = UCB0SOMI (MISO)
    GPIO_setAsPeripheralModuleFunctionInputPin(
            GPIO_PORT_P1, GPIO_PIN6 | GPIO_PIN7,
            GPIO_SECONDARY_MODULE_FUNCTION);

    // P2.2 = UCB0CLK (SCLK)
    GPIO_setAsPeripheralModuleFunctionInputPin(
            GPIO_PORT_P2, GPIO_PIN2,
            GPIO_SECONDARY_MODULE_FUNCTION);

    // P4.0 = SD Card CS (chip select, manual GPIO control)
    GPIO_setAsOutputPin(GPIO_PORT_P4, GPIO_PIN0);
    GPIO_setOutputHighOnPin(GPIO_PORT_P4, GPIO_PIN0);

    // P7.2 = SD Card Detect (optional, input with pullup)
    GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P7, GPIO_PIN2);

    EUSCI_B_SPI_initMasterParam sd_spi_init = {
            .selectClockSource = EUSCI_B_SPI_CLOCKSOURCE_SMCLK,
            .clockSourceFrequency = 8000000,
            .desiredSpiClock = 400000,  // Start with 400kHz for SD initialization
            .msbFirst = EUSCI_B_SPI_MSB_FIRST,
            .clockPhase = EUSCI_B_SPI_PHASE_DATA_CHANGED_ONFIRST_CAPTURED_ON_NEXT,
            .clockPolarity = EUSCI_B_SPI_CLOCKPOLARITY_INACTIVITY_HIGH,
            .spiMode = EUSCI_B_SPI_3PIN
    };
    EUSCI_B_SPI_initMaster(EUSCI_B0_BASE, &sd_spi_init);
    EUSCI_B_SPI_enable(EUSCI_B0_BASE);
    EUSCI_B_SPI_clearInterrupt(EUSCI_B0_BASE, EUSCI_B_SPI_RECEIVE_INTERRUPT);
#elif UCx_MODE == UCB_MODE_UCB0
    // because old SD card and new SPI-Nand are all comunicated via UCB0 SPI, so they have same SPI MOSI/MISO/CLK configuration
    // P1.6 = UCB0SIMO (MOSI), P1.7 = UCB0SOMI (MISO)
    GPIO_setAsPeripheralModuleFunctionInputPin(
            GPIO_PORT_P1, GPIO_PIN6 | GPIO_PIN7,
            GPIO_SECONDARY_MODULE_FUNCTION);

    // P2.2 = UCB0CLK (SCLK)
    GPIO_setAsPeripheralModuleFunctionInputPin(
            GPIO_PORT_P2, GPIO_PIN2,
            GPIO_SECONDARY_MODULE_FUNCTION);

    /* CS = P4.1 GPIO, same as before */
    GPIO_setAsOutputPin(GPIO_PORT_P4, GPIO_PIN1);
    GPIO_setOutputHighOnPin(GPIO_PORT_P4, GPIO_PIN1);

    // this part is the same as the spi_init param above, just change from UCB1 to UCB0 for tx/rx full-duplex w/ different DMA channel
    {
        EUSCI_B_SPI_initMasterParam spi_init = {
                .selectClockSource = EUSCI_B_SPI_CLOCKSOURCE_SMCLK,
                .clockSourceFrequency = 8000000,
                .desiredSpiClock = 8000000,
                .msbFirst = EUSCI_B_SPI_MSB_FIRST,
                .clockPhase = EUSCI_B_SPI_PHASE_DATA_CAPTURED_ONFIRST_CHANGED_ON_NEXT,
                .clockPolarity = EUSCI_B_SPI_CLOCKPOLARITY_INACTIVITY_LOW,
                .spiMode = EUSCI_B_SPI_3PIN };
        EUSCI_B_SPI_initMaster(EUSCI_B0_BASE, &spi_init);
        EUSCI_B_SPI_enable(EUSCI_B0_BASE);
        EUSCI_B_SPI_clearInterrupt(EUSCI_B0_BASE, EUSCI_B_SPI_RECEIVE_INTERRUPT);
    }
#elif UCx_MODE == UCA_MODE_UCA3
    /* Setup SPI (eUSCI_A3, P6.0/P6.1/P6.2) for NAND + DMA full-duplex */
    // P6.0 = UCA3SIMO, P6.1 = UCA3SOMI, P6.2 = UCA3CLK (all primary function)
    GPIO_setAsPeripheralModuleFunctionInputPin(
            GPIO_PORT_P6, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2,
            GPIO_PRIMARY_MODULE_FUNCTION);

    /* CS = P4.1 GPIO, same as before */
    GPIO_setAsOutputPin(GPIO_PORT_P4, GPIO_PIN1);
    GPIO_setOutputHighOnPin(GPIO_PORT_P4, GPIO_PIN1);

    {
        EUSCI_A_SPI_initMasterParam spi_init = {
                .selectClockSource = EUSCI_A_SPI_CLOCKSOURCE_SMCLK,
                .clockSourceFrequency = 8000000,
                .desiredSpiClock = 8000000,
                .msbFirst = EUSCI_A_SPI_MSB_FIRST,
                .clockPhase = EUSCI_A_SPI_PHASE_DATA_CAPTURED_ONFIRST_CHANGED_ON_NEXT,
                .clockPolarity = EUSCI_A_SPI_CLOCKPOLARITY_INACTIVITY_LOW,
                .spiMode = EUSCI_A_SPI_3PIN };
        EUSCI_A_SPI_initMaster(EUSCI_A3_BASE, &spi_init);
        EUSCI_A_SPI_enable(EUSCI_A3_BASE);
        EUSCI_A_SPI_clearInterrupt(EUSCI_A3_BASE, EUSCI_A_SPI_RECEIVE_INTERRUPT);
    }
#endif
//    hw_setup_ticks[CKPT_AFTER_SPI_GPIO] = get_current_tick(LOW_RES_CLK);

    /* Button P5.6 */
    GPIO_selectInterruptEdge(GPIO_PORT_P5, GPIO_PIN6, GPIO_HIGH_TO_LOW_TRANSITION);
    GPIO_clearInterrupt(GPIO_PORT_P5, GPIO_PIN6);
    GPIO_enableInterrupt(GPIO_PORT_P5, GPIO_PIN6);
    GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P5, GPIO_PIN6);

    /* LED 1.0 */
    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN0);
    GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0);

#if RERANK_MODE == RERANK_MODE_LEA
    msp_lea_init();
#endif

    int nand_ret = spi_nand_init();
//    hw_setup_ticks[CKPT_AFTER_NAND_INIT] = get_current_tick(LOW_RES_CLK);
//    int _i;
//    for (_i = 0; _i < CKPT_COUNT - 1; ++_i) {
//        hw_setup_cum_ticks[_i] += (uint64_t)(hw_setup_ticks[_i + 1] - hw_setup_ticks[_i]);
//    }
//    hw_setup_sample_count++;
//    __disable_interrupt();
    return nand_ret;
}
/*-----------------------------------------------------------*/

int _system_pre_init(void)
{
    /* Stop Watchdog timer. */
    WDT_A_hold( __MSP430_BASEADDRESS_WDT_A__);

    /*
    * Safe FRAM readout mode.
    *
    * Hold the P5.6 push button before applying USB power to stop here before
    * the C runtime initializes global variables and before main() performs
    * any hardware setup, counter updates, RESTORE(), or query execution.
    *
    * The button is sampled only during boot.  Once this loop is entered,
    * releasing the button does not resume execution; CCS can therefore be
    * opened later and the existing "Load Symbols Only" debug configuration
    * can be used without racing against the application.
    */
    P5DIR &= ~BIT6;          /* P5.6 input. */
    P5OUT |= BIT6;           /* Select pull-up resistor. */
    P5REN |= BIT6;           /* Enable the internal resistor. */
    PM5CTL0 &= ~LOCKLPM5;    /* Enable the GPIO configuration. */

    __no_operation();        /* Allow the input configuration to settle. */

    if ((P5IN & BIT6) == 0) {
       __disable_interrupt();

       while (1) {
           __no_operation();
       }
    }

    /* Return 1 for segments to be initialised. */
    return 1;
}

