#include "query_ops.h"

#pragma PERSISTENT(search_log)
uint8_t search_log[SEARCH_BUFFER_SIZE] = {0};
#pragma PERSISTENT(search_log_head)
uint8_t *search_log_head = (uint8_t*)search_log;

//#pragma SET_DATA_SECTION(".search_ext")
//uint8_t search_log_ext[SEARCH_BUFFER_EXT_SIZE] = {0};
//#pragma SET_DATA_SECTION()
#pragma PERSISTENT(search_log_ext)
uint8_t search_log_ext[SEARCH_BUFFER_EXT_SIZE] = {0};
#pragma PERSISTENT(search_log_ext_head)
uint8_t *search_log_ext_head = (uint8_t*)search_log_ext;

#pragma PERSISTENT(search_log_candidate_count)
uint32_t search_log_candidate_count = 0;
#pragma PERSISTENT(search_log_visited_count)
uint32_t search_log_visited_count = 0;
#pragma NOINIT(search_result_ids)
uint32_t search_result_ids[TOP_K_VALUE];
#pragma NOINIT(search_result_distance_squared)
uint32_t search_result_distance_squared[TOP_K_VALUE];

#if SEARCHLIST_MODE == SEARCHLIST_MODE_SORTED
#pragma PERSISTENT(search_log_smallest_entry)
SearchCandidate* search_log_smallest_entry = NULL;
#else
#pragma PERSISTENT(search_log_list_head)
SearchCandidate* search_log_list_head = NULL;
#pragma PERSISTENT(search_log_list_tail)
SearchCandidate* search_log_list_tail = NULL;
#endif

#pragma NOINIT(rerank_buf_fullvec)
VECTOR_ELEMENT_TYPE rerank_buf_fullvec[FULL_VECTOR_DIM];
#if RERANK_MODE == RERANK_MODE_CPU_LONGPQ
#pragma NOINIT(rerank_buf_pqcode_long)
uint8_t rerank_buf_pqcode_long[PQ_VECTOR_DIM * 2];
#endif

#pragma PERSISTENT(search_log_hash)
SearchCandidate *search_log_hash[SEARCH_LOG_HASH_SIZE] = {NULL};

#if RERANK_MODE == RERANK_MODE_LEA
msp_status status;
msp_sub_q15_params subP = {0};
msp_mac_q15_params macP = {
    .length = FULL_VECTOR_DIM
};

DSPLIB_DATA(batch_rerank_q, 4)
_q15 batch_rerank_q[RERANK_BATCH_B * FULL_VECTOR_DIM];
DSPLIB_DATA(batch_rerank_c, 4)
_q15 batch_rerank_c[RERANK_BATCH_B * FULL_VECTOR_DIM];
DSPLIB_DATA(batch_mac_result, 4)
_iq31 batch_mac_result;
#endif

#if OVERLAP_MODE == OVERLAP_MODE_ENABLE
#if QPS_PROFILE_MODE == QPS_PROFILE_MODE_SHORT
#pragma NOINIT(pq_prefetch_buf_A)
uint8_t pq_prefetch_buf_A[PQ_VECTOR_DIM];
#pragma NOINIT(pq_prefetch_buf_B)
uint8_t pq_prefetch_buf_B[PQ_VECTOR_DIM];
#else
#pragma NOINIT(pq_prefetch_buf_A)
uint8_t pq_prefetch_buf_A[PQ_ACTIVE_DIM];
#pragma NOINIT(pq_prefetch_buf_B)
uint8_t pq_prefetch_buf_B[PQ_ACTIVE_DIM];
#endif
#else
#if QPS_PROFILE_MODE == QPS_PROFILE_MODE_SHORT
#pragma NOINIT(add_to_L_pq_code)
uint8_t add_to_L_pq_code[PQ_VECTOR_DIM];
#else
#pragma NOINIT(add_to_L_pq_code)
uint8_t add_to_L_pq_code[PQ_ACTIVE_DIM];
#endif
#endif

#if EXPERIMENT == EXP_BASELINE || EXPERIMENT == EXP_POWER_EVENT || EXPERIMENT == EXP_REPLAY
#pragma NOINIT(add_to_L_pq_code_long)
uint8_t add_to_L_pq_code_long[PQ_VECTOR_DIM * 2];
#pragma PERSISTENT(current_L_size)
uint32_t current_L_size = MAX_L_SIZE;
#endif

#if CODE_MODE == CODE_MODE_DISKANN
#pragma PERSISTENT(search_log_sorted_tail)
SearchCandidate* search_log_sorted_tail = NULL;
#pragma NOINIT(expanded_log)
uint32_t expanded_log[MAX_EXPANDED_SIZE];
#pragma NOINIT(expanded_log_dists)
uint32_t expanded_log_dists[MAX_EXPANDED_SIZE];
#pragma PERSISTENT(expanded_log_count)
uint32_t expanded_log_count = 0;
#endif

// profiling counter
#pragma PERSISTENT(n_query)
uint32_t n_query          = 0;
#pragma PERSISTENT(n_hops)
uint32_t n_hops           = 0;
#pragma PERSISTENT(n_addL_new)
uint32_t n_addL_new       = 0;
#pragma PERSISTENT(n_addL_dup)
uint32_t n_addL_dup       = 0;

// profiling time ticks (accumulated)
#pragma PERSISTENT(tick_nbr_io)
uint64_t tick_nbr_io      = 0;
#pragma PERSISTENT(tick_nbr_loop)
uint64_t tick_nbr_loop    = 0;
#pragma PERSISTENT(tick_prune)
uint64_t tick_prune       = 0;
#pragma PERSISTENT(tick_hc_new)
uint64_t tick_hc_new      = 0;
#pragma PERSISTENT(tick_hc_dup)
uint64_t tick_hc_dup      = 0;
#pragma PERSISTENT(tick_get_closest)
uint64_t tick_get_closest = 0;
#pragma PERSISTENT(tick_compute_table)
uint64_t tick_compute_table = 0;
#pragma PERSISTENT(tick_searchlist_expand)
uint64_t tick_searchlist_expand = 0;
#pragma PERSISTENT(tick_rerank_answer)
uint64_t tick_rerank_answer= 0;
#pragma PERSISTENT(tick_rerank_io)
uint64_t tick_rerank_io   = 0;
#pragma PERSISTENT(tick_rerank_cmp)
uint64_t tick_rerank_cmp  = 0;
#pragma PERSISTENT(tick_rerank_sort)
uint64_t tick_rerank_sort = 0;

