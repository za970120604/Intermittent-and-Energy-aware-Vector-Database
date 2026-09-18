#ifndef DEBUG_H
#define DEBUG_H

#include "FreeRTOS.h"
#include "driverlib.h"

#if EXPERIMENT == EXP_BASELINE || EXPERIMENT == EXP_POWER_EVENT || EXPERIMENT == EXP_REPLAY
#define SET_BREAKPOINT(bp) { \
    breakpoint = bp; \
    P1OUT |= BIT0; \
    while (1) shutdown();       \
}
#else
#define SET_BREAKPOINT(bp) { \
    breakpoint = bp; \
    __no_operation(); \
    __no_operation(); \
}
#endif

#define SPI_ENERGY 0.081 //uJ
#define FLASH_PAGE_READ_ENERGY 2.1 // uJ
#define FLASH_PAGE_WRITE_ENERGY 2.37 //uJ
#define MIN_WORKING_VOLTAGE 1.8

typedef enum PHASE{
    PHASE_NONE,
    PHASE_CLEAR_BLOCKS,
    PHASE_READ_DATA,
    PHASE_CHECKPOINT,
    PHASE_RESTORE,
    PHASE_ALL_QUERY_PROCESSED,
    PHASE_FINISHED,
    PHASE_QUERY,
    PHASE_NEIGHBORLIST_IO, // added for flash profile
    PHASE_FULLVECTOR_IO, // added for flash profile
    PHASE_NUMBER
} PHASE;


typedef enum DATATYPE{
    TYPE_PQCODE,
    TYPE_NEIGHBORLIST,
    TYPE_FULLVECTOR,
    TYPE_L1_ADDRESS,
    TYPE_NUMBER
};

typedef struct Statistics {
    uint32_t count[PHASE_NUMBER];
    uint16_t power_event_phase[PHASE_NUMBER];

    uint32_t flash_page_read_typecnt[TYPE_NUMBER];
    uint32_t flash_spi_receive_typecnt[TYPE_NUMBER];
    uint32_t flash_cache_hit_typecnt[TYPE_NUMBER];
    uint32_t flash_erase_cnt;

    double elapsed_time[PHASE_NUMBER];
    double min_time[PHASE_NUMBER];
    double max_time[PHASE_NUMBER];
} Statistics;

typedef enum BREAKPOINT_TYPE {
    BP_NONE,
    BP_ERROR,
    BP_ERROR_CKPT,
    BP_TEST,
    BP_OVERFLOW,
    BP_MISCONFIGED,
    BP_FINISHED
} BREAKPOINT_TYPE;

#include "checkpoint.h"
#include "vectors_config.h"
#include "mapping.h"
#include "query_ops.h"
#include "insert_ops.h"

void init_stats();
//void check_system_consistency();
extern BREAKPOINT_TYPE breakpoint;
#endif

