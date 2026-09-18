//#ifndef GRAPHSEARCH_MERGE_OPS_H_
//#define GRAPHSEARCH_MERGE_OPS_H_
//
//#include "FreeRTOS.h"
//#include <stdlib.h>
//#include <string.h>
//#include <stddef.h>
//#include <stdint.h>
//#include <math.h>
//#include "utils.h"
//#include "my_timer.h"
//#include "spi_nand.h"
//#include "checkpoint.h"
//#include "debug.h"
//#include "query_ops.h"
//#include "config.h"
//#include "adc.h"
//#include <stdio.h>
//#include <vectors_config.h>
//#include "debug.h"
//#include "mapping.h"
//#include "insert_ops.h"
//
//
//uint32_t calculate_delta_vector_batch_capacity(uint32_t start_vector_id,
//                                                      uint32_t end_vector_id,
//                                                      uint32_t max_l2_entries,
//                                                      uint32_t max_l1_groups);
//void collect_L2_for_delta_vectors(uint32_t start_vector_id,
//                                   uint32_t batch_size,
//                                   uint8_t* pq_codes);
//void fill_L1_for_delta_vectors(uint32_t start_vector_id,
//                                uint32_t batch_size,
//                                address l2_flush_start,
//                                uint32_t l2_entry_pair_size,
//                                uint8_t* pq_codes);
//
//extern uint32_t base_graph_entry_vecID;
//extern uint32_t delta_log_entry_vecID;
//extern uint32_t delta_graph_entry_vecID;
//extern uint32_t delta_graph_entry_vecIDs[MAX_DELTA_ENTRIES];
//extern uint32_t delta_graph_start_vector_id;
//extern uint32_t delta_vector_count;
//extern uint32_t search_result_ids[TOP_K_VALUE];
//
//extern uint32_t current_max_vector_id;
//extern BREAKPOINT_TYPE breakpoint;
//#endif /* GRAPHSEARCH_MERGE_OPS_H_ */