#if SPI_DEBUG_MODE == SPI_DEBUG_MODE_ON
#pragma PERSISTENT(tick_pqcode_page_read_cmd)
uint64_t tick_pqcode_page_read_cmd = 0;
#pragma PERSISTENT(tick_pqcode_page_read_poll_for_oip)
uint64_t tick_pqcode_page_read_poll_for_oip = 0;
#pragma PERSISTENT(tick_pqcode_cache_read)
uint64_t tick_pqcode_cache_read = 0;
#pragma PERSISTENT(tick_pqtable_lookup)
uint64_t tick_pqtable_lookup = 0;
#pragma PERSISTENT(tick_sorted_insert)
uint64_t tick_sorted_insert = 0;
#pragma PERSISTENT(tick_other_cmp)
uint64_t tick_other_cmp = 0;
#pragma PERSISTENT(tick_dma_register)
uint64_t tick_dma_register = 0;
#pragma PERSISTENT(tick_dma_wait)
uint64_t tick_dma_wait = 0;
#endif

uint32_t groundtruth_write_page = 31958;

void read_query_gt_distance(uint32_t query_id, uint32_t* groundtruth_distances) {
    uint32_t gt_entries_per_page = PAGE_SIZE / (TOP_K_VALUE * sizeof(uint32_t));
    uint32_t page_idx    = query_id / gt_entries_per_page;
    uint32_t page_entry  = query_id % gt_entries_per_page;
    uint32_t actual_page = groundtruth_write_page - page_idx;
    uint32_t offset      = page_entry * TOP_K_VALUE * sizeof(uint32_t);
    read_op_dma(actual_page, offset, (uint32_t)groundtruth_distances, TOP_K_VALUE * sizeof(uint32_t));
}

double compute_recall(uint32_t query_id, const uint32_t* result_distances, uint32_t top_k) {
    uint32_t groundtruth_distances[TOP_K_VALUE];
    read_query_gt_distance(query_id, groundtruth_distances);

    uint32_t hits = 0;
    for (uint32_t r = 0; r < top_k; ++r) {
        uint32_t dist_squared = result_distances[r];
        for (uint32_t g = 0; g < top_k; ++g) {
            if (dist_squared == groundtruth_distances[g]) {
                ++hits;
                break;
            }
        }
    }
    return (double)hits / (double)top_k;
}

void init_search_log() {
    search_log_head = (uint8_t*)search_log;
    search_log_ext_head = (uint8_t*)search_log_ext;

#if SEARCHLIST_MODE == SEARCHLIST_MODE_SORTED
    search_log_smallest_entry = NULL;
#else
    search_log_list_head = NULL;
    search_log_list_tail = NULL;
#endif

    search_log_candidate_count = 0;
    search_log_visited_count = 0;
    for (uint32_t i = 0; i < SEARCH_LOG_HASH_SIZE; ++i) {
        search_log_hash[i] = NULL;
    }

#if CODE_MODE == CODE_MODE_DISKANN
    search_log_sorted_tail = NULL;
    expanded_log_count = 0;
#endif
}

SearchCandidate* get_closest_unvisited() {
    SearchCandidate* target = NULL;

#ifdef STAT_TIME_MIN_MAX
    uint32_t st_closest = get_current_tick(LOW_RES_CLK);
#endif

#if SEARCHLIST_MODE == SEARCHLIST_MODE_SORTED
    SearchCandidate* search_log_iter = search_log_smallest_entry;
    while (search_log_iter != NULL) {
        if (search_log_iter->visited == 0) {
            target = search_log_iter;
            break;
        }
        search_log_iter = search_log_iter->next;
    }
#else
    SearchCandidate* search_log_iter = search_log_list_head;
    while (search_log_iter != NULL) {
        if (search_log_iter->visited == 0 && (target == NULL || search_log_iter->distance_squared < target->distance_squared)) {
            target = search_log_iter;
        }
        search_log_iter = search_log_iter->next;
    }
#endif

#ifdef STAT_TIME_MIN_MAX
    uint32_t ed_closest = get_current_tick(LOW_RES_CLK);
    tick_get_closest += (ed_closest - st_closest);
#endif
    return target;
}

void mark_visited(SearchCandidate* target) {
    target->visited = 1;
    ++search_log_visited_count;
}

uint8_t has_candidate(uint32_t vector_id) {
    uint32_t hash = SIMPLE_SEARCH_HASH(vector_id);
    SearchCandidate* it = search_log_hash[hash];
    while (it != NULL) {
        if (it->vector_id == vector_id) {
            return 1;
        }
        it = it->hash_prev;
    }
    return 0;
}

#if OVERLAP_MODE == OVERLAP_MODE_NONE
void add_to_L(uint32_t vector_id) {
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
        return;
    }
#if SPI_DEBUG_MODE == SPI_DEBUG_MODE_ON
    tick_hc_new += (ed_hc - st_hc);
#endif
    ++n_addL_new;

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
        return;
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

    uint32_t group_id = vector_id / L1_ENTRY_COUNT;
    uint16_t target_page = (uint16_t)L0_mapping_table[group_id];
    uint32_t entry_index = vector_id % L1_ENTRY_COUNT;

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


#if SPI_DEBUG_MODE == SPI_DEBUG_MODE_ON
    uint32_t st_cache = get_current_tick(LOW_RES_CLK);
#endif

#if EXPERIMENT == EXP_BASELINE || EXPERIMENT == EXP_POWER_EVENT || EXPERIMENT == EXP_REPLAY
    if (use_long_PQ) {
#if SPI_MODE == SPI_MODE_DMA
        read_from_cache_x1_dma(L1_ENTRY_FIELD_OFFSET(entry_index, L1_PQ_CODES_LONG_OFFSET), (uint32_t)add_to_L_pq_code_long, sizeof(uint8_t) * (PQ_VECTOR_DIM * 2));
#else
        read_from_cache_x1(L1_ENTRY_FIELD_OFFSET(entry_index, L1_PQ_CODES_LONG_OFFSET), add_to_L_pq_code_long, sizeof(uint8_t) * (PQ_VECTOR_DIM * 2));
#endif
#ifdef STAT_FLASH_PROFILE
        stats.flash_spi_receive_typecnt[TYPE_PQCODE] += sizeof(uint8_t) * (PQ_VECTOR_DIM * 2);
#endif
    }
    else {
#if SPI_MODE == SPI_MODE_DMA
        read_from_cache_x1_dma(L1_ENTRY_FIELD_OFFSET(entry_index, L1_PQ_CODES_OFFSET), (uint32_t)add_to_L_pq_code, sizeof(uint8_t) * PQ_VECTOR_DIM);
#else
        read_from_cache_x1(L1_ENTRY_FIELD_OFFSET(entry_index, L1_PQ_CODES_OFFSET), add_to_L_pq_code, sizeof(uint8_t) * PQ_VECTOR_DIM);
#endif
#ifdef STAT_FLASH_PROFILE
        stats.flash_spi_receive_typecnt[TYPE_PQCODE] += sizeof(uint8_t) * (PQ_VECTOR_DIM);
#endif
    }
