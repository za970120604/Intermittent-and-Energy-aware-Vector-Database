//#ifndef GRAPHSEARCH_INSERT_OPS_H_
//#define GRAPHSEARCH_INSERT_OPS_H_
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
//#include "config.h"
//#include "adc.h"
//#include <stdio.h>
//#include <vectors_config.h>
//#include "debug.h"
//#include "mapping.h"
//#include "query_ops.h"
//#include "pq_codec.h"
//#include "graphsearch.h"
//
//#define INSERT_LOG_HASH_SIZE 128
//typedef struct InsertLogHashEntryDescriptor {
//    struct InsertLogHashEntryDescriptor *prev;
//    uint32_t vector_id;
//    uintptr_t addr;
//} InsertLogHashEntryDescriptor;
//
//#define MAX_NEW_NEIGHBORS_PER_NODE (INSERT_BUFFER_SIZE / (sizeof(InsertLogHashEntryDescriptor) + sizeof(VECTOR_ELEMENT_TYPE) * FULL_VECTOR_DIM))
//#define MAX_PENDING_NODE_UPDATES 512
//
//typedef struct {
//    uint32_t vector_id;
//    uint32_t new_neighbors[MAX_NEW_NEIGHBORS_PER_NODE];
//    uint8_t new_neighbor_count;
//} PendingNodeUpdate;
//
//
//uint32_t apply_insert_changes(void);
//void add_pending_reverse_edge(uint32_t vector_id, uint32_t new_neighbor_id);
//uint32_t calculate_insertlog_batch_count(uint8_t* start_iter, uint8_t* end_iter, uint32_t max_l2_entries, uint32_t max_l1_groups);
//uint32_t calculate_reverse_edge_batch_count(uint32_t start_index, uint32_t total_count, uint32_t max_l2_neighbors, uint32_t max_l1_groups);
//
//uint8_t* collect_L2_for_new_vectors(uint8_t* batch_start,
//                                     uint32_t batch_size,
//                                     uint8_t* pq_codes);
//void fill_L1_for_new_vectors(uint8_t* batch_start,
//                              uint32_t batch_size,
//                              address l2_flush_start,
//                              uint32_t l2_entry_pair_size,
//                              uint8_t* pq_codes);
//void collect_L2_for_reverse_edges(uint32_t start_index, uint32_t batch_size);
//void fill_L1_for_reverse_edges(uint32_t start_index,
//                                uint32_t batch_size,
//                                address l2_flush_start,
//                                uint8_t* pq_codes);
//
//extern uint8_t insert_log[INSERT_BUFFER_SIZE];
//extern uint8_t *insert_log_head;
//extern InsertLogHashEntryDescriptor *insert_log_hash[INSERT_LOG_HASH_SIZE];
//extern PendingNodeUpdate pending_node_updates[MAX_PENDING_NODE_UPDATES];
//extern uint8_t* pending_node_update_head;
//
//extern uint32_t base_graph_entry_vecID;
//extern uint32_t delta_log_entry_vecID;
//extern uint32_t delta_graph_entry_vecID;
//extern uint32_t delta_graph_entry_vecIDs[MAX_DELTA_ENTRIES];
//extern uint32_t delta_graph_start_vector_id;
//extern uint32_t delta_vector_count;
//extern BREAKPOINT_TYPE breakpoint;
//
//extern uint32_t replay_head;
//#endif /* GRAPHSEARCH_INSERT_OPS_H_ */
