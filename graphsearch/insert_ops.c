//#include "insert_ops.h"
//
//#pragma PERSISTENT(insert_log)
//uint8_t insert_log[INSERT_BUFFER_SIZE] = {0};
//#pragma PERSISTENT(insert_log_head)
//uint8_t *insert_log_head = (uint8_t*)insert_log;
//#pragma PERSISTENT(insert_log_hash)
//InsertLogHashEntryDescriptor *insert_log_hash[INSERT_LOG_HASH_SIZE] = {NULL};
//
//#pragma NOINIT(pending_node_updates)
//PendingNodeUpdate pending_node_updates[MAX_PENDING_NODE_UPDATES];
//#pragma PERSISTENT(pending_node_update_head)
//uint8_t* pending_node_update_head = (uint8_t*)pending_node_updates;
//
//#pragma NOINIT(reverse_L2_vector_data)
//VECTOR_ELEMENT_TYPE reverse_L2_vector_data[FULL_VECTOR_DIM];
//#pragma NOINIT(reverse_L2_neighbor_list)
//uint32_t reverse_L2_neighbor_list[MAX_OUT_NEIGHBORS];
//#pragma NOINIT(reverse_L2_candidates)
//uint32_t reverse_L2_candidates[MAX_OUT_NEIGHBORS + MAX_NEW_NEIGHBORS_PER_NODE];
//
//#pragma NOINIT(insert_batch_group_bitmap)
//uint32_t insert_batch_group_bitmap[(L0_ENTRY_COUNT + 31) / 32];
//
//#pragma NOINIT(apply_insert_pq_codes)
//uint8_t apply_insert_pq_codes[PQ_VECTOR_DIM];
//
//uint32_t apply_insert_changes() {
////    uint8_t pq_codes[PQ_VECTOR_DIM];
//    uint8_t* insert_log_iter = insert_log;
//    uint32_t total_new_vectors = 0;
////    uint32_t total_new_vectors = ((uintptr_t)insert_log_head - (uintptr_t)insert_log) / (sizeof(InsertLogHashEntryDescriptor) + sizeof(VECTOR_ELEMENT_TYPE) * FULL_VECTOR_DIM);
//
//    uint32_t l2_entry_pair_size = sizeof(L2_Vector_Entry) + sizeof(L2_Neighbor_Entry);
//    uint32_t max_l2_entries = L2_FLASH_BUFFER_SIZE / l2_entry_pair_size;
//    uint32_t max_l1_groups = L1_FLASH_BUFFER_SIZE / PAGE_SIZE;
////    volatile UBaseType_t wm;
//
//    while (insert_log_iter < insert_log_head) {
////        // Calculate batch capacity
//        uint32_t batch_count = calculate_insertlog_batch_count(insert_log_iter, insert_log_head, max_l2_entries, max_l1_groups);
////
//        if (batch_count == 0) {
//            SET_BREAKPOINT(BP_ERROR);
//        }
////
//        // Collect L2 data
//
////        wm = uxTaskGetStackHighWaterMark(NULL);
////        if (replay_head == 30) {
////           __no_operation();
////        }
//        uint8_t* next_iter = collect_L2_for_new_vectors(insert_log_iter, batch_count, apply_insert_pq_codes);
////        wm = uxTaskGetStackHighWaterMark(NULL);
////        if (replay_head == 30) {
////           __no_operation();
////        }
////
//        // Flush L2
////        wm = uxTaskGetStackHighWaterMark(NULL);
////        if (replay_head == 30) {
////           __no_operation();
////        }
////        address l2_flush_start = flush_buffer(L2_flash_buffer, &L2_buffer_head, L2_FLASH_BUFFER_SIZE);
////        wm = uxTaskGetStackHighWaterMark(NULL);
////        if (replay_head == 30) {
////           __no_operation();
////        }
////        CHECKPOINT(CHECKPOINT_L2_FLUSHED);
////
////        wm = uxTaskGetStackHighWaterMark(NULL);
////        if (replay_head == 30) {
////           __no_operation();
////        }
////        fill_L1_for_new_vectors(insert_log_iter, batch_count, l2_flush_start, l2_entry_pair_size, apply_insert_pq_codes);
////        wm = uxTaskGetStackHighWaterMark(NULL);
////        if (replay_head == 30) {
////           __no_operation();
////        }
////
////        // Flush L1
////        wm = uxTaskGetStackHighWaterMark(NULL);
////        if (replay_head == 30) {
////           __no_operation();
////        }
////        flush_buffer(L1_flash_buffer, &L1_buffer_head, L1_FLASH_BUFFER_SIZE);
////        wm = uxTaskGetStackHighWaterMark(NULL);
////        if (replay_head == 30) {
////           __no_operation();
////        }
//////        CHECKPOINT(CHECKPOINT_L1_FLUSHED);
////
//        total_new_vectors += batch_count;
//        insert_log_iter = next_iter;
//    }
//
//    uint32_t max_vector_id_after_insert = current_max_vector_id + total_new_vectors;
////    volatile uint32_t group_id;
////    volatile uint32_t entry_index;
////    volatile L2_Neighbor_Entry temp_check_l2_neighbor_entry;
////    volatile L2_Vector_Entry temp_check_l2_vector_entry;
////    volatile L1_Mapping_Entry temp_check_l1_mapping_entry;
////    for (uint32_t vector_id = 0; vector_id <= max_vector_id_after_insert; ++vector_id) {
////        group_id = vector_id / 64;
////        entry_index = vector_id % 64;
////
////        address target_neighborlist_address;
////        read_op(L0_mapping_table[group_id], (entry_index * 32 + 12), (uint8_t*)&target_neighborlist_address, sizeof(address));
////        read_op(target_neighborlist_address.block * PAGES_PER_BLOCK + target_neighborlist_address.page, target_neighborlist_address.offset, (uint8_t*)&temp_check_l2_neighbor_entry, sizeof(L2_Neighbor_Entry));
////        if (temp_check_l2_neighbor_entry.entry_info.type != ENTRY_TYPE_L2_NEIGHBOR || temp_check_l2_neighbor_entry.vector_id != vector_id) {
////            SET_BREAKPOINT(BP_ERROR);
////        }
////
////        address target_fullvec_address;
////        read_op(L0_mapping_table[group_id], (entry_index * 32 + 8), (uint8_t*)&target_fullvec_address, sizeof(address));
////        read_op(target_fullvec_address.block * PAGES_PER_BLOCK + target_fullvec_address.page, target_fullvec_address.offset, (uint8_t*)&temp_check_l2_vector_entry, sizeof(L2_Vector_Entry));
////        if (temp_check_l2_vector_entry.entry_info.type != ENTRY_TYPE_L2_VECTOR || temp_check_l2_vector_entry.vector_id != vector_id) {
////            SET_BREAKPOINT(BP_ERROR);
////        }
////        for (uint32_t j = 0; j < FULL_VECTOR_DIM; ++j) {
////            if (temp_check_l2_vector_entry.full_vector[j] != j) {
////                SET_BREAKPOINT(BP_ERROR);
////            }
////        }
////
////        read_op(L0_mapping_table[group_id], (entry_index * 32), (uint8_t*)&temp_check_l1_mapping_entry, sizeof(L1_Mapping_Entry));
////        if (temp_check_l1_mapping_entry.entry_info.type != ENTRY_TYPE_L1_MAPPING || temp_check_l1_mapping_entry.vector_id != vector_id) {
////            SET_BREAKPOINT(BP_ERROR);
////        }
////        for (uint32_t j = 0; j < PQ_VECTOR_DIM; ++j) {
////            if (temp_check_l1_mapping_entry.pq_codes[j] != j) {
////                SET_BREAKPOINT(BP_ERROR);
////            }
////        }
////    }
////
//    uint32_t total_pending = ((uintptr_t)pending_node_update_head - (uintptr_t)pending_node_updates) / sizeof(PendingNodeUpdate);
//    uint32_t pending_index = 0;
//    uint32_t max_l2_neighbors = L2_FLASH_BUFFER_SIZE / sizeof(L2_Neighbor_Entry);
////
//    while (pending_index < total_pending) {
//        // Calculate batch capacity
//        uint32_t batch_count = calculate_reverse_edge_batch_count(pending_index, total_pending, max_l2_neighbors, max_l1_groups);
//        if (batch_count == 0) {
//            SET_BREAKPOINT(BP_ERROR);
//        }
//
//        // Collect L2 data (neighbors only)
//        collect_L2_for_reverse_edges(pending_index, batch_count);
//
//        // Flush L2
////        address l2_flush_start = flush_buffer(L2_flash_buffer, &L2_buffer_head, L2_FLASH_BUFFER_SIZE);
////        CHECKPOINT(CHECKPOINT_L2_REVERSE_FLUSHED);
//
//        // Fill L1 entries
////        fill_L1_for_reverse_edges(pending_index, batch_count, l2_flush_start, apply_insert_pq_codes);
//
//        // Flush L1
////        flush_buffer(L1_flash_buffer, &L1_buffer_head, L1_FLASH_BUFFER_SIZE);
////        CHECKPOINT(CHECKPOINT_L1_REVERSE_FLUSHED);
//        pending_index += batch_count;
//    }
////
////    for(uint32_t vector_id = 0; vector_id <= max_vector_id_after_insert; ++vector_id) {
////        group_id = vector_id / 64;
////        entry_index = vector_id % 64;
////
////        address target_neighborlist_address;
////        read_op(L0_mapping_table[group_id], (entry_index * 32 + 12), (uint8_t*)&target_neighborlist_address, sizeof(address));
////        read_op(target_neighborlist_address.block * PAGES_PER_BLOCK + target_neighborlist_address.page, target_neighborlist_address.offset, (uint8_t*)&temp_check_l2_neighbor_entry, sizeof(L2_Neighbor_Entry));
////        if (temp_check_l2_neighbor_entry.entry_info.type != ENTRY_TYPE_L2_NEIGHBOR || temp_check_l2_neighbor_entry.vector_id != vector_id) {
////            SET_BREAKPOINT(BP_ERROR);
////        }
////
////        address target_fullvec_address;
////        read_op(L0_mapping_table[group_id], (entry_index * 32 + 8), (uint8_t*)&target_fullvec_address, sizeof(address));
////        read_op(target_fullvec_address.block * PAGES_PER_BLOCK + target_fullvec_address.page, target_fullvec_address.offset, (uint8_t*)&temp_check_l2_vector_entry, sizeof(L2_Vector_Entry));
////        if (temp_check_l2_vector_entry.entry_info.type != ENTRY_TYPE_L2_VECTOR || temp_check_l2_vector_entry.vector_id != vector_id) {
////            SET_BREAKPOINT(BP_ERROR);
////        }
////        for (uint32_t j = 0; j < FULL_VECTOR_DIM; ++j) {
////            if (temp_check_l2_vector_entry.full_vector[j] != j) {
////                SET_BREAKPOINT(BP_ERROR);
////            }
////        }
////
////        read_op(L0_mapping_table[group_id], (entry_index * 32), (uint8_t*)&temp_check_l1_mapping_entry, sizeof(L1_Mapping_Entry));
////        if (temp_check_l1_mapping_entry.entry_info.type != ENTRY_TYPE_L1_MAPPING || temp_check_l1_mapping_entry.vector_id != vector_id) {
////            SET_BREAKPOINT(BP_ERROR);
////        }
////        for (uint32_t j = 0; j < PQ_VECTOR_DIM; ++j) {
////            if (temp_check_l1_mapping_entry.pq_codes[j] != j) {
////                SET_BREAKPOINT(BP_ERROR);
////            }
////        }
////    }
//
//    insert_log_head = (uint8_t*)insert_log;
//    for (uint32_t i = 0; i < INSERT_LOG_HASH_SIZE; ++i) {
//        insert_log_hash[i] = NULL;
//    }
//    memset(pending_node_updates, 0, sizeof(pending_node_updates));
//    pending_node_update_head = (uint8_t*)pending_node_updates;
//    return max_vector_id_after_insert;
//}
//
//
//void add_pending_reverse_edge(uint32_t vector_id, uint32_t new_neighbor_id) {
//    PendingNodeUpdate* iter = pending_node_updates;
//    while((uintptr_t)iter < (uintptr_t)pending_node_update_head) {
//        if(iter->vector_id == vector_id) {
//            for (uint32_t j = 0; j < iter->new_neighbor_count; ++j) {
//                if (iter->new_neighbors[j] == new_neighbor_id) {
//                    return;
//                }
//            }
//
//            if (iter->new_neighbor_count < MAX_NEW_NEIGHBORS_PER_NODE) {
//                iter->new_neighbors[iter->new_neighbor_count] = new_neighbor_id;
//                ++iter->new_neighbor_count;
//                return;
//            }
//            else{
//                SET_BREAKPOINT(BP_ERROR);
//            }
//        }
//        iter += 1;
//    }
//
//    if ((uintptr_t)(pending_node_update_head + sizeof(PendingNodeUpdate)) <= (uintptr_t)&pending_node_updates[MAX_PENDING_NODE_UPDATES]) {
//        ((PendingNodeUpdate*)pending_node_update_head)->vector_id = vector_id;
//        ((PendingNodeUpdate*)pending_node_update_head)->new_neighbors[0] = new_neighbor_id;
//        ((PendingNodeUpdate*)pending_node_update_head)->new_neighbor_count = 1;
//        pending_node_update_head += sizeof(PendingNodeUpdate);
//    }
//    else {
//        SET_BREAKPOINT(BP_ERROR);
//    }
//}
//
//uint32_t calculate_insertlog_batch_count(uint8_t* start_iter, uint8_t* end_iter, uint32_t max_l2_entries, uint32_t max_l1_groups) {
//    uint32_t batch_count = 0;
//    uint32_t unique_groups = 0;
////    uint32_t group_bitmap[(L0_ENTRY_COUNT + 31) / 32] = {0};
//    uint8_t* iter = start_iter;
//
//    for(uint32_t i = 0; i < (L0_ENTRY_COUNT + 31) / 32; ++i) {
//        insert_batch_group_bitmap[i] = 0;
//    }
//
//    while (iter < end_iter) {
//        InsertLogHashEntryDescriptor* entry = (InsertLogHashEntryDescriptor*)iter;
//        uint32_t vector_id = entry->vector_id;
//        uint32_t group_id = vector_id / L1_ENTRY_COUNT;
//
//        uint32_t bitmap_index = group_id / 32;
//        uint32_t bitmap_bit = group_id % 32;
//        if (!(insert_batch_group_bitmap[bitmap_index] & (1U << bitmap_bit))) {
//            insert_batch_group_bitmap[bitmap_index] |= (1U << bitmap_bit);
//            ++unique_groups;
//
//            if (unique_groups > max_l1_groups) {
//                break;
//            }
//        }
//
//        ++batch_count;
//
//        if (batch_count >= max_l2_entries) {
//            break;
//        }
//
//        iter += sizeof(InsertLogHashEntryDescriptor) + sizeof(VECTOR_ELEMENT_TYPE) * FULL_VECTOR_DIM;
//    }
//
//    return batch_count;
//}
//
//uint32_t calculate_reverse_edge_batch_count(uint32_t start_index, uint32_t total_count, uint32_t max_l2_neighbors, uint32_t max_l1_groups) {
//    uint32_t batch_count = 0;
//    uint32_t unique_groups = 0;
////    uint32_t group_bitmap[(L0_ENTRY_COUNT + 31) / 32] = {0};
//
//    for(uint32_t i = 0; i < (L0_ENTRY_COUNT + 31) / 32; ++i) {
//        insert_batch_group_bitmap[i] = 0;
//    }
//
//    for (uint32_t i = start_index; i < total_count && batch_count < max_l2_neighbors; ++i) {
//        PendingNodeUpdate* update = &pending_node_updates[i];
//        uint32_t group_id = update->vector_id / L1_ENTRY_COUNT;
//
//        uint32_t bitmap_index = group_id / 32;
//        uint32_t bitmap_bit = group_id % 32;
//        if (!(insert_batch_group_bitmap[bitmap_index] & (1U << bitmap_bit))) {
//            insert_batch_group_bitmap[bitmap_index] |= (1U << bitmap_bit);
//            ++unique_groups;
//
//            if (unique_groups > max_l1_groups) {
//                break;
//            }
//        }
//
//        ++batch_count;
//    }
//
//    return batch_count;
//}
//
//uint8_t* collect_L2_for_new_vectors(uint8_t* batch_start, uint32_t batch_size, uint8_t* pq_codes) {
//    uint8_t* batch_iter = batch_start;
////    volatile UBaseType_t wm;
//
//    for (uint32_t i = 0; i < batch_size; ++i) {
//        InsertLogHashEntryDescriptor* entry = (InsertLogHashEntryDescriptor*)batch_iter;
//        uint32_t vector_id = entry->vector_id;
//        VECTOR_ELEMENT_TYPE* vector_data = (VECTOR_ELEMENT_TYPE*)(entry->addr);
//
////        wm = uxTaskGetStackHighWaterMark(NULL);
////        if (replay_head == 30) {
////           __no_operation();
////        }
//        compute_pq_codes(vector_data, pq_codes);
////        wm = uxTaskGetStackHighWaterMark(NULL);
////        if (replay_head == 30) {
////           __no_operation();
////        }
//        query(vector_data, delta_graph_entry_vecID);
////        wm = uxTaskGetStackHighWaterMark(NULL);
////        if (replay_head == 30) {
////           __no_operation();
////        }
//        robust_prune(vector_data, vector_id, search_result_ids, TOP_K_VALUE);
////        wm = uxTaskGetStackHighWaterMark(NULL);
////        if (replay_head == 30) {
////           __no_operation();
////        }
//        upsert_L2_vector(vector_id, vector_data);
////        wm = uxTaskGetStackHighWaterMark(NULL);
////        if (replay_head == 30) {
////           __no_operation();
////        }
//        upsert_L2_neighbors(vector_id, search_result_ids, TOP_K_VALUE);
////        wm = uxTaskGetStackHighWaterMark(NULL);
////        if (replay_head == 30) {
////           __no_operation();
////        }
//
//        for (uint32_t j = 0; j < TOP_K_VALUE; ++j) {
//            if (search_result_ids[j] != 0 && search_result_ids[j] <= current_max_vector_id) {
//                add_pending_reverse_edge(search_result_ids[j], vector_id);
//            }
//        }
//
//        batch_iter += sizeof(InsertLogHashEntryDescriptor) + sizeof(VECTOR_ELEMENT_TYPE) * FULL_VECTOR_DIM;
//    }
//
//    return batch_iter;
//}
//
//void fill_L1_for_new_vectors(uint8_t* batch_start, uint32_t batch_size, address l2_flush_start, uint32_t l2_entry_pair_size, uint8_t* pq_codes) {
//    uint8_t* batch_iter = batch_start;
//    uint32_t entry_index = 0;
//
//    for (uint32_t i = 0; i < batch_size; ++i) {
//        InsertLogHashEntryDescriptor* entry = (InsertLogHashEntryDescriptor*)batch_iter;
//        uint32_t vector_id = entry->vector_id;
//        VECTOR_ELEMENT_TYPE* vector_data = (VECTOR_ELEMENT_TYPE*)(entry->addr);
//
//        compute_pq_codes(vector_data, pq_codes);
//
//        uint32_t entry_base_offset = entry_index * l2_entry_pair_size;
//        uint32_t vec_offset = entry_base_offset;
//        uint32_t neigh_offset = entry_base_offset + sizeof(L2_Vector_Entry);
//
//        address full_vec_addr;
//        full_vec_addr.block = (l2_flush_start.block * PAGES_PER_BLOCK + l2_flush_start.page + (vec_offset / PAGE_SIZE)) / PAGES_PER_BLOCK;
//        full_vec_addr.page = (l2_flush_start.block * PAGES_PER_BLOCK + l2_flush_start.page + (vec_offset / PAGE_SIZE)) % PAGES_PER_BLOCK;
//        full_vec_addr.offset = vec_offset % PAGE_SIZE;
//        full_vec_addr.type = ENTRY_TYPE_L2_VECTOR;
//
//        address neighbor_addr;
//        neighbor_addr.block = (l2_flush_start.block * PAGES_PER_BLOCK + l2_flush_start.page + (neigh_offset / PAGE_SIZE)) / PAGES_PER_BLOCK;
//        neighbor_addr.page = (l2_flush_start.block * PAGES_PER_BLOCK + l2_flush_start.page + (neigh_offset / PAGE_SIZE)) % PAGES_PER_BLOCK;
//        neighbor_addr.offset = neigh_offset % PAGE_SIZE;
//        neighbor_addr.type = ENTRY_TYPE_L2_NEIGHBOR;
//
//        upsert_L1_entry(vector_id, pq_codes, full_vec_addr, neighbor_addr);
//
//        ++entry_index;
//        batch_iter += sizeof(InsertLogHashEntryDescriptor) + sizeof(VECTOR_ELEMENT_TYPE) * FULL_VECTOR_DIM;
//    }
//}
//
//void collect_L2_for_reverse_edges(uint32_t start_index, uint32_t batch_size) {
//    for (uint32_t i = 0; i < batch_size; ++i) {
//        PendingNodeUpdate* iter = &pending_node_updates[start_index + i];
//        uint32_t vector_id = iter->vector_id;
//
////        VECTOR_ELEMENT_TYPE vector_data[FULL_VECTOR_DIM];
//        read_fullvec_from_flash(vector_id, (uint8_t*)reverse_L2_vector_data);
//
////        uint32_t buf_neighbor_list[MAX_OUT_NEIGHBORS];
//        read_neighbors_from_flash(vector_id, (uint8_t*)reverse_L2_neighbor_list);
//
////        uint32_t candidates[MAX_OUT_NEIGHBORS + MAX_NEW_NEIGHBORS_PER_NODE];
//        uint32_t candidate_count = 0;
//
//        for (uint32_t j = 0; j < MAX_OUT_NEIGHBORS && reverse_L2_neighbor_list[j] != 0 && reverse_L2_neighbor_list[j] <= current_max_vector_id; ++j) {
//            reverse_L2_candidates[candidate_count] = reverse_L2_neighbor_list[j];
//            ++candidate_count;
//        }
//
//        for (uint32_t j = 0; j < iter->new_neighbor_count && iter->new_neighbors[j] != 0 && iter->new_neighbors[j] <= current_max_vector_id; ++j) {
//            reverse_L2_candidates[candidate_count] = iter->new_neighbors[j];
//            ++candidate_count;
//        }
//
//        robust_prune(reverse_L2_vector_data, vector_id, reverse_L2_candidates, candidate_count);
//        upsert_L2_neighbors(vector_id, search_result_ids, TOP_K_VALUE);
//    }
//}
//
//void fill_L1_for_reverse_edges(uint32_t start_index, uint32_t batch_size, address l2_flush_start, uint8_t* pq_codes) {
//    for (uint32_t i = 0; i < batch_size; ++i) {
//        PendingNodeUpdate* iter = &pending_node_updates[start_index + i];
//        uint32_t vector_id = iter->vector_id;
//
//        uint32_t neigh_offset = i * sizeof(L2_Neighbor_Entry);
//
//        address neighbor_addr;
//        neighbor_addr.block = (l2_flush_start.block * PAGES_PER_BLOCK + l2_flush_start.page + (neigh_offset / PAGE_SIZE)) / PAGES_PER_BLOCK;
//        neighbor_addr.page = (l2_flush_start.block * PAGES_PER_BLOCK + l2_flush_start.page + (neigh_offset / PAGE_SIZE)) % PAGES_PER_BLOCK;
//        neighbor_addr.offset = neigh_offset % PAGE_SIZE;
//        neighbor_addr.type = ENTRY_TYPE_L2_NEIGHBOR;
//
//        read_pqcode_from_flash(vector_id, pq_codes);
//        address old_vec_addr;
//        read_fullvec_info_from_flash(vector_id, (uint8_t*)&old_vec_addr);
//
//        upsert_L1_entry(vector_id, pq_codes, old_vec_addr, neighbor_addr);
//    }
//}
