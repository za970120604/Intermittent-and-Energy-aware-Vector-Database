#ifndef GRAPHSEARCH_QUERY_OPS_H_
#define GRAPHSEARCH_QUERY_OPS_H_

#include "FreeRTOS.h"
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <math.h>
#include "utils.h"
#include "my_timer.h"
#include "spi_nand.h"
#include "checkpoint.h"
#include "debug.h"
#include "config.h"
#include "adc.h"
#include <stdio.h>
#include "vectors_config.h"
#include "debug.h"
#include "mapping.h"
#include "pq_codec.h"
#include "DSPLib.h"

typedef struct SearchCandidate {
    uint32_t vector_id;
    uint32_t distance_squared;
    struct SearchCandidate *next;
    uint8_t visited;

    struct SearchCandidate *hash_prev;
} SearchCandidate;

#define SEARCH_LOG_HASH_SIZE 64  // Must be power of 2
#define SIMPLE_SEARCH_HASH(vector_id) ((vector_id) & (SEARCH_LOG_HASH_SIZE - 1))

void init_search_log(void);
SearchCandidate* get_closest_unvisited(void);
void mark_visited(SearchCandidate* target);
uint8_t has_candidate(uint32_t vector_id);

#if OVERLAP_MODE == OVERLAP_MODE_NONE
void add_to_L(uint32_t vector_id);
#endif

void prune_L_lazy(void);
void rerank_searchlist(const VECTOR_ELEMENT_TYPE *const query_vector);

void read_query_gt_distance(uint32_t query_id, uint32_t* groundtruth_distances);
double compute_recall(uint32_t query_id, const uint32_t* result_distances, uint32_t top_k);

extern uint8_t search_log[SEARCH_BUFFER_SIZE];
extern uint8_t *search_log_head;
extern uint8_t search_log_ext[SEARCH_BUFFER_EXT_SIZE];
extern uint8_t* search_log_ext_head;

#if SEARCHLIST_MODE == SEARCHLIST_MODE_SORTED
extern SearchCandidate* search_log_smallest_entry;
#else
extern SearchCandidate* search_log_list_head;
extern SearchCandidate* search_log_list_tail;
#endif

#if OVERLAP_MODE == OVERLAP_MODE_ENABLE
#if QPS_PROFILE_MODE == QPS_PROFILE_MODE_SHORT
extern uint8_t pq_prefetch_buf_A[PQ_VECTOR_DIM];
extern uint8_t pq_prefetch_buf_B[PQ_VECTOR_DIM];
#else
extern uint8_t pq_prefetch_buf_A[PQ_ACTIVE_DIM];
extern uint8_t pq_prefetch_buf_B[PQ_ACTIVE_DIM];
#endif
#else
#if QPS_PROFILE_MODE == QPS_PROFILE_MODE_SHORT
extern uint8_t add_to_L_pq_code[PQ_VECTOR_DIM];
#else
extern uint8_t add_to_L_pq_code[PQ_ACTIVE_DIM];
#endif
#endif

#if EXPERIMENT == EXP_BASELINE || EXPERIMENT == EXP_IMPROVED || EXPERIMENT == EXP_POWER_EVENT || EXPERIMENT == EXP_REPLAY
extern uint8_t use_long_PQ;
extern uint8_t add_to_L_pq_code_long[PQ_VECTOR_DIM * 2];
extern uint32_t current_L_size;
#endif

extern uint32_t search_log_candidate_count;
extern uint32_t search_log_visited_count;
extern uint32_t search_result_ids[TOP_K_VALUE];
extern uint32_t search_result_distance_squared[TOP_K_VALUE];
extern SearchCandidate *search_log_hash[SEARCH_LOG_HASH_SIZE];

#if CODE_MODE == CODE_MODE_DISKANN
extern SearchCandidate* search_log_sorted_tail;
extern uint32_t expanded_log[MAX_EXPANDED_SIZE];
extern uint32_t expanded_log_dists[MAX_EXPANDED_SIZE];
extern uint32_t expanded_log_count;
#endif

extern int16_t pq_global_mean[FULL_VECTOR_DIM];
extern uint32_t distance_table[PQ_VECTOR_DIM * 2][CENTROID_PER_SUBSPACE];
#if QPS_PROFILE_MODE == QPS_PROFILE_MODE_SHORT
extern int16_t codebook[PQ_VECTOR_DIM][CENTROID_PER_SUBSPACE][SUB_VECTOR_DIM];
#else
extern int16_t codebook[PQ_ACTIVE_DIM][CENTROID_PER_SUBSPACE][SUB_ACTIVE_DIM];
#endif
extern uint32_t current_max_vector_id;
extern BREAKPOINT_TYPE breakpoint;

// profiling counter
extern uint32_t n_query;
extern uint32_t n_hops;
extern uint32_t n_addL_new;
extern uint32_t n_addL_dup;

// profiling time ticks (accumulated)
extern uint64_t tick_nbr_io;
extern uint64_t tick_nbr_loop;
extern uint64_t tick_prune;
extern uint64_t tick_hc_new;
extern uint64_t tick_hc_dup;
extern uint64_t tick_get_closest;
extern uint64_t tick_compute_table;
extern uint64_t tick_searchlist_expand;
extern uint64_t tick_rerank_answer;
extern uint64_t tick_rerank_io;
extern uint64_t tick_rerank_cmp;
extern uint64_t tick_rerank_sort;

