#include "pq_codec.h"
#pragma NOINIT(distance_table)
uint32_t distance_table[PQ_VECTOR_DIM * 2][CENTROID_PER_SUBSPACE];
#if QPS_PROFILE_MODE == QPS_PROFILE_MODE_SHORT
#pragma NOINIT(codebook)
int16_t codebook[PQ_VECTOR_DIM][CENTROID_PER_SUBSPACE][SUB_VECTOR_DIM];
#pragma NOINIT(codebook_long)
int16_t codebook_long[PQ_VECTOR_DIM * 2][CENTROID_PER_SUBSPACE][SUB_VECTOR_DIM / 2];
#else
#pragma NOINIT(codebook)
int16_t codebook[PQ_ACTIVE_DIM][CENTROID_PER_SUBSPACE][SUB_ACTIVE_DIM];
#endif
