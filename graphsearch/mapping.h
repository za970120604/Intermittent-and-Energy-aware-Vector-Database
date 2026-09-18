#ifndef GRAPHSEARCH_MAPPING_H_
#define GRAPHSEARCH_MAPPING_H_

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
#include "stddef.h"

typedef enum {
    ENTRY_TYPE_UNKNOWN = 0,
    ENTRY_TYPE_L2_VECTOR = 1,
    ENTRY_TYPE_L2_NEIGHBOR = 2,
    ENTRY_TYPE_L1_MAPPING = 3
} EntryType;

typedef struct L1_Mapping_Entry {
    uint32_t vector_id;
    address entry_info;
    address full_vector_address;
    address neighbor_list_address;
    uint8_t pq_codes[PQ_VECTOR_DIM];
    uint8_t pq_codes_long[PQ_VECTOR_DIM * 2];
} L1_Mapping_Entry;

typedef struct L2_Neighbor_Entry {
    uint32_t vector_id;
    address entry_info;
    uint32_t neighbor_ids[MAX_OUT_NEIGHBORS];
} L2_Neighbor_Entry;

typedef struct L2_Vector_Entry {
    uint32_t vector_id;
    address entry_info;
    VECTOR_ELEMENT_TYPE full_vector[FULL_VECTOR_DIM];
} L2_Vector_Entry;

#define L1_VECTOR_ID_OFFSET         0
#define L1_ENTRY_INFO_OFFSET        (L1_VECTOR_ID_OFFSET + sizeof(uint32_t))
#define L1_FULLVEC_ADDR_OFFSET      (L1_ENTRY_INFO_OFFSET + sizeof(address))
#define L1_NEIGHBOR_ADDR_OFFSET     (L1_FULLVEC_ADDR_OFFSET + sizeof(address))
#define L1_PQ_CODES_OFFSET          (L1_NEIGHBOR_ADDR_OFFSET + sizeof(address))
#define L1_PQ_CODES_LONG_OFFSET     (L1_PQ_CODES_OFFSET + (PQ_VECTOR_DIM * sizeof(uint8_t)))

#define L1_ENTRY_FIELD_OFFSET(entry_idx, field_offset) \
    ((entry_idx) * sizeof(L1_Mapping_Entry) + (field_offset))

#define L2_NEIGHBOR_VECTOR_ID_OFFSET    0
#define L2_NEIGHBOR_ENTRY_INFO_OFFSET   (L2_NEIGHBOR_VECTOR_ID_OFFSET + sizeof(uint32_t))
#define L2_NEIGHBOR_IDS_OFFSET          (L2_NEIGHBOR_ENTRY_INFO_OFFSET + sizeof(address))
#define L2_VECTOR_VECTOR_ID_OFFSET      0
#define L2_VECTOR_ENTRY_INFO_OFFSET     (L2_VECTOR_VECTOR_ID_OFFSET + sizeof(uint32_t))
#define L2_VECTOR_DATA_OFFSET           (L2_VECTOR_ENTRY_INFO_OFFSET + sizeof(address))

// void read_pqcode_from_flash(uint32_t vector_id, uint8_t* buf_pq_code);
void read_neighbors_from_flash(uint32_t vector_id, uint8_t* buf_neighbor_list);
void read_fullvec_from_flash(uint32_t vector_id, uint8_t* buf_fullvec);

#if RERANK_MODE == RERANK_MODE_CPU_LONGPQ
void read_pqcode_long_from_flash(uint32_t vector_id, uint8_t* buf_pq_code_long);
#endif

extern uint32_t L0_mapping_table[L0_ENTRY_COUNT];
extern BREAKPOINT_TYPE breakpoint;

#endif /* GRAPHSEARCH_MAPPING_H_ */
