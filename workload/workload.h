#ifndef WORKLOAD_H
#define WORKLOAD_H

#define DATA_FAKE 0
#define DATA_SIFT 1
#define WORKLOAD_DATA DATA_SIFT

#if WORKLOAD_DATA == DATA_FAKE
#define EXPECTED_VECTOR_COUNT 100
#define QUERY_COUNT 100
#elif WORKLOAD_DATA == DATA_SIFT
#define EXPECTED_VECTOR_COUNT 200000 // 200K sample data from SIFT1M
#define QUERY_COUNT 10000
#endif
#define FLASH_DATA_START_PAGE 65534

typedef enum {
    SDCARD_FILETYPE_FULLVEC = 0,
    SDCARD_FILETYPE_NEIGHBOR = 1
} SDCARD_FILETYPE;

void setup_sample_rate_timer();
void main_workload();

extern uint32_t codebook_write_page_start;
extern uint32_t mean_write_page_start;
extern uint32_t fullvec_write_page_start;
extern uint32_t neighbor_write_page_start;
extern uint32_t L1_write_page_start;
extern uint32_t query_write_page_start;
extern uint32_t codebook_long_write_page_start;
extern uint32_t groundtruth_write_page_start;

extern BREAKPOINT_TYPE breakpoint;
#endif /* WORKLOAD_H */
