#include "FreeRTOS.h"
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <math.h>
#include "utils.h"
#include "my_timer.h"
#include "spi_nand.h"
#include "graphsearch.h"
#include "checkpoint.h"
#include "debug.h"
#include "config.h"
#include "adc.h"
#include <stdio.h>
#include "vectors_config.h"
#include "pq_codec.h"

extern PHASE phase;
extern Statistics stats;
extern BREAKPOINT_TYPE breakpoint;
extern uint16_t page_in_buffer;
extern uint32_t replay_head;

#if SPI_DEBUG_MODE == SPI_DEBUG_MODE_ON
extern uint64_t tick_pqcode_page_read_cmd;
extern uint64_t tick_pqcode_page_read_poll_for_oip;
#endif

#pragma PERSISTENT(base_graph_entry_vecID)
uint32_t base_graph_entry_vecID = 24753;
#pragma NOINIT(query_buf_neighbor_list)
uint32_t query_buf_neighbor_list[MAX_OUT_NEIGHBORS];
#pragma NOINIT(pq_global_mean)
int16_t pq_global_mean[FULL_VECTOR_DIM];

#if EXPERIMENT == EXP_BASELINE || EXPERIMENT == EXP_POWER_EVENT || EXPERIMENT == EXP_REPLAY
#pragma PERSISTENT(workload_gear_byte_count)
uint32_t workload_gear_byte_count[VOLTAGE_GEAR_COUNT] = {11400, 14269, 17025, 17936, 22315, 26552};
#pragma PERSISTENT(workload_table_L)
uint32_t workload_table_L[VOLTAGE_GEAR_COUNT] = {10, 15, 20, 10, 15, 20};// assume 6 operation point, 6 different L for these operation point
#pragma PERSISTENT(workload_PQ_boundary)
uint8_t workload_PQ_boundary = 3; // from the index third operation point, switch to long PQ code

#pragma PERSISTENT(force_wait_counter)
static uint8_t force_wait_counter = 0;

#if EXPERIMENT == EXP_REPLAY
#pragma PERSISTENT(workload_choice)
uint8_t workload_choice[REPLAY_COUNT] = {
//5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
//5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
//5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
//5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
//5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
////
//5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
//5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
//5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
//5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
//5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5
0
};
#else
#pragma PERSISTENT(workload_choice)
uint8_t workload_choice[REPLAY_COUNT] = {0};
#endif
#pragma PERSISTENT(use_long_PQ)
uint8_t use_long_PQ = 0;
#pragma PERSISTENT(op_point)
uint8_t op_point = 0;
extern uint32_t current_L_size;
#endif

