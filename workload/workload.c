#include "msp430.h"
#include "driverlib.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "config.h"
#include "debug.h"
#include "spi_nand.h"
#include "../HAL/HAL_SDCard.h"
#include "../SDCardLib/sdcard.h"
#include "vectors_config.h"
#include "workload.h"
#include "mapping.h"

#define ENTRIES_PER_PAGE(entry_type) (PAGE_SIZE / sizeof(entry_type))
#define CALC_ENTRY_PAGE(write_start, entry_idx, entry_type) \
    ((write_start) - ((entry_idx) / ENTRIES_PER_PAGE(entry_type)))
#define CALC_ENTRY_OFFSET(entry_idx, entry_type) \
    (((entry_idx) % ENTRIES_PER_PAGE(entry_type)) * sizeof(entry_type))

#pragma PERSISTENT(buffered_records_cnt)
uint32_t buffered_records_cnt = 0;

// EXP_WORKLOAD: workload.c streams CODEBOOK.BIN and MEAN.BIN from SD card to NAND flash in workload.c;
// EXP_TEST/EXP_POWER_EVENT: init_graphsearch() reads them back from flash into these FRAM arrays on every boot.
#if QPS_PROFILE_MODE == QPS_PROFILE_MODE_SHORT
extern int16_t codebook[PQ_VECTOR_DIM][CENTROID_PER_SUBSPACE][SUB_VECTOR_DIM];
#else
extern int16_t codebook[PQ_ACTIVE_DIM][CENTROID_PER_SUBSPACE][SUB_ACTIVE_DIM];
#endif
extern int16_t pq_global_mean[FULL_VECTOR_DIM];

#if EXPERIMENT == EXP_GEN_WORKLOAD
#pragma PERSISTENT(buf_filename)
char buf_filename[20] = {0};

#pragma PERSISTENT(buf_flash_flush)
uint8_t buf_flash_flush[PAGE_SIZE] = {0};
#pragma PERSISTENT(buf_flash_flush_head)
uint8_t* buf_flash_flush_head = buf_flash_flush;

#pragma NOINIT(codebook_write_page_start)
uint32_t codebook_write_page_start; // 65534 in sift1M_200Ksample/PCA64/int16_t vector element
#pragma NOINIT(mean_write_page_start)
uint32_t mean_write_page_start; // 65518 in sift1M_200Ksample/PCA64/int16_t vector element
#pragma NOINIT(fullvec_write_page_start)
uint32_t fullvec_write_page_start; // 65517 in sift1M_200Ksample/PCA64/int16_t vector element
#pragma NOINIT(neighbor_write_page_start)
uint32_t neighbor_write_page_start; // 52183 in sift1M_200Ksample/PCA64/int16_t vector element
#pragma NOINIT(L1_write_page_start)
uint32_t L1_write_page_start; // 38849 in sift1M_200Ksample/PCA64/int16_t vector element; L1_write_page_end == 32600
#pragma NOINIT(query_write_page_start)
uint32_t query_write_page_start; // 32599 in sift1M_200Ksample/PCA64/int16_t vector element
#pragma NOINIT(codebook_long_write_page_start)
uint32_t codebook_long_write_page_start; // 31974 in sift1M_200Ksample/PCA64/int16_t vector element
#pragma NOINIT(groundtruth_write_page_start)
uint32_t groundtruth_write_page_start; // 31958 in sift1M_200Ksample/PCA64/int16_t vector element

#pragma PERSISTENT(l1_inner_check_failed_vec_idx)
uint32_t l1_inner_check_failed_vec_idx = 0xFFFFFFFF;

SDCardLib sdCard;

