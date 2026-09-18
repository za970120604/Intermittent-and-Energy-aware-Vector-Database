#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <stdlib.h>
#include "vectors_config.h"
#include "queue.h"
#include "checkpoint.h"
#include "graphsearch.h"
#include "my_timer.h"
#include "utils.h"
#include "debug.h"
#include "config.h"
#include "workload.h"
#include "graphsearch.h"
#include "checkpoint.h"
#include "workload.h"

#define tskTEST_PRIORITY (tskIDLE_PRIORITY + 1) // in FreeRTOS, larger priority number means higher task priority
#pragma NOINIT(workload_vector)
L2_Vector_Entry workload_vector;
#pragma NOINIT(workload_page)
uint8_t workload_page[PAGE_SIZE];
#pragma NOINIT(workload_vector_head)
L2_Vector_Entry* workload_vector_head;

#pragma PERSISTENT(checkpoint_countdown)
uint16_t checkpoint_countdown = CHECKPOINT_INTERVAL;
#pragma PERSISTENT(replay_head)
uint32_t replay_head = 0;

#pragma NOINIT(stat_timer_1_start)
uint32_t stat_timer_1_start;
#pragma NOINIT(stat_timer_1_end)
uint32_t stat_timer_1_end;
#pragma NOINIT(stat_timer_2_start)
uint32_t stat_timer_2_start;
#pragma NOINIT(stat_timer_2_end)
uint32_t stat_timer_2_end;

#pragma PERSISTENT(phase)
PHASE phase = PHASE_NONE;

#pragma PERSISTENT(query_restart_count)
uint32_t query_restart_count[REPLAY_COUNT] = {0};
#pragma NOINIT(last_read_q_page_idx)
uint32_t last_read_q_page_idx;

//#pragma PERSISTENT(recall_arr)
//double recall_arr[REPLAY_COUNT] = {0};
//#pragma PERSISTENT(recall_arr_head)
//uint32_t recall_arr_head = 0;

//#pragma PERSISTENT(replay_head_tailored)
//uint16_t replay_head_tailored[500] = {
//9134,     0,     9156,     4,     9162,     8,     9165,     23,     9167,     26,     9168,     27,     9173,     40,     9178,     41,
//9182,     51,     9208,     52,     9211,     56,     9213,     58,     9218,     60,     9232,     70,     9240,     74,     9251,     77,
//9252,     79,     9260,     81,     9267,     82,     9279,     92,     9289,     95,     9310,     96,     9312,     105,     9314,     117,
//9318,     120,     9326,     121,     9330,     122,     9331,     127,     9354,     135,     9355,     147,     9368,     150,     9398,     152,
//9406,     154,     9423,     155,     9435,     158,     9438,     169,     9442,     172,     9468,     173,     9470,     174,     9471,     179,
//9478,     181,     9479,     182,     9485,     183,     9487,     187,     9489,     192,     9497,     193,     9507,     197,     9511,     199,
//9513,     207,     9514,     210,     9520,     214,     9539,     216,     9556,     217,     9558,     218,     9563,     223,     9570,     225,
//9576,     235,     9582,     243,     9589,     249,     9594,     252,     9613,     255,     9629,     260,     9644,     262,     9646,     267,
//9650,     269,     9664,     271,     9665,     275,     9686,     276,     9688,     279,     9691,     283,     9715,     290,     9720,     293,
//9723,     296,     9726,     300,     9733,     302,     9734,     303,     9735,     305,     9738,     307,     9752,     309,     9754,     313,
//9778,     316,     9788,     318,     9806,     320,     9807,     322,     9808,     329,     9815,     331,     9821,     340,     9823,     341,
//9826,     343,     9840,     346,     9886,     348,     9892,     349,     9902,     350,     9913,     351,     9927,     357,     9937,     359,
//9950,     369,     9981,     375,     9985,     376,     9994,     377,     382,     383,     390,     391,     395,     398,     402,     403,
//405,     406,     407,     409,     410,     412,     413,     418,     420,     426,     430,     437,     441,     451,     458,     460,
//465,     466,     467,     469,     473,     488,     490,     492,     494,     505,     506,     507,     509,     517,     522,     529,
//538,     541,     542,     544,     547,     548,     549,     559,     563,     567,     572,     573,     576,     578,     580,     582,
//589,     594,     596,     598,     600,     609,     612,     615,     620,     644,     652,     655,     664,     665,     666,     669,
//675,     682,     687,     688,     690,     692,     693,     701,     710,     714,     720,     723,     726,     729,     736,     737,
//741,     744,     754,     763,     770,     771,     775,     778,     781,     782,     786,     788,     789,     801,     803,     810,
//818,     823,     829,     832,     833,     836,     840,     841,     844,     845,     850,     857,     858,     865,     868,     870,
//876,     877,     883,     884,     890,     894,     899,     901,     904,     907,     908,     909,     923,     924,     929,     934,
//935,     937,     940,     941,     946,     947,     948,     952,     956,     958,     963,     965,     967,     969,     982,     983,
//989,     999,     1002,     1007,     1014,     1016,     1019,     1020,     1022,     1025,     1027,     1034,     1048,     1053,     1061,     1063,
//1072,     1074,     1075,     1081,     1083,     1087,     1088,     1089,     1096,     1099,     1103,     1105,     1115,     1117,     1118,     1119,
//1120,     1122,     1131,     1137,     1140,     1142,     1145,     1146,     1157,     1158,     1159,     1167,     1168,     1171,     1175,     1181,
//1183,     1185,     1189,     1190,     1200,     1201,     1207,     1222,     1225,     1227,     1228,     1233,     1235,     1236,     1240,     1242,
//1243,     1249,     1254,     1257,     1263,     1266,     1270,     1273,     1277,     1280,     1282,     1288,     1300,     1302,     1311,     1314,
//1321,     1325,     1327,     1329,     1335,     1342,     1344,     1347,     1350,     1351,     1352,     1360,     1364,     1367,     1368,     1370,
//1373,     1384,     1391,     1392,     1394,     1396,     1401,     1402,     1407,     1413,     1415,     1416,     1432,     1433,     1442,     1444,
//1446,     1450,     1451,     1452,     1453,     1456,     1461,     1464,     1465,     1470,     1477,     1479,     1487,     1488,     1496,     1497,
//1498,     1503,     1515,     1516,     1519,     1520,     1521,     1523,     1524,     1529,     1532,     1535,     1540,     1543,     1549,     1560,
//1561,     1564,     1565,     1566
//};