void init_graphsearch() {
    init_stats();

    // L0 mapping table filling
    uint32_t *l0 = (uint32_t *)L0_mapping_table;
    uint32_t L1_write_page_start = 38849; // check workload.c's comment
    for (uint32_t i = 0; i < L0_ENTRY_COUNT; ++i) {
        l0[i] = L1_write_page_start - i;
    }

#if WORKLOAD_DATA == DATA_FAKE
    read_op(65534, 0, pq_global_mean, sizeof(VECTOR_ELEMENT_TYPE) * FULL_VECTOR_DIM);
    for(uint32_t i = 0; i < FULL_VECTOR_DIM; ++i) {
        pq_global_mean[i] =  ((pq_global_mean[i] + 0x1) & FULL_VECTOR_DIM);
    }

#elif WORKLOAD_DATA == DATA_SIFT
    // Reload codebook from NAND flash into FRAM buffer.
    // Pages written downward from codebook_write_page_start by main_workload().

    uint8_t *cb_ptr = (uint8_t*)codebook; // for SPI NAND read_op() parameter design
    uint32_t cb_addr = (uint32_t)codebook; // for SPI NAND read_op_dma() parameter design, because DMA_setDstAddress() receive address as a uint32_t variable, not pointer

#if QPS_PROFILE_MODE == QPS_PROFILE_MODE_SHORT
    uint32_t cb_total = (uint32_t)PQ_VECTOR_DIM * CENTROID_PER_SUBSPACE * SUB_VECTOR_DIM * sizeof(int16_t);
#else
    uint32_t cb_total = (uint32_t)PQ_ACTIVE_DIM * CENTROID_PER_SUBSPACE * SUB_ACTIVE_DIM * sizeof(int16_t);
#endif

#if QPS_PROFILE_MODE == QPS_PROFILE_MODE_SHORT
    uint32_t page = 65534; // check workload.c's comment
#else
    uint32_t page = 31974; // check workload.c's comment
#endif

    for (uint32_t done = 0; done < cb_total; done += PAGE_SIZE, --page) {
        uint32_t chunk = ((cb_total - done) < PAGE_SIZE) ? (cb_total - done) : PAGE_SIZE;
#if (SPI_MODE == SPI_MODE_DMA)
        read_op_dma(page, 0, cb_addr + done, chunk);
#else
        read_op(page, 0, cb_ptr + done, chunk);
#endif
    }

    // Reload mean vector from NAND flash into FRAM buffer.
#if (SPI_MODE == SPI_MODE_DMA)
    read_op_dma(65518, 0, (uint32_t)pq_global_mean, (uint32_t)FULL_VECTOR_DIM * sizeof(int16_t)); // check workload.c's comment
#else
    read_op(65518, 0, (uint8_t *)pq_global_mean, (uint32_t)FULL_VECTOR_DIM * sizeof(int16_t)); // check workload.c's comment
#endif

#if QPS_PROFILE_MODE == QPS_PROFILE_MODE_SHORT
    // Reload codebook long from NAND flash into FRAM buffer.
    uint8_t *cb_long_ptr   = (uint8_t *)codebook_long; // for SPI NAND read_op() parameter design
    uint32_t cb_long_addr  = (uint32_t)codebook_long;  // for SPI NAND read_op_dma() parameter design, because DMA_setDstAddress() receive address as a uint32_t variable, not pointer

    uint32_t cb_long_total = (uint32_t)PQ_VECTOR_DIM * 2 * CENTROID_PER_SUBSPACE * (SUB_VECTOR_DIM / 2) * sizeof(int16_t);

    uint32_t cb_long_page  = 31974; // check workload.c's comment

    for (uint32_t done = 0; done < cb_long_total; done += PAGE_SIZE, --cb_long_page) {
        uint32_t chunk = ((cb_long_total - done) < PAGE_SIZE) ? (cb_long_total - done) : PAGE_SIZE;
#if (SPI_MODE == SPI_MODE_DMA)
        read_op_dma(cb_long_page, 0, cb_long_addr + done, chunk);
#else
        read_op(cb_long_page, 0, cb_long_ptr + done, chunk);
#endif
    }
#endif
#endif
}