#else
#if SPI_MODE == SPI_MODE_DMA
#if QPS_PROFILE_MODE == QPS_PROFILE_MODE_SHORT
    read_from_cache_x1_dma(L1_ENTRY_FIELD_OFFSET(entry_index, L1_PQ_CODES_OFFSET), (uint32_t)add_to_L_pq_code, sizeof(uint8_t) * PQ_VECTOR_DIM);
#else
    read_from_cache_x1_dma(L1_ENTRY_FIELD_OFFSET(entry_index, L1_PQ_CODES_LONG_OFFSET), (uint32_t)add_to_L_pq_code, sizeof(uint8_t) * PQ_ACTIVE_DIM);
#endif
#else
#if QPS_PROFILE_MODE == QPS_PROFILE_MODE_SHORT
    read_from_cache_x1(L1_ENTRY_FIELD_OFFSET(entry_index, L1_PQ_CODES_OFFSET), add_to_L_pq_code, sizeof(uint8_t) * PQ_VECTOR_DIM);
#else
    read_from_cache_x1(L1_ENTRY_FIELD_OFFSET(entry_index, L1_PQ_CODES_LONG_OFFSET), (uint32_t)add_to_L_pq_code, sizeof(uint8_t) * PQ_ACTIVE_DIM);
#endif
#endif
#ifdef STAT_FLASH_PROFILE
#if QPS_PROFILE_MODE == QPS_PROFILE_MODE_SHORT
    stats.flash_spi_receive_typecnt[TYPE_PQCODE] += sizeof(uint8_t) * PQ_VECTOR_DIM;
#else
    stats.flash_spi_receive_typecnt[TYPE_PQCODE] += sizeof(uint8_t) * PQ_ACTIVE_DIM;
#endif
#endif
#endif

#if SPI_DEBUG_MODE == SPI_DEBUG_MODE_ON
    uint32_t ed_cache = get_current_tick(LOW_RES_CLK);
    tick_pqcode_cache_read += (ed_cache - st_cache);
#endif

//#if EXPERIMENT == EXP_BASELINE || EXPERIMENT == EXP_POWER_EVENT || EXPERIMENT == EXP_REPLAY
//    if (use_long_PQ) {
//        for (uint8_t pq_dim = 0; pq_dim < (PQ_VECTOR_DIM * 2); ++pq_dim) {
//            add_to_L_pq_code_long[pq_dim] = (uint8_t)((vector_id%128 + pq_dim + 1) % (PQ_VECTOR_DIM * 2));
//        }
//    }
//    else {
//        for (uint8_t pq_dim = 0; pq_dim < PQ_VECTOR_DIM; ++pq_dim) {
//            add_to_L_pq_code[pq_dim] = (uint8_t)((vector_id%128 + pq_dim + 1) % (PQ_VECTOR_DIM));
//        }
//    }
//#endif

#if SPI_DEBUG_MODE == SPI_DEBUG_MODE_ON
    uint32_t st_pqtable_lookup = get_current_tick(LOW_RES_CLK);
#endif

#if EXPERIMENT == EXP_BASELINE || EXPERIMENT == EXP_POWER_EVENT || EXPERIMENT == EXP_REPLAY
    if (use_long_PQ) {
        for (uint8_t pq_dim = 0; pq_dim < (PQ_VECTOR_DIM * 2); ++pq_dim) {
            search_log_ptr->distance_squared += distance_table[pq_dim][add_to_L_pq_code_long[pq_dim]];
        }
    }
    else {
        for (uint8_t pq_dim = 0; pq_dim < PQ_VECTOR_DIM; ++pq_dim) {
            search_log_ptr->distance_squared += distance_table[pq_dim][add_to_L_pq_code[pq_dim]];
        }
    }
#else
#if QPS_PROFILE_MODE == QPS_PROFILE_MODE_SHORT
    for (uint8_t pq_dim = 0; pq_dim < PQ_VECTOR_DIM; ++pq_dim) {
        search_log_ptr->distance_squared += distance_table[pq_dim][add_to_L_pq_code[pq_dim]];
    }
#else
    for (uint8_t pq_dim = 0; pq_dim < PQ_ACTIVE_DIM; ++pq_dim) {
        search_log_ptr->distance_squared += distance_table[pq_dim][add_to_L_pq_code[pq_dim]];
    }
#endif
#endif

#if SPI_DEBUG_MODE == SPI_DEBUG_MODE_ON
    uint32_t ed_pqtable_lookup = get_current_tick(LOW_RES_CLK);
    tick_pqtable_lookup += (ed_pqtable_lookup - st_pqtable_lookup);
#endif

#if SPI_DEBUG_MODE == SPI_DEBUG_MODE_ON
    uint32_t st_sorted_insert = get_current_tick(LOW_RES_CLK);
#endif

#if CODE_MODE == CODE_MODE_DISKANN && SEARCHLIST_MODE == SEARCHLIST_MODE_SORTED && PRUNE_MODE == PRUNE_MODE_DISTANCE_ONLY
#if EXPERIMENT == EXP_BASELINE || EXPERIMENT == EXP_POWER_EVENT || EXPERIMENT == EXP_REPLAY
    if (search_log_sorted_tail != NULL &&
        search_log_candidate_count >= current_L_size &&
        search_log_ptr->distance_squared >= search_log_sorted_tail->distance_squared) {
#else
    if (search_log_sorted_tail != NULL &&
           search_log_candidate_count >=  MAX_L_SIZE &&
           search_log_ptr->distance_squared >= search_log_sorted_tail->distance_squared) {
#endif
#if SPI_DEBUG_MODE == SPI_DEBUG_MODE_ON
        uint32_t ed_sorted_insert = get_current_tick(LOW_RES_CLK);
        tick_sorted_insert += (ed_sorted_insert - st_sorted_insert);
#endif
        return;
    }
#endif

#if SEARCHLIST_MODE == SEARCHLIST_MODE_SORTED
    if (search_log_smallest_entry == NULL) {
        search_log_ptr->next = NULL;
        search_log_smallest_entry = search_log_ptr;
    }
    else if (search_log_ptr->distance_squared < search_log_smallest_entry->distance_squared) {
        search_log_ptr->next = search_log_smallest_entry;
        search_log_smallest_entry = search_log_ptr;
    }
    else {
        SearchCandidate* insert_iter = search_log_smallest_entry;
        while (insert_iter->next != NULL && insert_iter->next->distance_squared < search_log_ptr->distance_squared) {
           insert_iter = insert_iter->next;
        }
        search_log_ptr->next = insert_iter->next;
        insert_iter->next = search_log_ptr;
    }
#else
    search_log_ptr->next = NULL;
    if (search_log_list_head == NULL) {
        search_log_list_head = search_log_ptr;
        search_log_list_tail = search_log_ptr;
    }
    else {
        search_log_list_tail->next = search_log_ptr;
        search_log_list_tail = search_log_ptr;
    }
#endif
#if SPI_DEBUG_MODE == SPI_DEBUG_MODE_ON
    uint32_t ed_sorted_insert = get_current_tick(LOW_RES_CLK);
    tick_sorted_insert += (ed_sorted_insert - st_sorted_insert);
#endif
}
#endif