extern BREAKPOINT_TYPE breakpoint;
extern uint32_t buffered_records_cnt;
extern Statistics stats;
extern uint32_t cycle_query_count;

#define QUERY_VECS_PER_PAGE (PAGE_SIZE / (FULL_VECTOR_DIM * sizeof(VECTOR_ELEMENT_TYPE)))

void graphsearch_test();
void main_graphsearch() {
    xTaskCreate(graphsearch_test, "test", 1500, NULL, tskTEST_PRIORITY, NULL);
    vTaskStartScheduler();
}

void graphsearch_test() {

#if (UCx_MODE == UCB_MODE_UCB1 && SPI_MODE == SPI_MODE_DMA) || \
    (OVERLAP_MODE == OVERLAP_MODE_ENABLE && \
     ((UCx_MODE != UCB_MODE_UCB0 && UCx_MODE != UCA_MODE_UCA3) || \
      SPI_MODE != SPI_MODE_DMA || \
      SEARCHLIST_MODE != SEARCHLIST_MODE_SORTED)) || \
    (RERANK_MODE == RERANK_MODE_CPU_LONGPQ && LONGPQ_EXACT_TOPK > MAX_L_SIZE) || \
    ((CODE_MODE == CODE_MODE_DISKANN) && \
     (SEARCHLIST_MODE != SEARCHLIST_MODE_SORTED || PRUNE_MODE != PRUNE_MODE_DISTANCE_ONLY))
    SET_BREAKPOINT(BP_MISCONFIGED);
#endif

#if (QPS_PROFILE_MODE == QPS_PROFILE_MODE_LONG) && \
    (SPI_MODE == SPI_MODE_BUSYPOLLING || \
     RERANK_MODE == RERANK_MODE_CPU_LONGPQ)
    SET_BREAKPOINT(BP_MISCONFIGED);
#endif

#if (CODE_MODE == CODE_MODE_DISKANN) && \
    (MAX_EXPANDED_SIZE < MAX_GREEDY_SEARCH_ITER)
    SET_BREAKPOINT(BP_MISCONFIGED);
#endif

#if (PARTIAL_RERANK_MODE == PARTIAL_RERANK_MODE_ENABLE) && \
    (RERANK_PARTIAL_TOPN < TOP_K_VALUE)
    SET_BREAKPOINT(BP_MISCONFIGED);
#endif

#if ((EXPERIMENT == EXP_BASELINE) || (EXPERIMENT == EXP_POWER_EVENT) || (EXPERIMENT == EXP_REPLAY)) && (QPS_PROFILE_MODE != QPS_PROFILE_MODE_SHORT)
    SET_BREAKPOINT(BP_MISCONFIGED);
#endif

#if ((EXPERIMENT == EXP_BASELINE) || (EXPERIMENT == EXP_POWER_EVENT) || (EXPERIMENT == EXP_REPLAY)) && \
    ((OVERLAP_MODE == OVERLAP_MODE_ENABLE) || (CODE_MODE == CODE_MODE_ORIGINAL))
    SET_BREAKPOINT(BP_MISCONFIGED);
#endif

//#if 1
//    CHECKPOINT(CHECKPOINT_INIT);
//
//    if (replay_head == 0) {
//        int _i;
//        for (_i = 0; _i < 7 - 1; ++_i) {
//            hw_setup_cum_ticks[_i] = 0;
//        }
//        hw_setup_sample_count = 0;
//    }
//    while (replay_head < REPLAY_COUNT) {
//        CHECKPOINT(CHECKPOINT_QUERY);
//        wait_for_energy();
//        ++replay_head;
//    }
//#else
    CHECKPOINT(CHECKPOINT_INIT);
    init_graphsearch();

#if WORKLOAD_DATA == DATA_SIFT
#ifdef STAT_TIME_MIN_MAX
    uint64_t tick_qvec_read = 0;   // query vector flash read
    uint64_t tick_query_total = 0; // per-query wall clock (LOW_RES)
#endif
#endif

    while (replay_head < REPLAY_COUNT) {
        phase = PHASE_READ_DATA;

#if SAMPLE_RATE > 0
       while (buffered_records_cnt == 0);
#endif

#if WORKLOAD_DATA == DATA_FAKE
    // Fake mode: query vector = base vector with identity values.
    // L2_Vector_Entry: 136 B (32-dim uint32_t) --> 15/page; pages count down from 65534.
    // NOTE: this section was calibrated for the uint32_t 32-dim fake data layout.

       uint32_t vector_idx = replay_head % 100;
       uint32_t page_idx = vector_idx / 15;
       uint32_t page_entry_idx = vector_idx % 15;
       uint32_t actual_page = 65534 - page_idx;

       if (page_entry_idx == 0 || replay_head == 0) {
           CHECKPOINT(CHECKPOINT_READ_DATA);
           memset(workload_page, 0, sizeof(workload_page));
           read_op(actual_page, 0, workload_page, PAGE_SIZE);
       }

       L2_Vector_Entry* current_entry = (L2_Vector_Entry*)workload_page + page_entry_idx;
       workload_vector.vector_id = current_entry->vector_id;
       for (uint32_t i = 0; i < FULL_VECTOR_DIM; ++i) {
           workload_vector.full_vector[i] = current_entry->full_vector[i];
           if (workload_vector.full_vector[i] != i) {
               SET_BREAKPOINT(BP_ERROR);
           }
       }
#elif WORKLOAD_DATA == DATA_SIFT

       uint32_t query_idx       = replay_head % QUERY_COUNT;
       // uint32_t query_idx       = replay_head_tailored[replay_head] % QUERY_COUNT;
       uint32_t q_page_idx      = query_idx / QUERY_VECS_PER_PAGE;
       uint32_t q_page_entry    = query_idx % QUERY_VECS_PER_PAGE;
       uint32_t q_actual_page   = 32599 - q_page_idx; // hardcode flash page number where query vectors starts to write on flash backward (ex: 32599 for id = 0, 1, 2 vectors; 32598 for id = 3, 4....)

       if (replay_head == 0 || q_page_idx != last_read_q_page_idx) {
           CHECKPOINT(CHECKPOINT_READ_DATA);
           memset(workload_page, 0, sizeof(workload_page));
#ifdef STAT_TIME_MIN_MAX
           uint32_t st_qvec_read = get_current_tick(LOW_RES_CLK);
#endif

#if (SPI_MODE == SPI_MODE_DMA)
           read_op_dma(q_actual_page, 0, (uint32_t)workload_page, PAGE_SIZE);
#else
           read_op(q_actual_page, 0, workload_page, PAGE_SIZE);
#endif

           last_read_q_page_idx = q_page_idx;
#ifdef STAT_TIME_MIN_MAX
           tick_qvec_read += (get_current_tick(LOW_RES_CLK) - st_qvec_read);
#endif
       }

       // Cast page buffer to VECTOR_ELEMENT_TYPE array; each query is FULL_VECTOR_DIM elements.
       VECTOR_ELEMENT_TYPE* qvec_base = (VECTOR_ELEMENT_TYPE*)workload_page + q_page_entry * FULL_VECTOR_DIM;
       workload_vector.vector_id = query_idx;
       for (uint32_t i = 0; i < FULL_VECTOR_DIM; ++i) {
           workload_vector.full_vector[i] = qvec_base[i];
//           workload_vector.full_vector[i]  = (VECTOR_ELEMENT_TYPE)((query_idx + i) % 256);
       }
#endif

#if WORKLOAD_DATA == DATA_SIFT
#ifdef STAT_TIME_MIN_MAX
       uint32_t st_query = get_current_tick(LOW_RES_CLK);
#endif
#endif

       QUERY(workload_vector.full_vector);
       ++cycle_query_count;

//#if WORKLOAD_DATA == DATA_SIFT
//       recall_arr[recall_arr_head++] = compute_recall(query_idx, search_result_distance_squared, TOP_K_VALUE);
//#endif

#if WORKLOAD_DATA == DATA_SIFT
#ifdef STAT_TIME_MIN_MAX
       uint32_t ed_query = get_current_tick(LOW_RES_CLK);
#endif
#endif

       query_restart_count[replay_head] = restart_count_this_query + 1;
       if (restart_count_this_query > 0) {
           ++distinct_miss_count;
           restart_count_this_query = 0;
       }
       else {
           uint32_t q_ticks = ed_query - st_query;
#ifdef STAT_TIME_MIN_MAX
           tick_query_total += q_ticks;
#endif
           ++distinct_completed_count;
       }
#if SAMPLE_RATE > 0
       --buffered_records_cnt;
#endif

       ++replay_head;
    }

#ifdef STAT_TIME_MIN_MAX
    bank_active_segment();
    stats.elapsed_time[PHASE_ALL_QUERY_PROCESSED] = (double)cumulative_active_ticks / (double)CS_getSMCLK();
#endif

#if EXPERIMENT == EXP_POWER_EVENT
    disable_power_event_timer();
#endif

    phase = PHASE_FINISHED;
    CHECKPOINT(CHECKPOINT_FINISHED);

#ifdef STAT_TIME_MIN_MAX
    // profiling clock frequency
    volatile double lowres_clk_freq = (double)CS_getSMCLK(); // 32768 Hz

    // per-hop average time
    volatile double avg_nbr_io_time   = (n_hops > 0) ? ((double)tick_nbr_io   / lowres_clk_freq) / (double)n_hops : 0.0;
    volatile double avg_prune_time    = (n_hops > 0) ? ((double)tick_prune    / lowres_clk_freq) / (double)n_hops : 0.0;
    volatile double avg_get_closest_time = (n_hops > 0) ? ((double)tick_get_closest / lowres_clk_freq) / (double)n_hops : 0.0;

#if SPI_DEBUG_MODE == SPI_DEBUG_MODE_ON
    // per-call average time
    volatile double avg_pqcode_io_time  = (n_addL_new > 0) ? ((double)(tick_pqcode_page_read_cmd + tick_pqcode_page_read_poll_for_oip + tick_pqcode_cache_read)  / lowres_clk_freq) / (double)n_addL_new : 0.0;
    volatile double avg_pqcode_cmp_time = (n_addL_new > 0) ? ((double)(tick_pqtable_lookup + tick_sorted_insert + tick_other_cmp) / lowres_clk_freq) / (double)n_addL_new : 0.0;
#endif
    volatile double avg_hc_new_time  = (n_addL_new > 0) ? ((double)tick_hc_new  / lowres_clk_freq) / (double)n_addL_new : 0.0;
    volatile double avg_hc_dup_time  = (n_addL_dup > 0) ? ((double)tick_hc_dup  / lowres_clk_freq) / (double)n_addL_dup : 0.0;

    // ratios
    volatile double ratio_searchlist_hc = (tick_searchlist_expand > 0) ? (double)(tick_hc_new + tick_hc_dup) / (double)tick_searchlist_expand : 0.0;
#if SPI_DEBUG_MODE == SPI_DEBUG_MODE_ON
    volatile double ratio_searchlist_io = (tick_searchlist_expand > 0) ? (double)(tick_nbr_io + tick_pqcode_page_read_cmd + tick_pqcode_page_read_poll_for_oip + tick_pqcode_cache_read) / (double)tick_searchlist_expand : 0.0;
#endif
    volatile double ratio_addL_new = (n_hops > 0) ? (double)n_addL_new / (double)n_hops : 0.0;
    volatile double ratio_addL_dup = (n_hops > 0) ? (double)n_addL_dup / (double)n_hops : 0.0;
    volatile double ratio_rerank_io  = (tick_rerank_io + tick_rerank_cmp + tick_rerank_sort > 0) ? (double)tick_rerank_io / (double)(tick_rerank_io + tick_rerank_cmp + tick_rerank_sort) : 0.0;
    volatile double ratio_rerank_cmp = (tick_rerank_io + tick_rerank_cmp + tick_rerank_sort > 0) ? (double)(tick_rerank_cmp + tick_rerank_sort) / (double)(tick_rerank_io + tick_rerank_cmp + tick_rerank_sort) : 0.0;

    // per-query average phase time
    volatile double avg_compute_table_time    = (n_query > 0) ? ((double)tick_compute_table    / lowres_clk_freq) / (double)n_query : 0.0;
    volatile double avg_searchlist_expand_time = (n_query > 0) ? ((double)tick_searchlist_expand/ lowres_clk_freq) / (double)n_query : 0.0;
    volatile double avg_rerank_answer_time    = (n_query > 0) ? ((double)tick_rerank_answer    / lowres_clk_freq) / (double)n_query : 0.0;

    // phase ratios
    volatile double total_query_ticks = (double)(tick_compute_table + tick_searchlist_expand + tick_rerank_answer);
    volatile double ratio_compute_table    = (total_query_ticks > 0) ? (double)tick_compute_table    / total_query_ticks : 0.0;
    volatile double ratio_searchlist_expand= (total_query_ticks > 0) ? (double)tick_searchlist_expand / total_query_ticks : 0.0;
    volatile double ratio_rerank_answer    = (total_query_ticks > 0) ? (double)tick_rerank_answer    / total_query_ticks : 0.0;

    // per-query average time
#if WORKLOAD_DATA == DATA_SIFT
    volatile double avg_qvec_read_time   = (n_query > 0) ? ((double)tick_qvec_read / lowres_clk_freq) / (double)n_query : 0.0;
    volatile double avg_query_total_time = (n_query > 0) ? ((double)tick_query_total / lowres_clk_freq) / (double)n_query : 0.0;
#endif
#endif

#if SPI_DEBUG_MODE == SPI_DEBUG_MODE_ON
    volatile uint64_t total_io_ticks_searchlist = tick_nbr_io + tick_pqcode_page_read_cmd + tick_pqcode_page_read_poll_for_oip + tick_pqcode_cache_read;
    volatile uint64_t total_io_ticks_rerank     = tick_rerank_io;
    volatile uint64_t total_io_ticks_all        = total_io_ticks_searchlist + total_io_ticks_rerank;
    volatile double io_share_searchlist = (total_io_ticks_all > 0) ? (double)total_io_ticks_searchlist / (double)total_io_ticks_all : 0.0;
    volatile double io_share_rerank     = (total_io_ticks_all > 0) ? (double)total_io_ticks_rerank     / (double)total_io_ticks_all : 0.0;
    volatile double ratio_io_overall = (total_query_ticks > 0) ? (double)total_io_ticks_all / total_query_ticks : 0.0;
#endif

// #endif

    P1OUT |= BIT0; // light up the LED

    SET_BREAKPOINT(BP_FINISHED); // 53sec/437.513mJ --> op_point0/100 queries; 63sec/550.664mJ --> op_point1/100 queries; 71sec/608.310mJ --> op_point2/100 queries;
                                 // 59sec/475.079mJ --> op_point3/100 queries; 69sec/564.305mJ --> op_point4/100 queries; 78sec/651.898mJ --> op_point5/100 queries;


                                // 55sec/457.202mJ --> op_point0/100 queries; 64sec/545.874mJ --> op_point1/100 queries; 74sec/637.007mJ --> op_point2/100 queries
                                // 65sec/542.601mJ --> op_point3/100 queries, 77sec/644.823mJ --> op_point4/100 queries, 87sec/744.732mJ --> op_point5/100 queries

                                // 152, 779.642mJ, 250ms;
    for (;;);
}
