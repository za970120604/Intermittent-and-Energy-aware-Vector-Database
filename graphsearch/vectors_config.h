#ifndef VECTORS_CONFIG_H
#define VECTORS_CONFIG_H
#include "spi_nand.h"
#define FULL_VECTOR_DIM 64

typedef union address {
    uint32_t all;
    struct
    {
        uint32_t offset : 11;
        uint32_t page   : 6;
        uint32_t block  : 10;
        uint32_t type   : 5;
    };
} address;

#ifndef VECTOR_ELEMENT_TYPE
typedef int16_t VECTOR_ELEMENT_TYPE;
#endif

#define PQ_VECTOR_DIM 16 // it equals number of subspaces
#define SUB_VECTOR_DIM (FULL_VECTOR_DIM/PQ_VECTOR_DIM)
#define CENTROID_PER_SUBSPACE 256
#define DISTANCE_TABLE_ELEM_COUNT (PQ_VECTOR_DIM * CENTROID_PER_SUBSPACE)
#define CODEBOOK_ELEM_COUNT (PQ_VECTOR_DIM * CENTROID_PER_SUBSPACE * SUB_VECTOR_DIM)

#if QPS_PROFILE_MODE == QPS_PROFILE_MODE_LONG
#define PQ_ACTIVE_DIM  (PQ_VECTOR_DIM * 2)
#define SUB_ACTIVE_DIM (FULL_VECTOR_DIM / PQ_ACTIVE_DIM)
#endif

#if WORKLOAD_DATA == DATA_SIFT
#define MAX_VECTOR_COUNT (200000 * 1UL)
#elif WORKLOAD_DATA == DATA_FAKE
#define MAX_VECTOR_COUNT (100 * 1UL)
#endif

#define L1_ENTRY_SIZE sizeof(L1_Mapping_Entry)
#define L1_ENTRY_COUNT (PAGE_SIZE / L1_ENTRY_SIZE)
#define L0_ENTRY_COUNT ((MAX_VECTOR_COUNT + L1_ENTRY_COUNT - 1) / L1_ENTRY_COUNT)

#define SEARCH_BUFFER_SIZE 10000 // FRAM2 // 23000
#define SEARCH_BUFFER_EXT_SIZE 10000 // FRAM
#define RERANK_BATCH_B 14

#define MAX_GREEDY_SEARCH_ITER 50
#define MAX_OUT_NEIGHBORS 32
#define MAX_L_SIZE 10
#define TOP_K_VALUE 10
#define LONGPQ_EXACT_TOPK 30
#if PARTIAL_RERANK_MODE == PARTIAL_RERANK_MODE_ENABLE
#define RERANK_PARTIAL_TOPN 20
#endif
#if CODE_MODE == CODE_MODE_DISKANN
#if ITER_LIMIT_MODE == ITER_LIMIT_MODE_OFF
#define MAX_EXPANDED_SIZE 1000 // need to be carefully tailored if we change the workload dataset
#else
#define MAX_EXPANDED_SIZE MAX_GREEDY_SEARCH_ITER
#endif
#endif
#endif