//void prune_L_lazy() {
//#if EXPERIMENT != EXP_BASELINE && EXPERIMENT != EXP_IMPROVED && EXPERIMENT != EXP_POWER_EVENT && EXPERIMENT != EXP_REPLAY
//    if (search_log_candidate_count <= MAX_L_SIZE) {
//#else
//    if (search_log_candidate_count <= current_L_size) {
//#endif
//        return;
//    }
//
//#if (SEARCHLIST_MODE == SEARCHLIST_MODE_SORTED && PRUNE_MODE == PRUNE_MODE_DISTANCE_ONLY)
//    SearchCandidate* iter = search_log_smallest_entry;
//#if EXPERIMENT != EXP_BASELINE && EXPERIMENT != EXP_IMPROVED && EXPERIMENT != EXP_POWER_EVENT && EXPERIMENT != EXP_REPLAY
//    for (uint32_t i = 1; i < MAX_L_SIZE && iter != NULL; ++i) {
//#else
//    for (uint32_t i = 1; i < current_L_size && iter != NULL; ++i) {
//#endif
//        iter = iter->next;
//    }
//    if (iter != NULL) {
//        iter->next = NULL;
//    }
//#if CODE_MODE == CODE_MODE_DISKANN
//    search_log_sorted_tail = iter;
//#endif
//#elif (SEARCHLIST_MODE == SEARCHLIST_MODE_SORTED && PRUNE_MODE == PRUNE_MODE_VISITED_FIRST)
//#if EXPERIMENT != EXP_BASELINE && EXPERIMENT != EXP_IMPROVED && EXPERIMENT != EXP_POWER_EVENT && EXPERIMENT != EXP_REPLAY
//    uint32_t remove_count = search_log_candidate_count - MAX_L_SIZE;
//#else
//    uint32_t remove_count = search_log_candidate_count - current_L_size;
//#endif
//    for (uint32_t r = 0; r < remove_count; ++r) { // mark "remove_count" times
//        SearchCandidate* max_node = NULL;
//        SearchCandidate* iter = search_log_smallest_entry;
//        while (iter != NULL) {
//            if (iter->distance_squared != UINT32_MAX && iter->visited == 1 && (max_node == NULL || iter->distance_squared > max_node->distance_squared)) {
//                max_node = iter;
//            }
//            iter = iter->next;
//        }
//
//        if (max_node == NULL) {
//            iter = search_log_smallest_entry;
//            while (iter != NULL) {
//                if (iter->distance_squared != UINT32_MAX && iter->visited == 0 && (max_node == NULL || iter->distance_squared > max_node->distance_squared)) {
//                    max_node = iter;
//                }
//                iter = iter->next;
//            }
//        }
//
//        if (max_node != NULL) {
//            max_node->distance_squared = UINT32_MAX;
//        }
//        else {
//            break;
//        }
//    }
//#elif SEARCHLIST_MODE == SEARCHLIST_MODE_UNSORTED
//#if EXPERIMENT != EXP_BASELINE && EXPERIMENT != EXP_IMPROVED && EXPERIMENT != EXP_POWER_EVENT && EXPERIMENT != EXP_REPLAY
//    uint32_t remove_count = search_log_candidate_count - MAX_L_SIZE;
//#else
//    uint32_t remove_count = search_log_candidate_count - current_L_size;
//#endif
//    for (uint32_t r = 0; r < remove_count; ++r) { // mark "remove_count" times
//        SearchCandidate* max_node = NULL;
//        SearchCandidate* iter = search_log_list_head;
//
//#if PRUNE_MODE == PRUNE_MODE_DISTANCE_ONLY
//        while (iter != NULL) {
//            if (iter->distance_squared != UINT32_MAX && (max_node == NULL || iter->distance_squared > max_node->distance_squared)) {
//                max_node = iter;
//            }
//            iter = iter->next;
//        }
//#else
//        while (iter != NULL) {
//            if (iter->distance_squared != UINT32_MAX && iter->visited == 1 && (max_node == NULL || iter->distance_squared > max_node->distance_squared)) {
//                max_node = iter;
//            }
//            iter = iter->next;
//        }
//
//        if (max_node == NULL) {
//            iter = search_log_list_head;
//            while (iter != NULL) {
//                if (iter->distance_squared != UINT32_MAX && iter->visited == 0 && (max_node == NULL || iter->distance_squared > max_node->distance_squared)) {
//                    max_node = iter;
//                }
//                iter = iter->next;
//            }
//        }
//#endif
//
//        if (max_node != NULL) {
//            max_node->distance_squared = UINT32_MAX;
//        }
//        else {
//            break;
//        }
//    }
//#else
//    SET_BREAKPOINT(BP_MISCONFIGED);
//#endif
//
//    search_log_candidate_count = 0;
//    search_log_visited_count = 0;
//#if CODE_MODE == CODE_MODE_ORIGINAL
//    for (uint32_t i = 0; i < SEARCH_LOG_HASH_SIZE; ++i) {
//        search_log_hash[i] = NULL;
//    }
//#endif
//
//#if (SEARCHLIST_MODE == SEARCHLIST_MODE_SORTED && PRUNE_MODE == PRUNE_MODE_DISTANCE_ONLY)
//    iter = search_log_smallest_entry;
//    while (iter != NULL) {
//        ++search_log_candidate_count;
//        if (iter->visited) {
//            ++search_log_visited_count;
//        }
//
//#if CODE_MODE == CODE_MODE_ORIGINAL
//        uint32_t hash = SIMPLE_SEARCH_HASH(iter->vector_id);
//        iter->hash_prev = search_log_hash[hash];
//        search_log_hash[hash] = iter;
//#endif
//        iter = iter->next;
//    }
//#elif (SEARCHLIST_MODE == SEARCHLIST_MODE_SORTED && PRUNE_MODE == PRUNE_MODE_VISITED_FIRST)
//    SearchCandidate* new_head = NULL;
//    SearchCandidate* new_prev = NULL;
//    SearchCandidate* iter = search_log_smallest_entry;
//
//    while (iter != NULL) {
//        SearchCandidate* next_ = iter->next;
//        if (iter->distance_squared != UINT32_MAX) {
//            iter->next = NULL;
//            if (!new_head) {
//                new_head = iter;
//            }
//            else {
//                new_prev->next = iter;
//            }
//            new_prev = iter;
//
//            ++search_log_candidate_count;
//            if (iter->visited) {
//                ++search_log_visited_count;
//            }
//
//            uint32_t hash = SIMPLE_SEARCH_HASH(iter->vector_id);
//            iter->hash_prev = search_log_hash[hash];
//            search_log_hash[hash] = iter;
//        }
//        iter = next_;
//    }
//    search_log_smallest_entry = new_head;
//#else
//    SearchCandidate* new_head = NULL;
//    SearchCandidate* new_tail = NULL;
//    SearchCandidate* iter = search_log_list_head;
//    while (iter != NULL) {
//        SearchCandidate* next_ = iter->next;
//        if (iter->distance_squared != UINT32_MAX) {
//            iter->next = NULL;
//            if (!new_head) {
//                new_head = iter;
//                new_tail = iter;
//            }
//            else {
//                new_tail->next = iter;
//                new_tail = iter;
//            }
//
//            ++search_log_candidate_count;
//            if (iter->visited) {
//                ++search_log_visited_count;
//            }
//
//            uint32_t hash = SIMPLE_SEARCH_HASH(iter->vector_id);
//            iter->hash_prev = search_log_hash[hash];
//            search_log_hash[hash] = iter;
//        }
//        iter = next_;
//    }
//    search_log_list_head = new_head;
//    search_log_list_tail = new_tail;
//#endif
//}