#if SPI_DEBUG_MODE == SPI_DEBUG_MODE_ON
extern uint64_t tick_pqcode_page_read_cmd;
extern uint64_t tick_pqcode_page_read_poll_for_oip;
extern uint64_t tick_pqcode_cache_read;
extern uint64_t tick_pqtable_lookup;
extern uint64_t tick_sorted_insert;
extern uint64_t tick_other_cmp;
extern uint64_t tick_dma_register;
extern uint64_t tick_dma_wait;
#endif

extern Statistics stats;
extern PHASE phase;
extern uint16_t page_in_buffer;

#if OVERLAP_MODE == OVERLAP_MODE_ENABLE
static inline uint8_t add_to_L_hash_check_only(uint32_t vector_id) {
#if SPI_DEBUG_MODE == SPI_DEBUG_MODE_ON
    uint32_t st_hc = get_current_tick(LOW_RES_CLK);
#endif
    uint8_t already_exists = has_candidate(vector_id);
#if SPI_DEBUG_MODE == SPI_DEBUG_MODE_ON
    uint32_t ed_hc = get_current_tick(LOW_RES_CLK);
#endif

    if (already_exists != 0) {
#if SPI_DEBUG_MODE == SPI_DEBUG_MODE_ON
        tick_hc_dup += (ed_hc - st_hc);
#endif
        ++n_addL_dup;
        return 1;
    }
#if SPI_DEBUG_MODE == SPI_DEBUG_MODE_ON
    tick_hc_new += (ed_hc - st_hc);
#endif
    ++n_addL_new;
    return 0;
}

static inline SearchCandidate* add_to_L_other_cmp(uint32_t vector_id) {
#if SPI_DEBUG_MODE == SPI_DEBUG_MODE_ON
    uint32_t st_other_cmp = get_current_tick(LOW_RES_CLK);
#endif
    SearchCandidate* search_log_ptr;
    if ((uintptr_t)(search_log_head + sizeof(SearchCandidate)) <= (uintptr_t)&search_log[SEARCH_BUFFER_SIZE]) {
        search_log_ptr = (SearchCandidate*)search_log_head;
        search_log_head += sizeof(SearchCandidate);
    }
    else if ((uintptr_t)(search_log_ext_head + sizeof(SearchCandidate)) <= (uintptr_t)&search_log_ext[SEARCH_BUFFER_EXT_SIZE]) {
        search_log_ptr = (SearchCandidate*)search_log_ext_head;
        search_log_ext_head += sizeof(SearchCandidate);
    }
    else {
        SET_BREAKPOINT(BP_ERROR);
        return NULL;
    }
    search_log_ptr->vector_id = vector_id;
    search_log_ptr->distance_squared = 0;

    uint32_t hash = SIMPLE_SEARCH_HASH(vector_id);
    search_log_ptr->hash_prev = search_log_hash[hash];
    search_log_hash[hash] = search_log_ptr;

    search_log_ptr->visited = 0;
    ++search_log_candidate_count;

#if SPI_DEBUG_MODE == SPI_DEBUG_MODE_ON
    uint32_t ed_other_cmp = get_current_tick(LOW_RES_CLK);
    tick_other_cmp += (ed_other_cmp - st_other_cmp);
#endif
    return search_log_ptr;
}

static inline void add_to_L_prefetch_compute(SearchCandidate* slot, uint8_t* pqcode_buf) {
#if SPI_DEBUG_MODE == SPI_DEBUG_MODE_ON
    uint32_t st_pqtable_lookup = get_current_tick(LOW_RES_CLK);
#endif
#if QPS_PROFILE_MODE == QPS_PROFILE_MODE_SHORT
    for (uint8_t pq_dim = 0; pq_dim < PQ_VECTOR_DIM; ++pq_dim) {
#else
    for (uint8_t pq_dim = 0; pq_dim < PQ_ACTIVE_DIM; ++pq_dim) {
#endif
        slot->distance_squared += distance_table[pq_dim][pqcode_buf[pq_dim]];
    }
#if SPI_DEBUG_MODE == SPI_DEBUG_MODE_ON
    uint32_t ed_pqtable_lookup = get_current_tick(LOW_RES_CLK);
    tick_pqtable_lookup += (ed_pqtable_lookup - st_pqtable_lookup);
#endif

#if SPI_DEBUG_MODE == SPI_DEBUG_MODE_ON
    uint32_t st_sorted_insert = get_current_tick(LOW_RES_CLK);
#endif
    if (search_log_smallest_entry == NULL) {
        slot->next = NULL;
        search_log_smallest_entry = slot;
    }
    else if (slot->distance_squared < search_log_smallest_entry->distance_squared) {
        slot->next = search_log_smallest_entry;
        search_log_smallest_entry = slot;
    }
    else {
        SearchCandidate* insert_iter = search_log_smallest_entry;
        while (insert_iter->next != NULL && insert_iter->next->distance_squared < slot->distance_squared) {
            insert_iter = insert_iter->next;
        }
        slot->next = insert_iter->next;
        insert_iter->next = slot;
    }
#if SPI_DEBUG_MODE == SPI_DEBUG_MODE_ON
    uint32_t ed_sorted_insert = get_current_tick(LOW_RES_CLK);
    tick_sorted_insert += (ed_sorted_insert - st_sorted_insert);
#endif
}
#endif
#endif /* GRAPHSEARCH_QUERY_OPS_H_ */
