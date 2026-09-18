//#include "garbage_collection.h"
//#pragma NOINIT(gc_flush_l2_pq_codes);
//uint8_t gc_flush_l2_pq_codes[PQ_VECTOR_DIM];
//#pragma NOINIT(gc_group_bitmap)
//uint32_t gc_group_bitmap[(L0_ENTRY_COUNT + 31) / 32];
//
//void gc_read_l1_addr_field(uint32_t vector_id, uint16_t field_offset, address *buf_l1_addr) {
//    uint32_t group_id  = vector_id / L1_ENTRY_COUNT;
//    uint32_t entry_idx = vector_id % L1_ENTRY_COUNT;
//
//    // try to fetch from fram L1 flash buffer
//    uint8_t *iter = L1_flash_buffer;
//    while (iter < L1_buffer_head) {
//        L1_Mapping_Entry *first_entry = (L1_Mapping_Entry *)iter;
//        if (first_entry->entry_info.type != ENTRY_TYPE_L1_MAPPING) {
//            SET_BREAKPOINT(BP_ERROR);
//        }
//
//        if (first_entry->vector_id / L1_ENTRY_COUNT == group_id) {
//            L1_Mapping_Entry *target = (L1_Mapping_Entry *)(iter + entry_idx * sizeof(L1_Mapping_Entry));
//            if (target->vector_id != vector_id && vector_id <= current_max_vector_id) {
//               SET_BREAKPOINT(BP_ERROR);
//            }
//
//            *buf_l1_addr = *(address *)((uint8_t *)target + field_offset);
//            return;
//        }
//        iter += PAGE_SIZE;
//    }
//
//    // if failed, try to fetch from flash
//    if (group_id >= L0_ENTRY_COUNT) {
//        SET_BREAKPOINT(BP_ERROR);
//    }
//    read_op(L0_mapping_table[group_id], L1_ENTRY_FIELD_OFFSET(entry_idx, field_offset), (uint8_t *)buf_l1_addr, sizeof(address));
//}
//
//void gc_ensure_l1_buffer_has_room(uint32_t group_id) {
//    /* Check if group is already in buffer - if so, no action needed */
//    uint8_t *iter = L1_flash_buffer;
//    while (iter < L1_buffer_head) {
//        L1_Mapping_Entry *first_entry = (L1_Mapping_Entry *)iter;
//        if (first_entry->entry_info.type != ENTRY_TYPE_L1_MAPPING) {
//            SET_BREAKPOINT(BP_ERROR);
//        }
//
//        if (first_entry->vector_id / L1_ENTRY_COUNT == group_id) {
//            return;
//        }
//        iter += PAGE_SIZE;
//    }
//
//    /* Group not in buffer.  Flush if buffer is full. */
//    uint32_t pages_used = (uint32_t)((uintptr_t)L1_buffer_head - (uintptr_t)L1_flash_buffer) / PAGE_SIZE;
//    if (pages_used >= (L1_FLASH_BUFFER_SIZE / PAGE_SIZE)) {
//        flush_buffer(L1_flash_buffer, &L1_buffer_head, L1_FLASH_BUFFER_SIZE);
//    }
//}
//
//void gc_flush_l2_batch_and_update_l1(uint32_t bytes_in_l2) {
//    if (bytes_in_l2 == 0) {
//        return;
//    }
//
//    /* Step 1: flush L2 to flash ----------------------------------------- */
//    address l2_flush_start = flush_buffer(L2_flash_buffer, &L2_buffer_head, L2_FLASH_BUFFER_SIZE); // L2_buffer_head is now reset; L2_flash_buffer memory still holds data
//
//    /* Step 2 + 3: re-scan buffer, compute addresses, update L1 ----------- */
//    uint8_t *iter = L2_flash_buffer;
//    uint32_t byte_offset = 0;
//
//    while (byte_offset < bytes_in_l2) {
//        address *entry_info_ptr = (address *)(iter + sizeof(uint32_t));
//        uint32_t entry_size;
//
//        if (entry_info_ptr->type == ENTRY_TYPE_L2_VECTOR) {
//            entry_size = sizeof(L2_Vector_Entry);
//        }
//        else if (entry_info_ptr->type == ENTRY_TYPE_L2_NEIGHBOR) {
//            entry_size = sizeof(L2_Neighbor_Entry);
//        }
//        else {
//            break;   /* padding / end of valid data */
//        }
//
//        uint32_t vector_id = *((uint32_t *)iter);
//        uint32_t group_id  = vector_id / L1_ENTRY_COUNT;
//
//        /*
//         * Compute new flash address - same formula as fill_L1_for_new_vectors:
//         *
//         *   abs_page  = l2_start_abs + byte_offset / PAGE_SIZE
//         *   new_offset = byte_offset % PAGE_SIZE
//         */
//        address new_addr;
//        new_addr.block = (l2_flush_start.block * PAGES_PER_BLOCK + l2_flush_start.page + (byte_offset / PAGE_SIZE)) / PAGES_PER_BLOCK;
//        new_addr.page = (l2_flush_start.block * PAGES_PER_BLOCK + l2_flush_start.page + (byte_offset / PAGE_SIZE)) % PAGES_PER_BLOCK;
//        new_addr.offset = byte_offset % PAGE_SIZE;
//        new_addr.type = entry_info_ptr->type;
//        /*
//         * Read the "other half" address from L1 (buffer-first).
//         *
//         * Example: if we moved the L2_Vector_Entry for vector 5 to new_addr,
//         *   we need the CURRENT neighbor_list_address (unchanged) so we can
//         *   write upsert_L1_entry(5, pq, new_vec_addr, old_neigh_addr).
//         */
//        address other_addr;
//        if (entry_info_ptr->type == ENTRY_TYPE_L2_VECTOR) {
//            gc_read_l1_addr_field(vector_id, L1_NEIGHBOR_ADDR_OFFSET, &other_addr);
//        }
//        else {
//            gc_read_l1_addr_field(vector_id, L1_FULLVEC_ADDR_OFFSET,  &other_addr);
//        }
//
//        read_pqcode_from_flash(vector_id, gc_flush_l2_pq_codes);
//
//        /* Guarantee L1_flash_buffer has room for this group */
//        gc_ensure_l1_buffer_has_room(group_id);
//
//        /* Update L1 - upsert_L1_entry(vector id, pq code, full_vec_addr, neigh_addr) */
//        if (entry_info_ptr->type == ENTRY_TYPE_L2_VECTOR) {
//            upsert_L1_entry(vector_id, gc_flush_l2_pq_codes, new_addr, other_addr);
//        }
//        else {
//            upsert_L1_entry(vector_id, gc_flush_l2_pq_codes, other_addr, new_addr);
//        }
//
//        iter += entry_size;
//        byte_offset += entry_size;
//    }
//
//    /* Step 4: flush all dirty L1 pages ----------------------------------- */
//    if (L1_buffer_head != L1_flash_buffer) {
//        flush_buffer(L1_flash_buffer, &L1_buffer_head, L1_FLASH_BUFFER_SIZE);
//    }
//}
