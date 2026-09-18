#ifndef GRAPHSEARCH_GRAPHSEARCH_H_
#define GRAPHSEARCH_GRAPHSEARCH_H_
#include "vectors_config.h"
#include "checkpoint.h"
#include "mapping.h"
#include "query_ops.h"
#include "insert_ops.h"
#include "merge_ops.h"
#include "pq_codec.h"
#include "delta_store.h"
#include "delete_ops.h"
#include "workload.h"

/* real functions - algorithm flow*/
void init_graphsearch();
void query(const VECTOR_ELEMENT_TYPE *const query_vector, uint32_t entry_vector_id);

/* Wrappers */
inline void QUERY(const VECTOR_ELEMENT_TYPE* const query_vector);
//inline void DELETE_VECTOR(uint32_t deleted_vector_id);
//void INSERT_VECTOR(const VECTOR_ELEMENT_TYPE* const inserted_vector, uint32_t timestamp_id);
//inline void MERGE();
//inline void GC();

extern uint32_t base_graph_entry_vecID;
extern int16_t pq_global_mean[FULL_VECTOR_DIM];
#endif /* GRAPHSEARCH_GRAPHSEARCH_H_ */
