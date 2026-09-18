#ifndef GRAPHSEARCH_PQ_CODEC_H_
#define GRAPHSEARCH_PQ_CODEC_H_

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
#include "query_ops.h"

extern uint32_t distance_table[PQ_VECTOR_DIM * 2][CENTROID_PER_SUBSPACE];
#if QPS_PROFILE_MODE == QPS_PROFILE_MODE_SHORT
extern int16_t codebook[PQ_VECTOR_DIM][CENTROID_PER_SUBSPACE][SUB_VECTOR_DIM];
extern int16_t codebook_long[PQ_VECTOR_DIM * 2][CENTROID_PER_SUBSPACE][SUB_VECTOR_DIM / 2];
#else
extern int16_t codebook[PQ_ACTIVE_DIM][CENTROID_PER_SUBSPACE][SUB_ACTIVE_DIM];
#endif
extern uint32_t base_graph_entry_vecID;
extern BREAKPOINT_TYPE breakpoint;
#endif /* GRAPHSEARCH_PQ_CODEC_H_ */