void query(const VECTOR_ELEMENT_TYPE *const query_vector, uint32_t entry_vector_id) {
#if EXPERIMENT == EXP_BASELINE || EXPERIMENT == EXP_POWER_EVENT || EXPERIMENT == EXP_REPLAY
    use_long_PQ = 0;

#if EXPERIMENT == EXP_BASELINE
//    float adc_sample = get_voltage();
    float adc_sample = get_voltage(replay_head);
////    if(adc_sample > VOLTAGE_WORKING_MIN) {
//    if(adc_sample > VOLTAGE_WAIT_TRIGGER) {
//        in_energy_wait = 0;
//
//        float available_energy = CAP_FARAD * 500000 * (adc_sample * adc_sample - VOLTAGE_WORKING_MIN*VOLTAGE_WORKING_MIN) * SCALE_FACTOR;
//        uint32_t available_byte_cost = (uint32_t)(available_energy / ENERGY_PER_BYTE_UJ);
//        op_point = 0;
//        for(uint8_t i = 1; i < VOLTAGE_GEAR_COUNT; ++i) {
//            if (available_byte_cost >= workload_gear_byte_count[i]) {
//                op_point = i;
//            }
//            else{
//                break;
//            }
//        }
//        set_last_voltage_op(op_point);
//    }
//    else{
//        op_point = 0;
//        set_last_voltage_op(op_point);
////        set_last_voltage_op(VOLTAGE_OP_WAIT);
////        wait_for_energy();
//    }

    ++force_wait_counter;
    if (force_wait_counter % 3 == 0) {
        wait_for_energy();
    }
    else {
        in_energy_wait = 0;
        op_point = 5; // just for test
    }
#elif EXPERIMENT == EXP_REPLAY
    float adc_sample = get_voltage(replay_head);  // still sampled & logged into voltage_arr
    op_point = workload_choice[replay_head];
    set_last_voltage_op(op_point);
#else // EXPERIMENT == EXP_POWER_EVENT
    op_point = 0;
#endif
    current_L_size = workload_table_L[op_point];
    use_long_PQ = (op_point >= workload_PQ_boundary) ? 1 : 0;
#endif

#ifdef STAT_TIME_MIN_MAX
    uint32_t st_compute_table = get_current_tick(LOW_RES_CLK);
#endif

#if (QPS_PROFILE_MODE == QPS_PROFILE_MODE_SHORT)
#if EXPERIMENT == EXP_BASELINE || EXPERIMENT == EXP_POWER_EVENT || EXPERIMENT == EXP_REPLAY
    if (!use_long_PQ) {
#endif
        for (uint32_t subspace = 0; subspace < PQ_VECTOR_DIM; ++subspace) {
            uint32_t base = subspace * SUB_VECTOR_DIM;
            int32_t q_sub[SUB_VECTOR_DIM];
            for (uint32_t dim = 0; dim < SUB_VECTOR_DIM; ++dim) {
                q_sub[dim] = (int32_t)query_vector[base + dim] - (int32_t)pq_global_mean[base + dim];
            }

            for (uint32_t centroid = 0; centroid < CENTROID_PER_SUBSPACE; ++centroid) {
                uint32_t dist_sq = 0;
                for (uint32_t dim = 0; dim < SUB_VECTOR_DIM; ++dim) {
                    int32_t diff = (int32_t)codebook[subspace][centroid][dim] - q_sub[dim];
                    dist_sq += (uint32_t)(diff * diff);
                }
                distance_table[subspace][centroid] = dist_sq;
            }
        }

#if EXPERIMENT == EXP_BASELINE || EXPERIMENT == EXP_POWER_EVENT || EXPERIMENT == EXP_REPLAY
    }
#endif
#endif

#if QPS_PROFILE_MODE == QPS_PROFILE_MODE_LONG || EXPERIMENT == EXP_BASELINE || EXPERIMENT == EXP_POWER_EVENT || EXPERIMENT == EXP_REPLAY
#if EXPERIMENT == EXP_BASELINE || EXPERIMENT == EXP_POWER_EVENT || EXPERIMENT == EXP_REPLAY
    if (use_long_PQ) {
#endif
        for (uint32_t subspace = 0; subspace < (PQ_VECTOR_DIM * 2); ++subspace) {
            uint32_t base = subspace * (SUB_VECTOR_DIM / 2);
            int32_t q_sub[SUB_VECTOR_DIM / 2];
            for (uint32_t dim = 0; dim < (SUB_VECTOR_DIM / 2); ++dim) {
                q_sub[dim] = (int32_t)query_vector[base + dim] - (int32_t)pq_global_mean[base + dim];
            }

            for (uint32_t centroid = 0; centroid < CENTROID_PER_SUBSPACE; ++centroid) {
                uint32_t dist_sq = 0;
                for (uint32_t dim = 0; dim < (SUB_VECTOR_DIM / 2); ++dim) {
#if QPS_PROFILE_MODE == QPS_PROFILE_MODE_SHORT
                    int32_t diff = (int32_t)codebook_long[subspace][centroid][dim] - q_sub[dim];
#else
                    int32_t diff = (int32_t)codebook[subspace][centroid][dim] - q_sub[dim];
#endif
                    dist_sq += (uint32_t)(diff * diff);
                }
                distance_table[subspace][centroid] = dist_sq;
            }
        }
#if EXPERIMENT == EXP_BASELINE || EXPERIMENT == EXP_POWER_EVENT || EXPERIMENT == EXP_REPLAY
    }
#endif
#endif

#ifdef STAT_TIME_MIN_MAX
    uint32_t ed_compute_table = get_current_tick(LOW_RES_CLK);
    tick_compute_table += (ed_compute_table - st_compute_table);
#endif

    init_search_log();
#if OVERLAP_MODE == OVERLAP_MODE_NONE
    add_to_L(entry_vector_id);
#else
    uint8_t is_dup_entry = add_to_L_hash_check_only(entry_vector_id);
    SearchCandidate* entry_dma_target = add_to_L_other_cmp(entry_vector_id);

    uint32_t group_id = entry_vector_id / L1_ENTRY_COUNT;
    uint16_t target_page = (uint16_t)L0_mapping_table[group_id];
    uint32_t entry_index = entry_vector_id % L1_ENTRY_COUNT;

    if (page_in_buffer != target_page) {
#ifdef STAT_FLASH_PROFILE
        ++stats.flash_page_read_typecnt[TYPE_PQCODE];
#endif

#if SPI_DEBUG_MODE == SPI_DEBUG_MODE_ON
        uint32_t st_page = get_current_tick(LOW_RES_CLK);
#endif
        page_read_send_cmd_only(target_page);
#if SPI_DEBUG_MODE == SPI_DEBUG_MODE_ON
        uint32_t ed_page = get_current_tick(LOW_RES_CLK);
        tick_pqcode_page_read_cmd += (ed_page - st_page);
#endif

#if SPI_DEBUG_MODE == SPI_DEBUG_MODE_ON
        uint32_t st_poll_for_oip = get_current_tick(LOW_RES_CLK);
#endif
        poll_for_oip();
#if SPI_DEBUG_MODE == SPI_DEBUG_MODE_ON
        uint32_t ed_poll_for_oip = get_current_tick(LOW_RES_CLK);
        tick_pqcode_page_read_poll_for_oip += (ed_poll_for_oip - st_poll_for_oip);
#endif

        page_in_buffer = target_page;
    }
#ifdef STAT_FLASH_PROFILE
    else {
        ++stats.flash_cache_hit_typecnt[TYPE_PQCODE];
    }
#endif

#if QPS_PROFILE_MODE == QPS_PROFILE_MODE_SHORT
    read_from_cache_x1_dma(L1_ENTRY_FIELD_OFFSET(entry_index, L1_PQ_CODES_OFFSET), (uint32_t)pq_prefetch_buf_A, sizeof(uint8_t) * PQ_VECTOR_DIM);
#else
    read_from_cache_x1_dma(L1_ENTRY_FIELD_OFFSET(entry_index, L1_PQ_CODES_LONG_OFFSET), (uint32_t)pq_prefetch_buf_A, sizeof(uint8_t) * PQ_ACTIVE_DIM);
#endif
#ifdef STAT_FLASH_PROFILE
#if QPS_PROFILE_MODE == QPS_PROFILE_MODE_SHORT
    stats.flash_spi_receive_typecnt[TYPE_PQCODE] += sizeof(uint8_t) * PQ_VECTOR_DIM;
#else
    stats.flash_spi_receive_typecnt[TYPE_PQCODE] += sizeof(uint8_t) * PQ_ACTIVE_DIM;
#endif
#endif

    add_to_L_prefetch_compute(entry_dma_target, pq_prefetch_buf_A);
#endif

    uint32_t greedy_search_iter = 0;

#ifdef STAT_TIME_MIN_MAX
    uint32_t st_searchlist_expand = get_current_tick(LOW_RES_CLK);
#endif

#if ITER_LIMIT_MODE == ITER_LIMIT_MODE_ON
    while ((greedy_search_iter < MAX_GREEDY_SEARCH_ITER) && (search_log_candidate_count > search_log_visited_count)) {
#else
    while (search_log_candidate_count > search_log_visited_count) { // for unlimited search iteration counts
#endif
        SearchCandidate* p_star = get_closest_unvisited();
        if (p_star == NULL) {
            break;
        }

#if CODE_MODE == CODE_MODE_DISKANN
#if EARLY_TERMINATION_MODE == EARLY_TERMINATION_MODE_ON
        if (search_log_sorted_tail != NULL && p_star->distance_squared >= search_log_sorted_tail->distance_squared) {
            break;
        }
#endif
#endif

        mark_visited(p_star);
#if CODE_MODE == CODE_MODE_DISKANN
        if (expanded_log_count >= MAX_EXPANDED_SIZE) {
            SET_BREAKPOINT(BP_ERROR);
        }
        else {
#if PARTIAL_RERANK_MODE == PARTIAL_RERANK_MODE_ENABLE
            const uint32_t ins_id   = p_star->vector_id;
            const uint32_t ins_dist = p_star->distance_squared;

            int32_t ins_j = (int32_t)expanded_log_count - 1;

            while (ins_j >= 0 &&
                   expanded_log_dists[ins_j] > ins_dist) {
                expanded_log[ins_j + 1] =
                    expanded_log[ins_j];

                expanded_log_dists[ins_j + 1] =
                    expanded_log_dists[ins_j];

                --ins_j;
            }

            expanded_log[ins_j + 1]       = ins_id;
            expanded_log_dists[ins_j + 1] = ins_dist;
            ++expanded_log_count;
#else
            expanded_log[expanded_log_count++] = p_star->vector_id;
#endif
        }
#endif

#ifdef STAT_TIME_MIN_MAX
         uint32_t st_nbr_io = get_current_tick(LOW_RES_CLK);
#endif

         read_neighbors_from_flash(p_star->vector_id, (uint8_t*)query_buf_neighbor_list); // NOINIT buffer query_buf_neighbor_list

#ifdef STAT_TIME_MIN_MAX
         uint32_t ed_nbr_io = get_current_tick(LOW_RES_CLK);
         tick_nbr_io += (ed_nbr_io - st_nbr_io);
#endif


#ifdef STAT_TIME_MIN_MAX
         uint32_t st_nbr_loop = get_current_tick(LOW_RES_CLK);
#endif

#if OVERLAP_MODE == OVERLAP_MODE_NONE
#if CODE_MODE == CODE_MODE_ORIGINAL
        for (uint32_t neighbor = 0; neighbor < MAX_OUT_NEIGHBORS; ++neighbor) {
            add_to_L(query_buf_neighbor_list[neighbor]);
        }
#else
        for (uint32_t neighbor = 0; neighbor < MAX_OUT_NEIGHBORS; ++neighbor) {
            add_to_L(query_buf_neighbor_list[neighbor]);
#if EXPERIMENT != EXP_BASELINE && EXPERIMENT != EXP_POWER_EVENT && EXPERIMENT != EXP_REPLAY
            if (search_log_candidate_count > MAX_L_SIZE) {
#else
            if (search_log_candidate_count > current_L_size) {
#endif
#ifdef STAT_TIME_MIN_MAX
                uint32_t st_prune = get_current_tick(LOW_RES_CLK);
#endif
                prune_L_lazy();

#ifdef STAT_TIME_MIN_MAX
                uint32_t ed_prune = get_current_tick(LOW_RES_CLK);
                tick_prune += (ed_prune - st_prune);
#endif
            }
        }
#endif
#else
        SearchCandidate* prev_dma_target = NULL;
        uint8_t* prev_buf = NULL;
        uint8_t cur_buf_idx = 0;
        uint8_t* bufs[2] = { pq_prefetch_buf_A, pq_prefetch_buf_B };

        for (uint32_t neighbor = 0; neighbor < MAX_OUT_NEIGHBORS; ++neighbor) {
            uint8_t* cur_buf = bufs[cur_buf_idx];

// compute block-1
            uint8_t is_dup = add_to_L_hash_check_only(query_buf_neighbor_list[neighbor]);
            uint32_t group_id = query_buf_neighbor_list[neighbor] / L1_ENTRY_COUNT;
            uint16_t target_page = (uint16_t)L0_mapping_table[group_id];
            uint32_t entry_index = query_buf_neighbor_list[neighbor] % L1_ENTRY_COUNT;
            uint8_t page_hit_flag;
// end of compute block-1

            if (!is_dup) {
                SearchCandidate* cur_dma_target;

                if (page_in_buffer != target_page) {
#ifdef STAT_FLASH_PROFILE
                    ++stats.flash_page_read_typecnt[TYPE_PQCODE];
#endif
                    page_hit_flag = 0;
                    if (prev_dma_target != NULL) { // wait for iter i-1's pq code IO done, cause we need it to compute iter i-1's pq distance and release DMA bus for iter i's (current iter) pq code IO
                        read_from_cache_x1_dma_wait();
                    }
// IO block-1
#if SPI_DEBUG_MODE == SPI_DEBUG_MODE_ON
                    uint32_t st_page = get_current_tick(LOW_RES_CLK);
#endif

                    page_read_send_cmd_only(target_page);

#if SPI_DEBUG_MODE == SPI_DEBUG_MODE_ON
                    uint32_t ed_page = get_current_tick(LOW_RES_CLK);
                    tick_pqcode_page_read_cmd += (ed_page - st_page);
#endif
// end of IO block-1

// compute block-2
                    cur_dma_target = add_to_L_other_cmp(query_buf_neighbor_list[neighbor]);
// end of compute block-2

// IO block-2
#if SPI_DEBUG_MODE == SPI_DEBUG_MODE_ON
                    uint32_t st_poll_for_oip = get_current_tick(LOW_RES_CLK);
#endif

                    poll_for_oip();

#if SPI_DEBUG_MODE == SPI_DEBUG_MODE_ON
                    uint32_t ed_poll_for_oip = get_current_tick(LOW_RES_CLK);
                    tick_pqcode_page_read_poll_for_oip += (ed_poll_for_oip - st_poll_for_oip);
#endif
// end of IO block-2
                    page_in_buffer = target_page;
                }
                else {
#ifdef STAT_FLASH_PROFILE
                    ++stats.flash_cache_hit_typecnt[TYPE_PQCODE];
#endif
                    page_hit_flag = 1;
// compute block-2
                    cur_dma_target = add_to_L_other_cmp(query_buf_neighbor_list[neighbor]);
// end of compute block-2
                }

// IO block-3
                if (page_hit_flag && prev_dma_target != NULL) { // wait for iter i-1's pq code IO done, cause we need it to compute iter i-1's pq distance and release DMA bus for iter i's (current iter) pq code IO
                    read_from_cache_x1_dma_wait();
                }
#if QPS_PROFILE_MODE == QPS_PROFILE_MODE_SHORT
                read_from_cache_x1_dma_start(L1_ENTRY_FIELD_OFFSET(entry_index, L1_PQ_CODES_OFFSET), (uint32_t)cur_buf, sizeof(uint8_t) * PQ_VECTOR_DIM); // overlap mode enable + SPI busy-polling is not supported
#else
                read_from_cache_x1_dma_start(L1_ENTRY_FIELD_OFFSET(entry_index, L1_PQ_CODES_LONG_OFFSET), (uint32_t)cur_buf, sizeof(uint8_t) * PQ_ACTIVE_DIM);
#endif
#ifdef STAT_FLASH_PROFILE
#if QPS_PROFILE_MODE == QPS_PROFILE_MODE_SHORT
                stats.flash_spi_receive_typecnt[TYPE_PQCODE] += sizeof(uint8_t) * PQ_VECTOR_DIM;
#else
                stats.flash_spi_receive_typecnt[TYPE_PQCODE] += sizeof(uint8_t) * PQ_ACTIVE_DIM;
#endif
#endif
// end of IO block-3

// compute block-3
                if (prev_dma_target != NULL) {
                    add_to_L_prefetch_compute(prev_dma_target, prev_buf); // compute iter i-1's pq distance, which is logically overlapped. read_from_cache_x1_dma_wait() insure the iter i-1's pq code IO is already done
                }
#if CODE_MODE == CODE_MODE_DISKANN
                if (search_log_candidate_count > MAX_L_SIZE) {
#ifdef STAT_TIME_MIN_MAX
                    uint32_t st_prune = get_current_tick(LOW_RES_CLK);
#endif

                    prune_L_lazy();

#ifdef STAT_TIME_MIN_MAX
                    uint32_t ed_prune = get_current_tick(LOW_RES_CLK);
                    tick_prune += (ed_prune - st_prune);
#endif
                }
#endif
// end of compute block-3

                prev_dma_target = cur_dma_target;
                prev_buf = cur_buf;
                cur_buf_idx ^= 1;
            }
            else {
                if (prev_dma_target != NULL) { // wait for iter i-1's pq code IO done, cause we need it to compute iter i-1's pq distance and release DMA bus for iter i's (current iter) pq code IO
                    read_from_cache_x1_dma_wait();
                    add_to_L_prefetch_compute(prev_dma_target, prev_buf);
#if CODE_MODE == CODE_MODE_DISKANN
                    if (search_log_candidate_count > MAX_L_SIZE) {
#ifdef STAT_TIME_MIN_MAX
                        uint32_t st_prune = get_current_tick(LOW_RES_CLK);
#endif
                        prune_L_lazy();

#ifdef STAT_TIME_MIN_MAX
                        uint32_t ed_prune = get_current_tick(LOW_RES_CLK);
                        tick_prune += (ed_prune - st_prune);
#endif
                    }
#endif
                }

                prev_dma_target = NULL;
                prev_buf = NULL;
            }
        }

        if (prev_dma_target != NULL) {
            read_from_cache_x1_dma_wait();
            add_to_L_prefetch_compute(prev_dma_target, prev_buf);
#if CODE_MODE == CODE_MODE_DISKANN
            if (search_log_candidate_count > MAX_L_SIZE) {
#ifdef STAT_TIME_MIN_MAX
                uint32_t st_prune = get_current_tick(LOW_RES_CLK);
#endif
                prune_L_lazy();

#ifdef STAT_TIME_MIN_MAX
                uint32_t ed_prune = get_current_tick(LOW_RES_CLK);
                tick_prune += (ed_prune - st_prune);
#endif
            }
#endif
        }
#endif

#ifdef STAT_TIME_MIN_MAX
        uint32_t ed_nbr_loop = get_current_tick(LOW_RES_CLK);
        tick_nbr_loop += (ed_nbr_loop - st_nbr_loop);
#endif

#if CODE_MODE == CODE_MODE_ORIGINAL
        if (search_log_candidate_count > MAX_L_SIZE) {

#ifdef STAT_TIME_MIN_MAX
            uint32_t st_prune = get_current_tick(LOW_RES_CLK);
#endif
            prune_L_lazy();

#ifdef STAT_TIME_MIN_MAX
            uint32_t ed_prune = get_current_tick(LOW_RES_CLK);
            tick_prune += (ed_prune - st_prune);
#endif
        }
#endif

        ++greedy_search_iter;
    }

#ifdef STAT_TIME_MIN_MAX
    uint32_t ed_searchlist_expand = get_current_tick(LOW_RES_CLK);
    tick_searchlist_expand += (ed_searchlist_expand - st_searchlist_expand);
#endif

    n_hops += greedy_search_iter;
    ++n_query;

    for (uint32_t i = 0; i < TOP_K_VALUE; ++i) { // avoid to use memset() if possible, cause calling memset() might cause another level of function stack usage
        search_result_ids[i] = 0;
        search_result_distance_squared[i] = 0;
    }

#ifdef STAT_TIME_MIN_MAX
    uint32_t st_rerank = get_current_tick(LOW_RES_CLK);
#endif

    rerank_searchlist(query_vector);

#if CODE_MODE == CODE_MODE_ORIGINAL
#if SEARCHLIST_MODE == SEARCHLIST_MODE_SORTED
    SearchCandidate* result_iter = search_log_smallest_entry;
#else
    SearchCandidate* result_iter = search_log_list_head;
#endif

    uint32_t count = 0;
    while (result_iter != NULL && count < TOP_K_VALUE) {
        search_result_ids[count] = result_iter->vector_id;
        search_result_distance_squared[count] = result_iter->distance_squared;
        ++count;
        result_iter = result_iter->next;
    }
#else
    uint32_t count = 0;
    while (count < TOP_K_VALUE && count < expanded_log_count) {
        search_result_ids[count]              = expanded_log[count];
        search_result_distance_squared[count] = expanded_log_dists[count];
        ++count;
    }
#endif

#ifdef STAT_TIME_MIN_MAX
    uint32_t ed_rerank = get_current_tick(LOW_RES_CLK);
    tick_rerank_answer += (ed_rerank - st_rerank);
#endif
}

inline void QUERY(const VECTOR_ELEMENT_TYPE* const query_vector) {
    phase = PHASE_QUERY;
    ++stats.count[PHASE_QUERY];

    query(query_vector, base_graph_entry_vecID);

#if EXPERIMENT == EXP_BASELINE || EXPERIMENT == EXP_POWER_EVENT || EXPERIMENT == EXP_REPLAY
    workload_choice[replay_head] = op_point;
#endif
    CHECKPOINT(CHECKPOINT_QUERY);
}
