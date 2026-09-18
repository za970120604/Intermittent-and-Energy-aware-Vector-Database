//#include "merge_ops.h"
//#pragma NOINIT(merge_batch_group_bitmap)
//uint32_t merge_batch_group_bitmap[(L0_ENTRY_COUNT + 31) / 32];
//#pragma NOINIT(merge_buf_fullvec_list)
//VECTOR_ELEMENT_TYPE merge_buf_fullvec_list[FULL_VECTOR_DIM];
//
//uint32_t calculate_delta_vector_batch_capacity(uint32_t start_vector_id, uint32_t end_vector_id, uint32_t max_l2_entries, uint32_t max_l1_groups) {
//    uint32_t batch_count = 0;
//    uint32_t unique_groups = 0;
////    uint32_t group_bitmap[(L0_ENTRY_COUNT + 31) / 32] = {0};
//
//    for(uint32_t i = 0; i < (L0_ENTRY_COUNT + 31) / 32; ++i) {
//        merge_batch_group_bitmap[i] = 0;
//    }
//
//    for (uint32_t vector_id = start_vector_id; vector_id < end_vector_id && batch_count < max_l2_entries; ++vector_id) {
//        uint32_t group_id = vector_id / L1_ENTRY_COUNT;
//
//        uint32_t bitmap_index = group_id / 32;
//        uint32_t bitmap_bit = group_id % 32;
//        if (!(merge_batch_group_bitmap[bitmap_index] & (1U << bitmap_bit))) {
//            merge_batch_group_bitmap[bitmap_index] |= (1U << bitmap_bit);
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
//void collect_L2_for_delta_vectors(uint32_t start_vector_id, uint32_t batch_size, uint8_t* pq_codes) {
//    for (uint32_t i = 0; i < batch_size; ++i) {
//        uint32_t vector_id = start_vector_id + i;
//
////        VECTOR_ELEMENT_TYPE buf_fullvec_list[FULL_VECTOR_DIM];
//        read_fullvec_from_flash(vector_id, (uint8_t*)merge_buf_fullvec_list);
//        query(merge_buf_fullvec_list, base_graph_entry_vecID);
//        robust_prune(merge_buf_fullvec_list, vector_id, search_result_ids, TOP_K_VALUE);
//
//        upsert_L2_vector(vector_id, merge_buf_fullvec_list);
//        upsert_L2_neighbors(vector_id, search_result_ids, TOP_K_VALUE);
//
//        for (uint32_t j = 0; j < TOP_K_VALUE; ++j) {
//            if (search_result_ids[j] != 0 && search_result_ids[j] <= current_max_vector_id) {
//                add_pending_reverse_edge(search_result_ids[j], vector_id);
//            }
//        }
//    }
//}
//
//void fill_L1_for_delta_vectors(uint32_t start_vector_id, uint32_t batch_size, address l2_flush_start, uint32_t l2_entry_pair_size, uint8_t* pq_codes) {
//    for (uint32_t i = 0; i < batch_size; ++i) {
//        uint32_t vector_id = start_vector_id + i;
//
//        // Read PQ codes (they should still be valid)
//        read_pqcode_from_flash(vector_id, pq_codes);
//
//        // Calculate L2 addresses
//        // L2 buffer pattern: [vec0][neigh0][vec1][neigh1]...
//        uint32_t entry_base_offset = i * l2_entry_pair_size;
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
//        // Update L1 entry (both vector and neighbor addresses change!)
//        upsert_L1_entry(vector_id, pq_codes, full_vec_addr, neighbor_addr);
//    }
//}
