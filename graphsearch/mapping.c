#include "mapping.h"
#pragma NOINIT(L0_mapping_table)
uint32_t L0_mapping_table[L0_ENTRY_COUNT];

extern uint16_t page_in_buffer;
extern Statistics stats;
extern PHASE phase;

//void read_pqcode_from_flash(uint32_t vector_id, uint8_t* buf_pq_code)
//{
//    uint32_t group_id = vector_id / L1_ENTRY_COUNT;
//    if (group_id >= L0_ENTRY_COUNT) {
//        SET_BREAKPOINT(BP_ERROR);
//    }
//    uint32_t entry_index = vector_id % L1_ENTRY_COUNT;
//
//#if (SPI_MODE == SPI_MODE_DMA)
//#if (QPS_PROFILE_MODE == QPS_PROFILE_MODE_8)
//    read_op_dma(L0_mapping_table[group_id], L1_ENTRY_FIELD_OFFSET(entry_index, L1_PQ_CODES_OFFSET), (uint32_t)buf_pq_code, sizeof(uint8_t) * PQ_VECTOR_DIM);
//#else
//    read_op_dma(L0_mapping_table[group_id], L1_ENTRY_FIELD_OFFSET(entry_index, L1_PQ_CODES_OFFSET), (uint32_t)buf_pq_code, sizeof(uint8_t) * PQ_VECTOR_DIM);
//    read_op_dma(L0_mapping_table[group_id], L1_ENTRY_FIELD_OFFSET(entry_index, L1_PQ_CODES_LONG_OFFSET), (uint32_t)buf_pq_code, sizeof(uint8_t) * PQ_ACTIVE_DIM);
//#endif
//#else
//    read_op(L0_mapping_table[group_id], L1_ENTRY_FIELD_OFFSET(entry_index, L1_PQ_CODES_OFFSET), (uint8_t*)buf_pq_code, sizeof(uint8_t) * PQ_VECTOR_DIM);
//#endif
//}

#if RERANK_MODE == RERANK_MODE_CPU_LONGPQ
void read_pqcode_long_from_flash(uint32_t vector_id, uint8_t* buf_pqcode_long)
{
    uint32_t group_id = vector_id / L1_ENTRY_COUNT;
    if (group_id >= L0_ENTRY_COUNT) {
        SET_BREAKPOINT(BP_ERROR);
    }
    uint32_t entry_index = vector_id % L1_ENTRY_COUNT;

#if (SPI_MODE == SPI_MODE_DMA)
    read_op_dma(L0_mapping_table[group_id], L1_ENTRY_FIELD_OFFSET(entry_index, L1_PQ_CODES_LONG_OFFSET), (uint32_t)buf_pqcode_long, sizeof(uint8_t) * PQ_VECTOR_DIM * 2);
#else
    read_op(L0_mapping_table[group_id], L1_ENTRY_FIELD_OFFSET(entry_index, L1_PQ_CODES_LONG_OFFSET), buf_pqcode_long, sizeof(uint8_t) * PQ_VECTOR_DIM * 2);
#endif
#ifdef STAT_FLASH_PROFILE
    stats.flash_spi_receive_typecnt[TYPE_PQCODE] += sizeof(uint8_t) * PQ_VECTOR_DIM * 2;
#endif
}
#endif

void read_neighbors_from_flash(uint32_t vector_id, uint8_t* buf_neighbor_list)
{
    phase = PHASE_NEIGHBORLIST_IO;
    uint32_t group_id = vector_id / L1_ENTRY_COUNT;
    if (group_id >= L0_ENTRY_COUNT) {
        SET_BREAKPOINT(BP_ERROR);
    }
    uint32_t entry_index = vector_id % L1_ENTRY_COUNT;

    address target_neighborlist_address;
#if (SPI_MODE == SPI_MODE_DMA)
    read_op_dma(L0_mapping_table[group_id], L1_ENTRY_FIELD_OFFSET(entry_index, L1_NEIGHBOR_ADDR_OFFSET), (uint32_t)(&target_neighborlist_address), sizeof(address));
    read_op_dma(target_neighborlist_address.block * PAGES_PER_BLOCK + target_neighborlist_address.page, target_neighborlist_address.offset + L2_NEIGHBOR_IDS_OFFSET, (uint32_t)buf_neighbor_list, sizeof(uint32_t) * MAX_OUT_NEIGHBORS);
#else
    read_op(L0_mapping_table[group_id], L1_ENTRY_FIELD_OFFSET(entry_index, L1_NEIGHBOR_ADDR_OFFSET), (uint8_t*)&target_neighborlist_address, sizeof(address));
    read_op(target_neighborlist_address.block * PAGES_PER_BLOCK + target_neighborlist_address.page, target_neighborlist_address.offset + L2_NEIGHBOR_IDS_OFFSET, (uint8_t*)buf_neighbor_list, sizeof(uint32_t) * MAX_OUT_NEIGHBORS);
#endif
//    for(uint32_t i = 0; i < MAX_OUT_NEIGHBORS; ++i) {
//        ((uint32_t*)buf_neighbor_list)[i] = vector_id + i + 1;
//    }

#ifdef STAT_FLASH_PROFILE
    stats.flash_spi_receive_typecnt[TYPE_NEIGHBORLIST] += sizeof(uint32_t) * MAX_OUT_NEIGHBORS;
    stats.flash_spi_receive_typecnt[TYPE_L1_ADDRESS] += sizeof(address);
#endif
}

void read_fullvec_from_flash(uint32_t vector_id, uint8_t* buf_fullvec)
{
    phase = PHASE_FULLVECTOR_IO;
    uint32_t group_id = vector_id / L1_ENTRY_COUNT;
    if (group_id >= L0_ENTRY_COUNT) {
        SET_BREAKPOINT(BP_ERROR);
    }
    uint32_t entry_index = vector_id % L1_ENTRY_COUNT;

    address target_fullvec_address;
#if (SPI_MODE == SPI_MODE_DMA)
    read_op_dma(L0_mapping_table[group_id], L1_ENTRY_FIELD_OFFSET(entry_index, L1_FULLVEC_ADDR_OFFSET), (uint32_t)(&target_fullvec_address), sizeof(address));
    read_op_dma(target_fullvec_address.block * PAGES_PER_BLOCK + target_fullvec_address.page, target_fullvec_address.offset + L2_VECTOR_DATA_OFFSET, (uint32_t)buf_fullvec, sizeof(VECTOR_ELEMENT_TYPE) * FULL_VECTOR_DIM);
#else
    read_op(L0_mapping_table[group_id], L1_ENTRY_FIELD_OFFSET(entry_index, L1_FULLVEC_ADDR_OFFSET), (uint8_t*)&target_fullvec_address, sizeof(address));
    read_op(target_fullvec_address.block * PAGES_PER_BLOCK + target_fullvec_address.page, target_fullvec_address.offset + L2_VECTOR_DATA_OFFSET, (uint8_t*)buf_fullvec, sizeof(VECTOR_ELEMENT_TYPE) * FULL_VECTOR_DIM);
#endif
//    for(uint32_t i = 0; i < FULL_VECTOR_DIM; ++i) {
//        ((VECTOR_ELEMENT_TYPE*)buf_fullvec)[i] = vector_id + i + 1;
//    }
#ifdef STAT_FLASH_PROFILE
    stats.flash_spi_receive_typecnt[TYPE_FULLVECTOR] += sizeof(VECTOR_ELEMENT_TYPE) * FULL_VECTOR_DIM;
    stats.flash_spi_receive_typecnt[TYPE_L1_ADDRESS] += sizeof(address);
#endif
}
