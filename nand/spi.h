#ifndef SPI_H_
#define SPI_H_

#include <stddef.h>
#include "config.h"
extern uint64_t tick_dma_register;
extern uint64_t tick_dma_wait;

int spi_send(const uint8_t *send_buffer, size_t buffer_len);
int spi_send_recv(const uint8_t *send_buffer, uint8_t *recv_buffer, size_t buffer_len);

#if (SPI_MODE == SPI_MODE_DMA)
int spi_recv_dma(uint32_t recv_addr, size_t buffer_len);

#if OVERLAP_MODE == OVERLAP_MODE_ENABLE
void spi_recv_dma_start(uint32_t recv_addr, size_t buffer_len);
void spi_recv_dma_wait();
#endif

#else
int spi_recv(uint8_t *recv_buffer, size_t buffer_len);
#endif

#endif