void main_workload() {
    FRESULT fresult;
    FIL file;
    uint32_t bytesRead = 0;
    volatile uint32_t cur_nand_write_page = FLASH_DATA_START_PAGE;

    // detect and initialize SD card module + erase all blocks in SPI Nand flash
    if (SDCard_detectCard() != SDCARDLIB_STATUS_PRESENT) {
        SET_BREAKPOINT(BP_ERROR);
    }
    SDCard_init();
    SDCardLib_init(&sdCard, &sdIntf_MSP430FR5994LP);

    for (uint32_t block = 0 ; block < BLOCKS_PER_PLANE; ++block) {
        erase_op(block << 6);
    }

#if WORKLOAD_DATA == DATA_SIFT
    /* write codebook to flash */
    codebook_write_page_start = cur_nand_write_page;

    memcpy(buf_filename, "CODEBOOK.BIN", 12 + 1);
    fresult = f_open(&file, buf_filename, FA_READ);
    if (fresult != FR_OK) {
        SET_BREAKPOINT(BP_ERROR);
    }
    int16_t buf_cb_elem;
    uint32_t cb_total_elems = (uint32_t)PQ_VECTOR_DIM * CENTROID_PER_SUBSPACE * SUB_VECTOR_DIM;
    for (uint32_t idx = 0; idx < cb_total_elems; ++idx) {
        fresult = f_read(&file, (uint8_t*)&buf_cb_elem, sizeof(int16_t), &bytesRead); // codebook elements are stored as type int16_t
        if (fresult != FR_OK || bytesRead != sizeof(int16_t)) {
            SET_BREAKPOINT(BP_ERROR);
        }

        memcpy(buf_flash_flush_head, &buf_cb_elem, sizeof(int16_t));
        buf_flash_flush_head += sizeof(int16_t);

        if ((uintptr_t)(buf_flash_flush_head + sizeof(int16_t)) > (uintptr_t)&buf_flash_flush[PAGE_SIZE]) {
            write_op(cur_nand_write_page, 0, buf_flash_flush, PAGE_SIZE);
            --cur_nand_write_page;
            memset(buf_flash_flush, 0, PAGE_SIZE);
            buf_flash_flush_head = buf_flash_flush;
        }
    }
    f_close(&file);
    if (buf_flash_flush_head != buf_flash_flush) {
        write_op(cur_nand_write_page, 0, buf_flash_flush, PAGE_SIZE);
        --cur_nand_write_page;
        memset(buf_flash_flush, 0, PAGE_SIZE);
        buf_flash_flush_head = buf_flash_flush;
    }

    /* write mean vector to flash */
    mean_write_page_start = cur_nand_write_page;

    memcpy(buf_filename, "MEAN.BIN", 8 + 1);
    fresult = f_open(&file, buf_filename, FA_READ);
    if (fresult != FR_OK) {
        SET_BREAKPOINT(BP_ERROR);
    }
    int16_t buf_mean_elem;
    for (uint32_t dim = 0; dim < FULL_VECTOR_DIM; ++dim) {
        fresult = f_read(&file, (uint8_t*)&buf_mean_elem, sizeof(int16_t), &bytesRead); // mean vector elements are stored as type int16_t
        if (fresult != FR_OK || bytesRead != sizeof(int16_t)) {
            SET_BREAKPOINT(BP_ERROR);
        }

        memcpy(buf_flash_flush_head, &buf_mean_elem, sizeof(int16_t));
        buf_flash_flush_head += sizeof(int16_t);

        if ((uintptr_t)(buf_flash_flush_head + sizeof(int16_t)) > (uintptr_t)&buf_flash_flush[PAGE_SIZE]) {
            write_op(cur_nand_write_page, 0, buf_flash_flush, PAGE_SIZE);
            --cur_nand_write_page;
            memset(buf_flash_flush, 0, PAGE_SIZE);
            buf_flash_flush_head = buf_flash_flush;
        }
    }
    f_close(&file);
    if (buf_flash_flush_head != buf_flash_flush) {
        write_op(cur_nand_write_page, 0, buf_flash_flush, PAGE_SIZE);
        --cur_nand_write_page;
        memset(buf_flash_flush, 0, PAGE_SIZE);
        buf_flash_flush_head = buf_flash_flush;
    }
#endif

    /* write full vector to flash */
    fullvec_write_page_start = cur_nand_write_page;

    memcpy(buf_filename, "VEC.BIN", 7 + 1);
    fresult = f_open(&file, buf_filename, FA_READ);
    if (fresult != FR_OK) {
        SET_BREAKPOINT(BP_ERROR);
    }
    volatile uint32_t raw_vec_count = 0;
    VECTOR_ELEMENT_TYPE buf_fullvec[FULL_VECTOR_DIM];
    while (raw_vec_count < EXPECTED_VECTOR_COUNT) {
        fresult = f_read(&file, (uint8_t*)buf_fullvec, sizeof(buf_fullvec), &bytesRead);
        if (fresult != FR_OK) {
           SET_BREAKPOINT(BP_ERROR);
        }

        if (bytesRead == 0) { // EOF
            break;
        }

        if(bytesRead != sizeof(VECTOR_ELEMENT_TYPE) * FULL_VECTOR_DIM) {
           SET_BREAKPOINT(BP_ERROR);
        }

        L2_Vector_Entry temp_vec_entry;
        temp_vec_entry.vector_id = raw_vec_count;
        temp_vec_entry.entry_info.type = ENTRY_TYPE_L2_VECTOR;
        for(uint32_t dim = 0; dim < FULL_VECTOR_DIM; ++dim) {

#if WORKLOAD_DATA == DATA_FAKE
            if (buf_fullvec[dim] != dim) {
                SET_BREAKPOINT(BP_ERROR);
            }
#endif
            temp_vec_entry.full_vector[dim] = buf_fullvec[dim];
        }
        memcpy(buf_flash_flush_head, &temp_vec_entry, sizeof(L2_Vector_Entry));
        buf_flash_flush_head += sizeof(L2_Vector_Entry);
        ++raw_vec_count;

        if ((uintptr_t)(buf_flash_flush_head + sizeof(L2_Vector_Entry)) > (uintptr_t)&buf_flash_flush[PAGE_SIZE]) {
            write_op(cur_nand_write_page, 0, buf_flash_flush, PAGE_SIZE);
            --cur_nand_write_page;
            memset(buf_flash_flush, 0, PAGE_SIZE);
            buf_flash_flush_head = buf_flash_flush;
        }
    }
    f_close(&file);
    if (buf_flash_flush_head != buf_flash_flush) {
        write_op(cur_nand_write_page, 0, buf_flash_flush, PAGE_SIZE);
        --cur_nand_write_page;
        memset(buf_flash_flush, 0, PAGE_SIZE);
        buf_flash_flush_head = buf_flash_flush;
    }

    /* write neighbor list to flash */
    neighbor_write_page_start = cur_nand_write_page;

    memcpy(buf_filename, "NEIGHBOR.BIN", 12 + 1);
    fresult = f_open(&file, buf_filename, FA_READ);
    if (fresult != FR_OK) {
        SET_BREAKPOINT(BP_ERROR);
    }
    raw_vec_count = 0;
    uint32_t buf_neighborlist[MAX_OUT_NEIGHBORS]; // neighbor indexes are stored as type uint32_t
    while (raw_vec_count < EXPECTED_VECTOR_COUNT) {
        fresult = f_read(&file, (uint8_t*)buf_neighborlist, sizeof(buf_neighborlist), &bytesRead);
        if (fresult != FR_OK) {
           SET_BREAKPOINT(BP_ERROR);
        }

        if (bytesRead == 0) { // EOF
            break;
        }

        if(bytesRead != sizeof(uint32_t) * MAX_OUT_NEIGHBORS) {
           SET_BREAKPOINT(BP_ERROR);
        }

        L2_Neighbor_Entry temp_neighbor_entry;
        temp_neighbor_entry.vector_id = raw_vec_count;
        temp_neighbor_entry.entry_info.type = ENTRY_TYPE_L2_NEIGHBOR;
        for(uint32_t neigh = 0; neigh < MAX_OUT_NEIGHBORS; ++neigh) {
#if WORKLOAD_DATA == DATA_FAKE
            if (buf_neighborlist[neigh] != ((raw_vec_count + neigh + 1) % EXPECTED_VECTOR_COUNT)) {
                SET_BREAKPOINT(BP_ERROR);
            }
#endif
            temp_neighbor_entry.neighbor_ids[neigh] = buf_neighborlist[neigh];
        }
        memcpy(buf_flash_flush_head, &temp_neighbor_entry, sizeof(L2_Neighbor_Entry));
        buf_flash_flush_head += sizeof(L2_Neighbor_Entry);
        ++raw_vec_count;

        if ((uintptr_t)(buf_flash_flush_head + sizeof(L2_Neighbor_Entry)) > (uintptr_t)&buf_flash_flush[PAGE_SIZE]) {
            write_op(cur_nand_write_page, 0, buf_flash_flush, PAGE_SIZE);
            --cur_nand_write_page;
            memset(buf_flash_flush, 0, PAGE_SIZE);
            buf_flash_flush_head = buf_flash_flush;
        }
    }
    f_close(&file);
    if (buf_flash_flush_head != buf_flash_flush) {
        write_op(cur_nand_write_page, 0, buf_flash_flush, PAGE_SIZE);
        --cur_nand_write_page;
        memset(buf_flash_flush, 0, PAGE_SIZE);
        buf_flash_flush_head = buf_flash_flush;
    }

    /* write L1 mapping entries to flash */
    L1_write_page_start = cur_nand_write_page;

    uint32_t cur_group_id = 0;

#if WORKLOAD_DATA == DATA_SIFT
    FIL pqcode_file;
    uint8_t buf_pqcode[PQ_VECTOR_DIM]; // a single PQ code are stored as type uint8_t
    memcpy(buf_filename, "PQCODE.BIN", 10 + 1);
    fresult = f_open(&pqcode_file, buf_filename, FA_READ);
    if (fresult != FR_OK) {
        SET_BREAKPOINT(BP_ERROR);
    }

    FIL pqcode_file_long;
    uint8_t buf_pqcode_long[PQ_VECTOR_DIM * 2]; // a single PQ code are stored as type uint8_t
    memcpy(buf_filename, "LONG.BIN", 8 + 1);
    fresult = f_open(&pqcode_file_long, buf_filename, FA_READ);
    if (fresult != FR_OK) {
        SET_BREAKPOINT(BP_ERROR);
    }
#endif
    volatile uint32_t vec_idx_ = 0;
    for (vec_idx_ = 0; vec_idx_ < EXPECTED_VECTOR_COUNT; ++vec_idx_) {
        L1_Mapping_Entry temp_map_entry;
        memset(&temp_map_entry, 0, sizeof(L1_Mapping_Entry));
        temp_map_entry.vector_id = vec_idx_;
        temp_map_entry.entry_info.type = ENTRY_TYPE_L1_MAPPING;

        // compute full vector address
        volatile uint32_t fullvec_actual_page = CALC_ENTRY_PAGE(fullvec_write_page_start, vec_idx_, L2_Vector_Entry);
        temp_map_entry.full_vector_address.block  = fullvec_actual_page / PAGES_PER_BLOCK;
        temp_map_entry.full_vector_address.page   = fullvec_actual_page % PAGES_PER_BLOCK;
        temp_map_entry.full_vector_address.offset = CALC_ENTRY_OFFSET(vec_idx_, L2_Vector_Entry);
        temp_map_entry.full_vector_address.type   = ENTRY_TYPE_L2_VECTOR;

        // compute neighbor list address
        volatile uint32_t neighbor_actual_page = CALC_ENTRY_PAGE(neighbor_write_page_start, vec_idx_, L2_Neighbor_Entry);
        temp_map_entry.neighbor_list_address.block  = neighbor_actual_page / PAGES_PER_BLOCK;
        temp_map_entry.neighbor_list_address.page   = neighbor_actual_page % PAGES_PER_BLOCK;
        temp_map_entry.neighbor_list_address.offset = CALC_ENTRY_OFFSET(vec_idx_, L2_Neighbor_Entry);
        temp_map_entry.neighbor_list_address.type   = ENTRY_TYPE_L2_NEIGHBOR;

        // write PQ codes
#if WORKLOAD_DATA == DATA_FAKE
        for(uint32_t i = 0; i < PQ_VECTOR_DIM; ++i) {
            temp_map_entry.pq_codes[i] = i;
        }
#elif WORKLOAD_DATA == DATA_SIFT
        fresult = f_read(&pqcode_file, buf_pqcode, PQ_VECTOR_DIM, &bytesRead);
        if (fresult != FR_OK || bytesRead != PQ_VECTOR_DIM * sizeof(uint8_t)) {
            SET_BREAKPOINT(BP_ERROR);
        }
        for(uint32_t i = 0; i < PQ_VECTOR_DIM; ++i) {
            temp_map_entry.pq_codes[i] = buf_pqcode[i];
        }

        fresult = f_read(&pqcode_file_long, buf_pqcode_long, PQ_VECTOR_DIM * 2, &bytesRead);
        if (fresult != FR_OK || bytesRead != PQ_VECTOR_DIM * 2 * sizeof(uint8_t)) {
            SET_BREAKPOINT(BP_ERROR);
        }
        for(uint32_t i = 0; i < PQ_VECTOR_DIM * 2; ++i) {
            temp_map_entry.pq_codes_long[i] = buf_pqcode_long[i];
        }
#endif

        memcpy(buf_flash_flush_head, &temp_map_entry, sizeof(L1_Mapping_Entry));
        buf_flash_flush_head += sizeof(L1_Mapping_Entry);

        if ((uintptr_t)(buf_flash_flush_head + sizeof(L1_Mapping_Entry)) > (uintptr_t)&buf_flash_flush[PAGE_SIZE]) {
            if ((vec_idx_ / L1_ENTRY_COUNT) != cur_group_id || (vec_idx_ % L1_ENTRY_COUNT) != (L1_ENTRY_COUNT-1)) { // assume flash page size is a multiple of L1 Mapping Entry
                l1_inner_check_failed_vec_idx = vec_idx_;
                SET_BREAKPOINT(BP_ERROR);
            }
            ++cur_group_id;
            write_op(cur_nand_write_page, 0, buf_flash_flush, PAGE_SIZE);
            --cur_nand_write_page;
            memset(buf_flash_flush, 0, PAGE_SIZE);
            buf_flash_flush_head = buf_flash_flush;
        }
    }
    volatile uint32_t pre_trail = (uint32_t)(buf_flash_flush_head - buf_flash_flush);
    volatile uint32_t trail_page = cur_nand_write_page;
    volatile int trail_write_ret = -1;
    if (buf_flash_flush_head != buf_flash_flush) {
        trail_write_ret = write_op(cur_nand_write_page, 0, buf_flash_flush, PAGE_SIZE);
        --cur_nand_write_page;
        memset(buf_flash_flush, 0, PAGE_SIZE);
        buf_flash_flush_head = buf_flash_flush;
    }
//    volatile uint32_t post_trail_page = cur_nand_write_page;

//    volatile uint32_t sanity_vid = 0xDEADBEEF;
//    volatile uint32_t sanity_page = CALC_ENTRY_PAGE(L1_write_page_start, 199999, L1_Mapping_Entry);
//    volatile uint32_t sanity_off  = CALC_ENTRY_OFFSET(199999, L1_Mapping_Entry);
//    read_op_dma(sanity_page, sanity_off, (uint8_t*)&sanity_vid, 4);
//    if (sanity_vid != 199999UL) {
//        SET_BREAKPOINT(BP_ERROR);
//    }

    volatile uint32_t L1_write_page_end = cur_nand_write_page + 1;
#if WORKLOAD_DATA == DATA_SIFT
    f_close(&pqcode_file);
    f_close(&pqcode_file_long);
#endif

//    /* End-to-End verification all on-flash data */
//    volatile L1_Mapping_Entry l1_entry;
//    volatile L2_Neighbor_Entry l2_neighbor;
//    volatile L2_Vector_Entry l2_vec;
//    volatile uint32_t vec_idx = 0;
//    for(vec_idx = 0; vec_idx < EXPECTED_VECTOR_COUNT; ++vec_idx) {
//        volatile uint32_t l1_page = CALC_ENTRY_PAGE(L1_write_page_start, vec_idx, L1_Mapping_Entry);
//        volatile uint32_t l1_offset_in_page = CALC_ENTRY_OFFSET(vec_idx, L1_Mapping_Entry);
//        // verify L1 Mapping Entry
//        read_op(l1_page, l1_offset_in_page, (uint8_t*)&l1_entry, sizeof(L1_Mapping_Entry));
//        if(l1_entry.vector_id != vec_idx || l1_entry.entry_info.type != ENTRY_TYPE_L1_MAPPING) {
//            SET_BREAKPOINT(BP_ERROR);
//        }
//#if WORKLOAD_DATA == DATA_FAKE
//        for(uint8_t dim = 0; dim < PQ_VECTOR_DIM; ++dim) {
//            if (l1_entry.pq_codes[dim] != dim) {
//                SET_BREAKPOINT(BP_ERROR);
//            }
//        }
//#endif
//        // verify full vector mapping
//        volatile uint32_t fullvec_page = CALC_ENTRY_PAGE(fullvec_write_page_start, vec_idx, L2_Vector_Entry);
//        volatile uint32_t fullvec_offset_in_page = CALC_ENTRY_OFFSET(vec_idx, L2_Vector_Entry);
//        read_op(fullvec_page, fullvec_offset_in_page, (uint8_t*)&l2_vec, sizeof(L2_Vector_Entry));
//        if(l2_vec.vector_id != vec_idx || l2_vec.entry_info.type != ENTRY_TYPE_L2_VECTOR) {
//            SET_BREAKPOINT(BP_ERROR);
//        }
//#if WORKLOAD_DATA == DATA_FAKE
//        for(uint8_t dim = 0; dim < FULL_VECTOR_DIM; ++dim) {
//            if (l2_vec.full_vector[dim] != dim) {
//                SET_BREAKPOINT(BP_ERROR);
//            }
//        }
//#endif
//        volatile uint32_t expected_fullvec_block = fullvec_page / PAGES_PER_BLOCK;
//        volatile uint32_t expected_fullvec_page_in_block = fullvec_page % PAGES_PER_BLOCK;
//        if(l1_entry.full_vector_address.block  != expected_fullvec_block ||
//           l1_entry.full_vector_address.page   != expected_fullvec_page_in_block ||
//           l1_entry.full_vector_address.offset != fullvec_offset_in_page ||
//           l1_entry.full_vector_address.type   != ENTRY_TYPE_L2_VECTOR) {
//            SET_BREAKPOINT(BP_ERROR);
//        }
//        // verify neighbor list mapping
//        volatile uint32_t neighbor_page = CALC_ENTRY_PAGE(neighbor_write_page_start, vec_idx, L2_Neighbor_Entry);
//        volatile uint32_t neighbor_offset_in_page = CALC_ENTRY_OFFSET(vec_idx, L2_Neighbor_Entry);
//        read_op(neighbor_page, neighbor_offset_in_page, (uint8_t*)&l2_neighbor, sizeof(L2_Neighbor_Entry));
//        if(l2_neighbor.vector_id != vec_idx || l2_neighbor.entry_info.type != ENTRY_TYPE_L2_NEIGHBOR) {
//            SET_BREAKPOINT(BP_ERROR);
//        }
//#if WORKLOAD_DATA == DATA_FAKE
//        for(uint8_t neigh = 0; neigh < MAX_OUT_NEIGHBORS; ++neigh) {
//            if (l2_neighbor.neighbor_ids[neigh] != ((vec_idx + neigh + 1) % EXPECTED_VECTOR_COUNT)) {
//                SET_BREAKPOINT(BP_ERROR);
//            }
//        }
//#endif
//        volatile uint32_t expected_neighbor_block = neighbor_page / PAGES_PER_BLOCK;
//        volatile uint32_t expected_neighbor_page_in_block = neighbor_page % PAGES_PER_BLOCK;
//        if(l1_entry.neighbor_list_address.block  != expected_neighbor_block ||
//           l1_entry.neighbor_list_address.page   != expected_neighbor_page_in_block ||
//           l1_entry.neighbor_list_address.offset != neighbor_offset_in_page ||
//           l1_entry.neighbor_list_address.type   != ENTRY_TYPE_L2_NEIGHBOR) {
//            SET_BREAKPOINT(BP_ERROR);
//        }
//    }

#if WORKLOAD_DATA == DATA_SIFT
    /* write query to flash */
    query_write_page_start = cur_nand_write_page;

    memcpy(buf_filename, "QUERY.BIN", 9 + 1);
    fresult = f_open(&file, buf_filename, FA_READ);
    if (fresult != FR_OK) {
        SET_BREAKPOINT(BP_ERROR);
    }
    VECTOR_ELEMENT_TYPE buf_qvec_elem;
    uint32_t total_query_elems = (uint32_t)QUERY_COUNT * FULL_VECTOR_DIM;
    for (uint32_t idx = 0; idx < total_query_elems; ++idx) {
        fresult = f_read(&file, (uint8_t*)&buf_qvec_elem, sizeof(VECTOR_ELEMENT_TYPE), &bytesRead);
        if (fresult != FR_OK || bytesRead != sizeof(VECTOR_ELEMENT_TYPE)) {
            SET_BREAKPOINT(BP_ERROR);
        }

        memcpy(buf_flash_flush_head, &buf_qvec_elem, sizeof(VECTOR_ELEMENT_TYPE));
        buf_flash_flush_head += sizeof(VECTOR_ELEMENT_TYPE);

        if ((uintptr_t)(buf_flash_flush_head + sizeof(VECTOR_ELEMENT_TYPE)) > (uintptr_t)&buf_flash_flush[PAGE_SIZE]) {
            write_op(cur_nand_write_page, 0, buf_flash_flush, PAGE_SIZE);
            --cur_nand_write_page;
            memset(buf_flash_flush, 0, PAGE_SIZE);
            buf_flash_flush_head = buf_flash_flush;
        }
    }
    f_close(&file);
    if (buf_flash_flush_head != buf_flash_flush) {
        write_op(cur_nand_write_page, 0, buf_flash_flush, PAGE_SIZE);
        --cur_nand_write_page;
        memset(buf_flash_flush, 0, PAGE_SIZE);
        buf_flash_flush_head = buf_flash_flush;
    }
#endif

#if WORKLOAD_DATA == DATA_SIFT
    /* write codebook long (codebook for longer PQ codes) to flash */
    codebook_long_write_page_start = cur_nand_write_page;
    memcpy(buf_filename, "LONGBOOK.BIN", 12 + 1);
    fresult = f_open(&file, buf_filename, FA_READ);
    if (fresult != FR_OK) {
        SET_BREAKPOINT(BP_ERROR);
    }
    int16_t buf_cb_long_elem;
    uint32_t cb_long_total_elems = (uint32_t)PQ_VECTOR_DIM * 2 * CENTROID_PER_SUBSPACE * (SUB_VECTOR_DIM / 2);
    for (uint32_t idx = 0; idx < cb_long_total_elems; ++idx) {
        fresult = f_read(&file, (uint8_t*)&buf_cb_long_elem, sizeof(int16_t), &bytesRead);
        if (fresult != FR_OK || bytesRead != sizeof(int16_t)) {
            SET_BREAKPOINT(BP_ERROR);
        }
        memcpy(buf_flash_flush_head, &buf_cb_long_elem, sizeof(int16_t));
        buf_flash_flush_head += sizeof(int16_t);
        if ((uintptr_t)(buf_flash_flush_head + sizeof(int16_t)) > (uintptr_t)&buf_flash_flush[PAGE_SIZE]) {
            write_op(cur_nand_write_page, 0, buf_flash_flush, PAGE_SIZE);
            --cur_nand_write_page;
            memset(buf_flash_flush, 0, PAGE_SIZE);
            buf_flash_flush_head = buf_flash_flush;
        }
    }
    f_close(&file);
    if (buf_flash_flush_head != buf_flash_flush) {
        write_op(cur_nand_write_page, 0, buf_flash_flush, PAGE_SIZE);
        --cur_nand_write_page;
        memset(buf_flash_flush, 0, PAGE_SIZE);
        buf_flash_flush_head = buf_flash_flush;
    }
#endif

#if WORKLOAD_DATA == DATA_SIFT
    /* write ground truth top-K SQUARED DISTANCES to flash (for on-device recall@10) */
    groundtruth_write_page_start = cur_nand_write_page;

    memcpy(buf_filename, "GT.BIN", 6 + 1);
    fresult = f_open(&file, buf_filename, FA_READ);
    if (fresult != FR_OK) {
        SET_BREAKPOINT(BP_ERROR);
    }
    // PAGE_SIZE (2048) is not a multiple of TOP_K_VALUE*4 (40), so each
    // query's GT block is flushed as a whole entry (never split across a
    // page) -- same idea as the L1_Mapping_Entry loop below, just simpler:
    // check-before-append instead of append-then-check.
    uint32_t buf_gt_entry[TOP_K_VALUE];
    for (uint32_t q = 0; q < QUERY_COUNT; ++q) {
        fresult = f_read(&file, (uint8_t*)buf_gt_entry, sizeof(buf_gt_entry), &bytesRead);
        if (fresult != FR_OK || bytesRead != sizeof(buf_gt_entry)) {
            SET_BREAKPOINT(BP_ERROR);
        }

        if ((uintptr_t)(buf_flash_flush_head + sizeof(buf_gt_entry)) > (uintptr_t)&buf_flash_flush[PAGE_SIZE]) {
            write_op(cur_nand_write_page, 0, buf_flash_flush, PAGE_SIZE);
            --cur_nand_write_page;
            memset(buf_flash_flush, 0, PAGE_SIZE);
            buf_flash_flush_head = buf_flash_flush;
        }

        memcpy(buf_flash_flush_head, buf_gt_entry, sizeof(buf_gt_entry));
        buf_flash_flush_head += sizeof(buf_gt_entry);
    }
    f_close(&file);
    if (buf_flash_flush_head != buf_flash_flush) {
        write_op(cur_nand_write_page, 0, buf_flash_flush, PAGE_SIZE);
        --cur_nand_write_page;
        memset(buf_flash_flush, 0, PAGE_SIZE);
        buf_flash_flush_head = buf_flash_flush;
    }
#endif
    SET_BREAKPOINT(BP_FINISHED);
}
#endif

void setup_sample_rate_timer()
{
    Timer_B_initUpModeParam initUpParam = {0};
    initUpParam.clockSource = TIMER_B_CLOCKSOURCE_SMCLK;
    initUpParam.clockSourceDivider = TIMER_B_CLOCKSOURCE_DIVIDER_32;
    initUpParam.timerPeriod = (CS_getSMCLK() >> 5) / SAMPLE_RATE;
    initUpParam.timerInterruptEnable_TBIE = TIMER_B_TBIE_INTERRUPT_ENABLE;
    initUpParam.timerClear = TIMER_B_DO_CLEAR;
    initUpParam.startTimer = false;
    Timer_B_initUpMode(TIMER_B0_BASE, &initUpParam);
    Timer_B_clear(TIMER_B0_BASE);
    Timer_B_startCounter(TIMER_B0_BASE, TIMER_B_UP_MODE);
}

#pragma vector=TIMER0_B1_VECTOR
__interrupt void B0_ISR( void )
{
    TB0CTL &= ~TBIFG;

    ++buffered_records_cnt;

    __bic_SR_register_on_exit( SCG1 + SCG0 + OSCOFF + CPUOFF );
}