void prune_L_lazy() {
#if EXPERIMENT != EXP_BASELINE && EXPERIMENT != EXP_POWER_EVENT && EXPERIMENT != EXP_REPLAY
    if (search_log_candidate_count <= MAX_L_SIZE) {
#else
    if (search_log_candidate_count <= current_L_size) {
#endif
        return;
    }

    SearchCandidate* iter = search_log_smallest_entry;
    uint32_t kept_count = 0;
    uint32_t kept_visited = 0;

#if EXPERIMENT != EXP_BASELINE && EXPERIMENT != EXP_POWER_EVENT && EXPERIMENT != EXP_REPLAY
    uint32_t limit = MAX_L_SIZE;
#else
    uint32_t limit = current_L_size;
#endif

    while (iter != NULL && kept_count < limit) {
        if (iter->visited) {
            ++kept_visited;
        }
        ++kept_count;

        if (kept_count == limit) {
            SearchCandidate* cut_point = iter;
            iter = cut_point->next;
            cut_point->next = NULL;
            search_log_sorted_tail = cut_point;
            break;
        }
        iter = iter->next;
    }

    search_log_candidate_count = kept_count;
    search_log_visited_count   = kept_visited;
}

#if CODE_MODE == CODE_MODE_ORIGINAL
void rerank_searchlist(const VECTOR_ELEMENT_TYPE *const query_vector) {
#if SEARCHLIST_MODE == SEARCHLIST_MODE_SORTED
    if (search_log_smallest_entry == NULL) {
        return;
    }
#else
    if (search_log_list_head == NULL) {
        return;
    }
#endif

#if SEARCHLIST_MODE == SEARCHLIST_MODE_SORTED
    SearchCandidate* iter = search_log_smallest_entry;
#else
    SearchCandidate* iter = search_log_list_head;
#endif

#if RERANK_MODE == RERANK_MODE_CPU_FULLVEC
    while (iter != NULL) {
        uint32_t st_fullvec_io = get_current_tick(LOW_RES_CLK);
        read_fullvec_from_flash(iter->vector_id, (uint8_t*)rerank_buf_fullvec);
        uint32_t ed_fullvec_io = get_current_tick(LOW_RES_CLK);
        tick_rerank_io += (ed_fullvec_io - st_fullvec_io);

        uint32_t st_distcmp = get_current_tick(LOW_RES_CLK);
        uint32_t exact_dist = 0;
        for (uint32_t dim = 0; dim < FULL_VECTOR_DIM; ++dim) {
            int32_t diff = (int32_t)query_vector[dim] - (int32_t)rerank_buf_fullvec[dim];
            exact_dist += (uint32_t)(diff * diff);
        }
        iter->distance_squared = exact_dist;
        uint32_t ed_distcmp = get_current_tick(LOW_RES_CLK);
        tick_rerank_cmp += (ed_distcmp - st_distcmp);
        iter = iter->next;
    }
#elif RERANK_MODE == RERANK_MODE_CPU_LONGPQ
    for (uint32_t subspace = 0; subspace < PQ_VECTOR_DIM * 2; ++subspace) {
        uint32_t base = subspace * (SUB_VECTOR_DIM / 2);
        int32_t q_sub[SUB_VECTOR_DIM / 2];
        for (uint32_t dim = 0; dim < SUB_VECTOR_DIM / 2; ++dim) {
            q_sub[dim] = (int32_t)query_vector[base + dim] - (int32_t)pq_global_mean[base + dim];
        }

        for (uint32_t centroid = 0; centroid < CENTROID_PER_SUBSPACE; ++centroid) {
            uint32_t dist_sq = 0;
            for (uint32_t dim = 0; dim < SUB_VECTOR_DIM / 2; ++dim) {
                int32_t diff = (int32_t)codebook_long[subspace][centroid][dim] - q_sub[dim];
                dist_sq += (uint32_t)(diff * diff);
            }
            distance_table[subspace][centroid] = dist_sq;
        }
    }
    while (iter != NULL) {
        uint32_t st_longpq_io = get_current_tick(LOW_RES_CLK);
        read_pqcode_long_from_flash(iter->vector_id, rerank_buf_pqcode_long);
        uint32_t ed_longpq_io = get_current_tick(LOW_RES_CLK);
        tick_rerank_io += (ed_longpq_io - st_longpq_io);

        uint32_t st_longpq_cmp = get_current_tick(LOW_RES_CLK);
        uint32_t pq_dist = 0;
        for (uint32_t subspace = 0; subspace < PQ_VECTOR_DIM * 2; ++subspace) {
            pq_dist += distance_table[subspace][rerank_buf_pqcode_long[subspace]];
        }
        iter->distance_squared = pq_dist;
        uint32_t ed_longpq_cmp = get_current_tick(LOW_RES_CLK);
        tick_rerank_cmp += (ed_longpq_cmp - st_longpq_cmp);
        iter = iter->next;
    }
#else
    for (uint8_t pair = 0; pair < RERANK_BATCH_B; ++pair) {
        uint16_t base = pair * FULL_VECTOR_DIM;
        for (uint16_t i = 0; i < FULL_VECTOR_DIM; ++i) {
            batch_rerank_q[base + i] = (_q15)query_vector[i];
        }
    }

    while(iter != NULL) {
        SearchCandidate* batch_start_iter = iter;
        uint8_t batch_count_this_run = 0;
        for(uint8_t pair = 0; pair < RERANK_BATCH_B && iter != NULL; ++pair) {
            uint16_t base = pair * FULL_VECTOR_DIM;
            uint32_t st_fullvec_io = get_current_tick(LOW_RES_CLK);
            read_fullvec_from_flash(iter->vector_id, (uint8_t*)(batch_rerank_c + base));
            uint32_t ed_fullvec_io = get_current_tick(LOW_RES_CLK);
            tick_rerank_io += (ed_fullvec_io - st_fullvec_io);

//            for (uint16_t i = 0; i < FULL_VECTOR_DIM; ++i) {
//                batch_rerank_c[base + i] = (_q15)rerank_buf_fullvec[i];
//            }

            iter = iter->next;
            ++batch_count_this_run;
        }


        uint32_t st_distcmp = get_current_tick(LOW_RES_CLK);
        subP.length = batch_count_this_run * FULL_VECTOR_DIM;

        status = msp_sub_q15(&subP, batch_rerank_q, batch_rerank_c, batch_rerank_c);
        if (status != MSP_SUCCESS) {
            SET_BREAKPOINT(BP_ERROR);
        }

        for (uint8_t pair = 0; pair < batch_count_this_run; ++pair) {
            status = msp_mac_q15(&macP,
                 batch_rerank_c + pair * FULL_VECTOR_DIM,
                 batch_rerank_c + pair * FULL_VECTOR_DIM,
                &batch_mac_result);

            if (status != MSP_SUCCESS) {
                SET_BREAKPOINT(BP_ERROR);
            }

            batch_start_iter->distance_squared = (uint32_t)(batch_mac_result >> 1);
            batch_start_iter = batch_start_iter->next;
        }
        uint32_t ed_distcmp = get_current_tick(LOW_RES_CLK);
        tick_rerank_cmp += (ed_distcmp - st_distcmp);
    }
#endif

    uint32_t st_sort = get_current_tick(LOW_RES_CLK);

#if SEARCHLIST_MODE == SEARCHLIST_MODE_SORTED
    SearchCandidate* old_list = search_log_smallest_entry;
    search_log_smallest_entry = NULL;

    while (old_list != NULL) {
        SearchCandidate* next_ = old_list->next;

        if (search_log_smallest_entry == NULL) {
            old_list->next = NULL;
            search_log_smallest_entry = old_list;
        }
        else if (old_list->distance_squared < search_log_smallest_entry->distance_squared) {
            old_list->next = search_log_smallest_entry;
            search_log_smallest_entry = old_list;
        }
        else {
            SearchCandidate* insert_iter = search_log_smallest_entry;
            while (insert_iter->next != NULL && insert_iter->next->distance_squared < old_list->distance_squared) {
                insert_iter = insert_iter->next;
            }
            old_list->next = insert_iter->next;
            insert_iter->next = old_list;
        }

        old_list = next_;
    }
#else
    SearchCandidate* old_list = search_log_list_head;
    search_log_list_head = NULL;
    search_log_list_tail = NULL; // for safety

    while (old_list != NULL) {
        SearchCandidate* next_ = old_list->next;
        old_list->next = NULL;

        if (search_log_list_head == NULL) {
            search_log_list_head = old_list;
            search_log_list_tail = old_list;
        }
        else if (old_list->distance_squared < search_log_list_head->distance_squared) {
            old_list->next = search_log_list_head;
            search_log_list_head = old_list;
        }
        else {
            SearchCandidate* insert_iter = search_log_list_head;
            while (insert_iter->next != NULL && insert_iter->next->distance_squared < old_list->distance_squared) {
                insert_iter = insert_iter->next;
            }
            old_list->next = insert_iter->next;
            insert_iter->next = old_list;
            if (old_list->next == NULL) {
                search_log_list_tail = old_list;
            }
        }

        old_list = next_;
    }
#endif

    uint32_t ed_sort = get_current_tick(LOW_RES_CLK);
    tick_rerank_sort += (ed_sort - st_sort);

#if RERANK_MODE == RERANK_MODE_CPU_LONGPQ
#if SEARCHLIST_MODE == SEARCHLIST_MODE_SORTED
    SearchCandidate* iter_resort = search_log_smallest_entry;
#else
    SearchCandidate* iter_resort = search_log_list_head;
#endif
    for (uint32_t i = 0; i < LONGPQ_EXACT_TOPK && iter_resort != NULL; ++i, iter_resort = iter_resort->next) {
        uint32_t st_fullvec_resort_io = get_current_tick(LOW_RES_CLK);
        read_fullvec_from_flash(iter_resort->vector_id, (uint8_t*)rerank_buf_fullvec);
        uint32_t ed_fullvec_resort_io = get_current_tick(LOW_RES_CLK);
        tick_rerank_io += (ed_fullvec_resort_io - st_fullvec_resort_io);

        uint32_t st_fullvec_resort_cmp = get_current_tick(LOW_RES_CLK);
        uint32_t exact_dist = 0;
        for (uint32_t dim = 0; dim < FULL_VECTOR_DIM; ++dim) {
            int32_t diff = (int32_t)query_vector[dim] - (int32_t)rerank_buf_fullvec[dim];
            exact_dist += (uint32_t)(diff * diff);
        }
        iter_resort->distance_squared = exact_dist;
        uint32_t ed_fullvec_resort_cmp = get_current_tick(LOW_RES_CLK);
        tick_rerank_cmp += (ed_fullvec_resort_cmp - st_fullvec_resort_cmp);

        if (i == LONGPQ_EXACT_TOPK - 1) {
            iter_resort->next = NULL;
        }
    }

    uint32_t st_sort2 = get_current_tick(LOW_RES_CLK);
#if SEARCHLIST_MODE == SEARCHLIST_MODE_SORTED
    SearchCandidate* old_list2 = search_log_smallest_entry;
    search_log_smallest_entry = NULL;
    while (old_list2 != NULL) {
        SearchCandidate* next2_ = old_list2->next;
        if (search_log_smallest_entry == NULL) {
            old_list2->next = NULL;
            search_log_smallest_entry = old_list2;
        }
        else if (old_list2->distance_squared < search_log_smallest_entry->distance_squared) {
            old_list2->next = search_log_smallest_entry;
            search_log_smallest_entry = old_list2;
        }
        else {
            SearchCandidate* insert_iter2 = search_log_smallest_entry;
            while (insert_iter2->next != NULL && insert_iter2->next->distance_squared < old_list2->distance_squared) {
                insert_iter2 = insert_iter2->next;
            }
            old_list2->next = insert_iter2->next;
            insert_iter2->next = old_list2;
        }
        old_list2 = next2_;
    }
#else
    SearchCandidate* old_list2 = search_log_list_head;
    search_log_list_head = NULL;
    search_log_list_tail = NULL; // for safety

    while (old_list2 != NULL) {
        SearchCandidate* next2_ = old_list2->next;
        old_list2->next = NULL;

        if (search_log_list_head == NULL) {
            search_log_list_head = old_list2;
            search_log_list_tail = old_list2;
        }
        else if (old_list2->distance_squared < search_log_list_head->distance_squared) {
            old_list2->next = search_log_list_head;
            search_log_list_head = old_list2;
        }
        else {
            SearchCandidate* insert_iter2 = search_log_list_head;
            while (insert_iter2->next != NULL && insert_iter2->next->distance_squared < old_list2->distance_squared) {
                insert_iter2 = insert_iter2->next;
            }
            old_list2->next = insert_iter2->next;
            insert_iter2->next = old_list2;
            if (old_list2->next == NULL) {
                search_log_list_tail = old_list2;
            }
        }

        old_list2 = next2_;
    }
#endif
   uint32_t ed_sort2 = get_current_tick(LOW_RES_CLK);
   tick_rerank_sort += (ed_sort2 - st_sort2);
#endif
}
#else
void rerank_searchlist(const VECTOR_ELEMENT_TYPE *const query_vector) {
    if (expanded_log_count == 0) {
        return;
    }

#if PARTIAL_RERANK_MODE == PARTIAL_RERANK_MODE_ENABLE
    uint32_t rerank_count = (expanded_log_count < (uint32_t)RERANK_PARTIAL_TOPN) ? expanded_log_count : (uint32_t)RERANK_PARTIAL_TOPN;
#else
    uint32_t rerank_count = expanded_log_count;
#endif


#if RERANK_MODE == RERANK_MODE_CPU_FULLVEC
    for (uint32_t i = 0; i < rerank_count; ++i) {
#ifdef STAT_TIME_MIN_MAX
        uint32_t st_fullvec_io = get_current_tick(LOW_RES_CLK);
#endif

        read_fullvec_from_flash(expanded_log[i], (uint8_t*)rerank_buf_fullvec);

#ifdef STAT_TIME_MIN_MAX
        uint32_t ed_fullvec_io = get_current_tick(LOW_RES_CLK);
        tick_rerank_io += (ed_fullvec_io - st_fullvec_io);
#endif

#ifdef STAT_TIME_MIN_MAX
        uint32_t st_distcmp = get_current_tick(LOW_RES_CLK);
#endif

        uint32_t exact_dist = 0;
        for (uint32_t dim = 0; dim < FULL_VECTOR_DIM; ++dim) {
            int32_t diff = (int32_t)query_vector[dim] - (int32_t)rerank_buf_fullvec[dim];
            exact_dist += (uint32_t)(diff * diff);
        }
        expanded_log_dists[i] = exact_dist;

#ifdef STAT_TIME_MIN_MAX
        uint32_t ed_distcmp = get_current_tick(LOW_RES_CLK);
        tick_rerank_cmp += (ed_distcmp - st_distcmp);
#endif
    }
    expanded_log_count = rerank_count;

#elif RERANK_MODE == RERANK_MODE_CPU_LONGPQ
    for (uint32_t subspace = 0; subspace < PQ_VECTOR_DIM * 2; ++subspace) {
        uint32_t base = subspace * (SUB_VECTOR_DIM / 2);
        int32_t q_sub[SUB_VECTOR_DIM / 2];
        for (uint32_t dim = 0; dim < SUB_VECTOR_DIM / 2; ++dim) {
            q_sub[dim] = (int32_t)query_vector[base + dim] - (int32_t)pq_global_mean[base + dim];
        }
        for (uint32_t centroid = 0; centroid < CENTROID_PER_SUBSPACE; ++centroid) {
            uint32_t dist_sq = 0;
            for (uint32_t dim = 0; dim < SUB_VECTOR_DIM / 2; ++dim) {
                int32_t diff = (int32_t)codebook_long[subspace][centroid][dim] - q_sub[dim];
                dist_sq += (uint32_t)(diff * diff);
            }
            distance_table[subspace][centroid] = dist_sq;
        }
    }

    for (uint32_t i = 0; i < expanded_log_count; ++i) {
#ifdef STAT_TIME_MIN_MAX
        uint32_t st_longpq_io = get_current_tick(LOW_RES_CLK);
#endif

        read_pqcode_long_from_flash(expanded_log[i], rerank_buf_pqcode_long);

#ifdef STAT_TIME_MIN_MAX
        uint32_t ed_longpq_io = get_current_tick(LOW_RES_CLK);
        tick_rerank_io += (ed_longpq_io - st_longpq_io);
#endif

#ifdef STAT_TIME_MIN_MAX
        uint32_t st_longpq_cmp = get_current_tick(LOW_RES_CLK);
#endif

        uint32_t pq_dist = 0;
        for (uint32_t subspace = 0; subspace < PQ_VECTOR_DIM * 2; ++subspace) {
            pq_dist += distance_table[subspace][rerank_buf_pqcode_long[subspace]];
        }
        expanded_log_dists[i] = pq_dist;

#ifdef STAT_TIME_MIN_MAX
        uint32_t ed_longpq_cmp = get_current_tick(LOW_RES_CLK);
        tick_rerank_cmp += (ed_longpq_cmp - st_longpq_cmp);
#endif
    }

#else // RERANK_MODE_LEA
    for (uint8_t pair = 0; pair < RERANK_BATCH_B; ++pair) {
        uint16_t base = pair * FULL_VECTOR_DIM;
        for (uint16_t i = 0; i < FULL_VECTOR_DIM; ++i) {
            batch_rerank_q[base + i] = (_q15)query_vector[i];
        }
    }

    uint32_t i = 0;
    while (i < rerank_count) {
        uint8_t batch_count_this_run = 0;
        uint32_t batch_start_i = i;

        for (uint8_t pair = 0; pair < RERANK_BATCH_B && i < rerank_count; ++pair) {
            uint16_t base = pair * FULL_VECTOR_DIM;
#ifdef STAT_TIME_MIN_MAX
            uint32_t st_fullvec_io = get_current_tick(LOW_RES_CLK);
#endif

            read_fullvec_from_flash(expanded_log[i], (uint8_t*)(batch_rerank_c + base));

#ifdef STAT_TIME_MIN_MAX
            uint32_t ed_fullvec_io = get_current_tick(LOW_RES_CLK);
            tick_rerank_io += (ed_fullvec_io - st_fullvec_io);
#endif
            ++i;
            ++batch_count_this_run;
        }

#ifdef STAT_TIME_MIN_MAX
        uint32_t st_distcmp = get_current_tick(LOW_RES_CLK);
#endif

        subP.length = batch_count_this_run * FULL_VECTOR_DIM;
        status = msp_sub_q15(&subP, batch_rerank_q, batch_rerank_c, batch_rerank_c);
        if (status != MSP_SUCCESS) {
            SET_BREAKPOINT(BP_ERROR);
        }

        for (uint8_t pair = 0; pair < batch_count_this_run; ++pair) { // LEA - LEA MAC
            status = msp_mac_q15(&macP,
                 batch_rerank_c + pair * FULL_VECTOR_DIM,
                 batch_rerank_c + pair * FULL_VECTOR_DIM,
                &batch_mac_result);
            if (status != MSP_SUCCESS) {
                SET_BREAKPOINT(BP_ERROR);
            }
            expanded_log_dists[batch_start_i + pair] = (uint32_t)(batch_mac_result >> 1);
        }

//        for (uint8_t pair = 0; pair < batch_count_this_run; ++pair) { // LEA - CPU MAC
//            uint32_t dist = 0;
//            for (uint16_t dim = 0; dim < FULL_VECTOR_DIM; ++dim) {
//                int32_t d = (int32_t)batch_rerank_c[pair * FULL_VECTOR_DIM + dim];
//                dist += (uint32_t)(d * d);
//            }
//            expanded_log_dists[batch_start_i + pair] = dist;
//        }

#ifdef STAT_TIME_MIN_MAX
        uint32_t ed_distcmp = get_current_tick(LOW_RES_CLK);
        tick_rerank_cmp += (ed_distcmp - st_distcmp);
#endif
    }
#endif

    expanded_log_count = rerank_count;

#ifdef STAT_TIME_MIN_MAX
    uint32_t st_sort = get_current_tick(LOW_RES_CLK);
#endif

    for (uint32_t i = 1; i < expanded_log_count; ++i) {
        uint32_t key_id   = expanded_log[i];
        uint32_t key_dist = expanded_log_dists[i];
        int32_t j = (int32_t)i - 1;
        while (j >= 0 && expanded_log_dists[j] > key_dist) {
            expanded_log[j + 1]       = expanded_log[j];
            expanded_log_dists[j + 1] = expanded_log_dists[j];
            j--;
        }
        expanded_log[j + 1]       = key_id;
        expanded_log_dists[j + 1] = key_dist;
    }

#ifdef STAT_TIME_MIN_MAX
    uint32_t ed_sort = get_current_tick(LOW_RES_CLK);
    tick_rerank_sort += (ed_sort - st_sort);
#endif

#if RERANK_MODE == RERANK_MODE_CPU_LONGPQ
    uint32_t exact_count = (expanded_log_count < (uint32_t)LONGPQ_EXACT_TOPK) ? expanded_log_count : (uint32_t)LONGPQ_EXACT_TOPK;
    for (uint32_t i = 0; i < exact_count; ++i) {
#ifdef STAT_TIME_MIN_MAX
        uint32_t st_fullvec_resort_io = get_current_tick(LOW_RES_CLK);
#endif

        read_fullvec_from_flash(expanded_log[i], (uint8_t*)rerank_buf_fullvec);

#ifdef STAT_TIME_MIN_MAX
        uint32_t ed_fullvev_resort_io = get_current_tick(LOW_RES_CLK);
        tick_rerank_io += (ed_fullvev_resort_io - st_fullvec_resort_io);
#endif

#ifdef STAT_TIME_MIN_MAX
        uint32_t st_fullvec_resort_cmp = get_current_tick(LOW_RES_CLK);
#endif

        uint32_t exact_dist = 0;
        for (uint32_t dim = 0; dim < FULL_VECTOR_DIM; ++dim) {
            int32_t diff = (int32_t)query_vector[dim] - (int32_t)rerank_buf_fullvec[dim];
            exact_dist += (uint32_t)(diff * diff);
        }
        expanded_log_dists[i] = exact_dist;

#ifdef STAT_TIME_MIN_MAX
        uint32_t ed_fullvec_resort_cmp = get_current_tick(LOW_RES_CLK);
        tick_rerank_cmp += (ed_fullvec_resort_cmp - st_fullvec_resort_cmp);
#endif
    }

#ifdef STAT_TIME_MIN_MAX
    uint32_t st_sort2 = get_current_tick(LOW_RES_CLK);
#endif

    for (uint32_t i = 1; i < exact_count; ++i) {
        uint32_t key_id   = expanded_log[i];
        uint32_t key_dist = expanded_log_dists[i];
        int32_t j = (int32_t)i - 1;
        while (j >= 0 && expanded_log_dists[j] > key_dist) {
            expanded_log[j + 1]       = expanded_log[j];
            expanded_log_dists[j + 1] = expanded_log_dists[j];
            j--;
        }
        expanded_log[j + 1]       = key_id;
        expanded_log_dists[j + 1] = key_dist;
    }
    expanded_log_count = exact_count;

#ifdef STAT_TIME_MIN_MAX
    uint32_t ed_sort2 = get_current_tick(LOW_RES_CLK);
    tick_rerank_sort += (ed_sort2 - st_sort2);
#endif
#endif
}
#endif
