#ifndef CHECKPOINT_H
#define CHECKPOINT_H

#include "FreeRTOS.h"
#include "config.h"
#include <stdint.h>
#include "vectors_config.h"

#define SNAPSHOT_OPTIMIZATION

typedef enum CHECKPOINT_TYPE{
    CHECKPOINT_INIT = 1,
    CHECKPOINT_READ_DATA,
    CHECKPOINT_QUERY,
    CHECKPOINT_WORKLOAD,
    CHECKPOINT_FINISHED
} CHECKPOINT_TYPE;

typedef enum COMMIT_STATUS {
    COMMIT_NULL       = 0x00,
    COMMIT_INCOMPLETE = 0x87, 
    COMMIT_COMPLETE   = 0xAA
} COMMIT_STATUS;

extern char __bss__[], __bssEnd__[], __data__[], __dataEnd__[];

#define BSS_SIZE (__bssEnd__ - __bss__)
#define DATA_SIZE (__dataEnd__ - __data__)
// #define STACK_SIZE                (__stackEnd__ - __stack__)
#ifdef SNAPSHOT_OPTIMIZATION
#define UCHEAP_SIZE 6000
#else
#define UCHEAP_SIZE (configTOTAL_HEAP_SIZE)
#endif

#define SNAPSHOT_SLOT_COUNT     2
#define SNAPSHOT_BSS_SIZE       1024
#define SNAPSHOT_DATA_SIZE      1024
// #define SNAPSHOT_STACK_SIZE     1024
//#define UNDO_LOG_SIZE           25000
//#define UNDO_LOG_HASH_SIZE      512// Must be power of 2

extern uint32_t restart_count_this_query;
extern uint32_t distinct_completed_count;
extern uint32_t distinct_miss_count;

typedef struct Snapshot {
    /* program context */
    uint32_t registers[16];
    uint8_t bss[SNAPSHOT_BSS_SIZE];
    uint8_t data[SNAPSHOT_DATA_SIZE];
    // uint8_t stack[STACK_SIZE];
    uint8_t heap[UCHEAP_SIZE];

    /* Progress tracking */
    uint32_t replay_head;

    /* algo behavior related */
    uint32_t saved_n_query;
    uint32_t saved_n_hops;
    uint32_t saved_n_addL_new;
    uint32_t saved_n_addL_dup;

    uint64_t saved_tick_nbr_io;
    uint64_t saved_tick_nbr_loop;
    uint64_t saved_tick_prune;
    uint64_t saved_tick_hc_new;
    uint64_t saved_tick_hc_dup;
    uint64_t saved_tick_get_closest;
    uint64_t saved_tick_compute_table;
    uint64_t saved_tick_searchlist_expand;
    uint64_t saved_tick_rerank_answer;
    uint64_t saved_tick_rerank_io;
    uint64_t saved_tick_rerank_cmp;
    uint64_t saved_tick_rerank_sort;

#if SPI_DEBUG_MODE == SPI_DEBUG_MODE_ON
    uint64_t saved_tick_pqcode_page_read_cmd;
    uint64_t saved_tick_pqcode_page_read_poll_for_oip;
    uint64_t saved_tick_pqcode_cache_read;
    uint64_t saved_tick_pqtable_lookup;
    uint64_t saved_tick_sorted_insert;
    uint64_t saved_tick_other_cmp;
    uint64_t saved_tick_dma_register;
    uint64_t saved_tick_dma_wait;
#endif

    /* informations */
    CHECKPOINT_TYPE type;
    uint8_t status;
} Snapshot;

/* Checkpoint */
void checkpoint(CHECKPOINT_TYPE type);
void restore();
// void rebuild_search_log_hash();

/* Power event simulation */
void shutdown();
void setup_power_event_timer();
void disable_power_event_timer();

#if EXPERIMENT == EXP_BASELINE
void arm_rtc_wakeup_ms(uint32_t off_ms);
void wait_for_energy(void);
#endif

#if EXPERIMENT == EXP_BASELINE
extern uint8_t in_energy_wait;
extern uint32_t wait_for_energy_count;
#endif

/* Wrappers */
void CHECKPOINT(CHECKPOINT_TYPE type);
void RESTORE();
#endif
