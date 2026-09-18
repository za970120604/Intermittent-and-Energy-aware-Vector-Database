#include "FreeRTOS.h"
#include "driverlib.h"
#include "spi.h"
#include "config.h"
#include "debug.h"

#if (SPI_MODE == SPI_MODE_DMA)
static uint8_t spi_dma_dummy_tx = 0x00;
volatile uint8_t spi_dma_done = 0;
#define SPI_DMA_TIMEOUT_COUNT  10000UL
#endif

#if (UCx_MODE == UCB_MODE_UCB1 && SPI_MODE == SPI_MODE_BUSYPOLLING)
int spi_send(const uint8_t* send_buffer, size_t buffer_len)
{
    int i;
    volatile uint8_t dummy;
    for (i = 0; i < buffer_len; ++i)
    {
        while (!(UCB1IFG & UCTXIFG)); 
        UCB1TXBUF = send_buffer[i]; 
        while (!(UCB1IFG & UCRXIFG));
        dummy = UCB1RXBUF;
    }

    return 0;
}

int spi_recv(uint8_t *recv_buffer, size_t buffer_len)
{
    int i;
    for (i = 0; i < buffer_len; ++i)
    {
        while (!(UCB1IFG & UCTXIFG));
        UCB1TXBUF = 0; 
        while (!(UCB1IFG & UCRXIFG)); 
        recv_buffer[i] = UCB1RXBUF; 
    }

    return 0;
}

int spi_send_recv(const uint8_t* send_buffer, uint8_t *recv_buffer, size_t buffer_len)
{
    int i;
    for (i = 0; i < buffer_len; ++i)
    {
        while (!(UCB1IFG & UCTXIFG));
        UCB1TXBUF = send_buffer[i]; 
        while (!(UCB1IFG & UCRXIFG));
        recv_buffer[i] = UCB1RXBUF; 
    }

    return 0;
}

#elif (UCx_MODE == UCB_MODE_UCB0 && SPI_MODE == SPI_MODE_BUSYPOLLING)
int spi_send(const uint8_t* send_buffer, size_t buffer_len)
{
    int i;
    volatile uint8_t dummy;
    for (i = 0; i < buffer_len; ++i)
    {
        while (!(UCB0IFG & UCTXIFG));
        UCB0TXBUF = send_buffer[i];
        while (!(UCB0IFG & UCRXIFG));
        dummy = UCB0RXBUF;
    }

    return 0;
}

int spi_recv(uint8_t *recv_buffer, size_t buffer_len)
{
    int i;
    for (i = 0; i < buffer_len; ++i)
    {
        while (!(UCB0IFG & UCTXIFG));
        UCB0TXBUF = 0;
        while (!(UCB0IFG & UCRXIFG));
        recv_buffer[i] = UCB0RXBUF;
    }

    return 0;
}

int spi_send_recv(const uint8_t* send_buffer, uint8_t *recv_buffer, size_t buffer_len)
{
    int i;
    for (i = 0; i < buffer_len; ++i)
    {
        while (!(UCB0IFG & UCTXIFG));
        UCB0TXBUF = send_buffer[i];
        while (!(UCB0IFG & UCRXIFG));
        recv_buffer[i] = UCB0RXBUF;
    }

    return 0;
}
#elif (UCx_MODE == UCB_MODE_UCB0 && SPI_MODE == SPI_MODE_DMA)
int spi_send(const uint8_t* send_buffer, size_t buffer_len)
{
    int i;
    volatile uint8_t dummy;
    for (i = 0; i < buffer_len; ++i)
    {
        while (!(UCB0IFG & UCTXIFG));
        UCB0TXBUF = send_buffer[i];
        while (!(UCB0IFG & UCRXIFG));
        dummy = UCB0RXBUF;
    }

    return 0;
}

int spi_recv_dma(uint32_t recv_addr, size_t buffer_len)
{
    if (buffer_len == 0) {
        return 0;
    }

    DMACTL0 = (DMA0TSEL__UCB0RXIFG | DMA1TSEL__UCB0TXIFG); // Map DMA triggers: DMA0 <- UCB0RXIFG, DMA1 <- UCB0TXIFG
    UCB0IFG &= ~UCRXIFG; // Clear stale RX flag to avoid unintended first DMA trigger
    spi_dma_done = 0;

    // DMA0: SPI RXBUF -> recv buffer (increment dst)
    // DMA0CTL = 0;
    DMA_setSrcAddress(DMA_CHANNEL_0, EUSCI_B_SPI_getReceiveBufferAddress(EUSCI_B0_BASE), DMA_DIRECTION_UNCHANGED);
    DMA_setDstAddress(DMA_CHANNEL_0, recv_addr, DMA_DIRECTION_INCREMENT);
    DMA0SZ = buffer_len;
    DMA0CTL = DMADT_0 | DMASRCBYTE | DMADSTBYTE | DMASRCINCR_0 | DMADSTINCR_3 | DMAIE | DMAEN; // enable RX interrupt, because we want to be notified that flash data is ready to be used

    // DMA1: dummy byte -> SPI TXBUF (generate clock)
    // DMA1CTL = 0;
    DMA_setSrcAddress(DMA_CHANNEL_1, (uint32_t)&spi_dma_dummy_tx, DMA_DIRECTION_UNCHANGED);
    DMA_setDstAddress(DMA_CHANNEL_1, EUSCI_B_SPI_getTransmitBufferAddress(EUSCI_B0_BASE), DMA_DIRECTION_UNCHANGED);
    DMA1SZ = buffer_len - 1; // because we'll manually send first byte via CPU, which means we'll manually kick out the transfer
    DMA1CTL = DMADT_0 | DMASRCBYTE | DMADSTBYTE | DMASRCINCR_0 | DMADSTINCR_0 | DMAEN;

    // Kick-start SPI transfer (first byte)
    while (!(UCB0IFG & UCTXIFG));
    UCB0TXBUF = 0x00;

    volatile uint32_t timeout = SPI_DMA_TIMEOUT_COUNT; // we'll decouple this blocking wait in the below functions used for overlapping IO/Compute
    while (!spi_dma_done) {
        if (--timeout == 0) {
            SET_BREAKPOINT(BP_ERROR);
            return -1;
        }
    }

    // Ensure last byte fully shifted out
    while (UCB0STATW & UCBUSY);

    // Clear RX flag again for next transaction, but actually we'll do this again when this function is called in the next round, so this line is just for safe sake
    UCB0IFG &= ~UCRXIFG;
    return 0;
}

#pragma vector = DMA_VECTOR
__interrupt void SPI_DMA_ISR(void)
{
    switch (__even_in_range(DMAIV, 16))
    {
        case DMAIV_DMA0IFG:     // DMA0 done: RX transfer complete, this flag corresponds to why DMAIE is set in the DMA0CTL register
            spi_dma_done = 1;   // raise the finish flag. DMAIV indicates the highest-priority pending DMA interrupt source, and reading DMAIV clears the corresponding DMAxIFG flag automatically.
            break;
        default:
            break;
    }
}

#if OVERLAP_MODE == OVERLAP_MODE_ENABLE
void spi_recv_dma_start(uint32_t recv_addr, size_t buffer_len)
{
    if (buffer_len == 0) {
        return;
    }

    DMACTL0 = (DMA0TSEL__UCB0RXIFG | DMA1TSEL__UCB0TXIFG); // Map DMA triggers: DMA0 <- UCB0RXIFG, DMA1 <- UCB0TXIFG
    UCB0IFG &= ~UCRXIFG; // Clear stale RX flag to avoid unintended first DMA trigger
    spi_dma_done = 0;

    // DMA0: SPI RXBUF -> recv buffer (increment dst)
    // DMA0CTL = 0;
    DMA_setSrcAddress(DMA_CHANNEL_0, EUSCI_B_SPI_getReceiveBufferAddress(EUSCI_B0_BASE), DMA_DIRECTION_UNCHANGED);
    DMA_setDstAddress(DMA_CHANNEL_0, recv_addr, DMA_DIRECTION_INCREMENT);
    DMA0SZ = buffer_len;
    DMA0CTL = DMADT_0 | DMASRCBYTE | DMADSTBYTE | DMASRCINCR_0 | DMADSTINCR_3 | DMAIE | DMAEN; // enable RX interrupt, because we want to be notified that flash data is ready to be used

    // DMA1: dummy byte -> SPI TXBUF (generate clock)
    // DMA1CTL = 0;
    DMA_setSrcAddress(DMA_CHANNEL_1, (uint32_t)&spi_dma_dummy_tx, DMA_DIRECTION_UNCHANGED);
    DMA_setDstAddress(DMA_CHANNEL_1, EUSCI_B_SPI_getTransmitBufferAddress(EUSCI_B0_BASE), DMA_DIRECTION_UNCHANGED);
    DMA1SZ = buffer_len - 1; // because we'll manually send first byte via CPU, which means we'll manually kick out the transfer
    DMA1CTL = DMADT_0 | DMASRCBYTE | DMADSTBYTE | DMASRCINCR_0 | DMADSTINCR_0 | DMAEN;

    // Kick-start SPI transfer (first byte)
    while (!(UCB0IFG & UCTXIFG));
    UCB0TXBUF = 0x00;
    // No blocking wait here, this function is just called for kick-start DMA
}

void spi_recv_dma_wait()
{
    volatile uint32_t timeout = SPI_DMA_TIMEOUT_COUNT;
    while (!spi_dma_done) {
        if (--timeout == 0) {
            SET_BREAKPOINT(BP_ERROR);
            return;
        }
    }

    // Ensure last byte fully shifted out
    while (UCB0STATW & UCBUSY);

    // Clear RX flag again for next transaction
    UCB0IFG &= ~UCRXIFG;
}
#endif

int spi_send_recv(const uint8_t* send_buffer, uint8_t *recv_buffer, size_t buffer_len)
{
    int i;
    for (i = 0; i < buffer_len; ++i)
    {
        while (!(UCB0IFG & UCTXIFG));
        UCB0TXBUF = send_buffer[i];
        while (!(UCB0IFG & UCRXIFG));
        recv_buffer[i] = UCB0RXBUF;
    }

    return 0;
}
#else
int spi_send(const uint8_t* send_buffer, size_t buffer_len)
{
    int i;
    volatile uint8_t dummy;
    for (i = 0; i < buffer_len; ++i)
    {
        while (!(UCA3IFG & UCTXIFG));
        UCA3TXBUF = send_buffer[i];
        while (!(UCA3IFG & UCRXIFG));
        dummy = UCA3RXBUF;
    }

    return 0;
}

#if SPI_MODE == SPI_MODE_BUSYPOLLING
int spi_recv(uint8_t *recv_buffer, size_t buffer_len)
{
    int i;
    for (i = 0; i < buffer_len; ++i)
    {
        while (!(UCA3IFG & UCTXIFG));
        UCA3TXBUF = 0;
        while (!(UCA3IFG & UCRXIFG));
        recv_buffer[i] = UCA3RXBUF;
    }
    return 0;
}
#endif

#if SPI_MODE == SPI_MODE_DMA
int spi_recv_dma(uint32_t recv_addr, size_t buffer_len)
{
#if SPI_DEBUG_MODE == SPI_DEBUG_MODE_ON
    uint32_t st_dma_register = get_current_tick(LOW_RES_CLK);
#endif

    if (buffer_len == 0) {
        return 0;
    }

    DMACTL2 = (DMA4TSEL__UCA3RXIFG | DMA5TSEL__UCA3TXIFG); // Map DMA triggers: DMA4 <- UCA3RXIFG, DMA5 <- UCA3TXIFG
    UCA3IFG &= ~UCRXIFG; // Clear stale RX flag to avoid unintended first DMA trigger
    spi_dma_done = 0;

    // DMA4: SPI RXBUF -> recv buffer (increment dst)
    DMA_setSrcAddress(DMA_CHANNEL_4, EUSCI_A_SPI_getReceiveBufferAddress(EUSCI_A3_BASE), DMA_DIRECTION_UNCHANGED);
    DMA_setDstAddress(DMA_CHANNEL_4, recv_addr, DMA_DIRECTION_INCREMENT);
    DMA4SZ = buffer_len;
    DMA4CTL = DMADT_0 | DMASRCBYTE | DMADSTBYTE | DMASRCINCR_0 | DMADSTINCR_3 | DMAIE | DMAEN; // enable RX interrupt, because we want to be notified that flash data is ready to be used

    // DMA5: dummy byte -> SPI TXBUF (generate clock)
    DMA_setSrcAddress(DMA_CHANNEL_5, (uint32_t)&spi_dma_dummy_tx, DMA_DIRECTION_UNCHANGED);
    DMA_setDstAddress(DMA_CHANNEL_5, EUSCI_A_SPI_getTransmitBufferAddress(EUSCI_A3_BASE), DMA_DIRECTION_UNCHANGED);
    DMA5SZ = buffer_len - 1; // because we'll manually send first byte via CPU, which means we'll manually kick out the transfer
    DMA5CTL = DMADT_0 | DMASRCBYTE | DMADSTBYTE | DMASRCINCR_0 | DMADSTINCR_0 | DMAEN;

    // Kick-start SPI transfer (first byte)
    // while (!(UCA3IFG & UCTXIFG));
    UCA3TXBUF = 0x00;

#if SPI_DEBUG_MODE == SPI_DEBUG_MODE_ON
    uint32_t ed_dma_register = get_current_tick(LOW_RES_CLK);
    tick_dma_register += (ed_dma_register - st_dma_register);
#endif

#if SPI_DEBUG_MODE == SPI_DEBUG_MODE_ON
    uint32_t st_dma_wait = get_current_tick(LOW_RES_CLK);
#endif
    volatile uint32_t timeout = SPI_DMA_TIMEOUT_COUNT; // we'll decouple this blocking wait in the below functions used for overlapping IO/Compute
    while (!spi_dma_done) {
        if (--timeout == 0) {
            SET_BREAKPOINT(BP_ERROR);
            return -1;
        }
    }

    // Ensure last byte fully shifted out
    while (UCA3STATW & UCBUSY);

    // Clear RX flag again for next transaction, but actually we'll do this again when this function is called in the next round, so this line is just for safe sake
    UCA3IFG &= ~UCRXIFG;
#if SPI_DEBUG_MODE == SPI_DEBUG_MODE_ON
    uint32_t ed_dma_wait = get_current_tick(LOW_RES_CLK);
    tick_dma_wait += (ed_dma_wait - st_dma_wait);
#endif
    return 0;
}

#pragma vector = DMA_VECTOR
__interrupt void SPI_DMA_ISR(void)
{
    switch (__even_in_range(DMAIV, 16))
    {
        case DMAIV_DMA4IFG:     // DMA4 done: RX transfer complete, this flag corresponds to why DMAIE is set in the DMA4CTL register
            spi_dma_done = 1;   // raise the finish flag. DMAIV indicates the highest-priority pending DMA interrupt source, and reading DMAIV clears the corresponding DMAxIFG flag automatically.
            break;
        default:
            break;
    }
}

#if OVERLAP_MODE == OVERLAP_MODE_ENABLE
void spi_recv_dma_start(uint32_t recv_addr, size_t buffer_len)
{
#if SPI_DEBUG_MODE == SPI_DEBUG_MODE_ON
    uint32_t st_dma_register = get_current_tick(LOW_RES_CLK);
#endif

    if (buffer_len == 0) {
        return;
    }

    DMACTL2 = (DMA4TSEL__UCA3RXIFG | DMA5TSEL__UCA3TXIFG); // Map DMA triggers: DMA4 <- UCA3RXIFG, DMA5 <- UCA3TXIFG
    UCA3IFG &= ~UCRXIFG; // Clear stale RX flag to avoid unintended first DMA trigger
    spi_dma_done = 0;

    // DMA4: SPI RXBUF -> recv buffer (increment dst)
    DMA_setSrcAddress(DMA_CHANNEL_4, EUSCI_A_SPI_getReceiveBufferAddress(EUSCI_A3_BASE), DMA_DIRECTION_UNCHANGED);
    DMA_setDstAddress(DMA_CHANNEL_4, recv_addr, DMA_DIRECTION_INCREMENT);
    DMA4SZ = buffer_len;
    DMA4CTL = DMADT_0 | DMASRCBYTE | DMADSTBYTE | DMASRCINCR_0 | DMADSTINCR_3 | DMAIE | DMAEN; // enable RX interrupt, because we want to be notified that flash data is ready to be used

    // DMA5: dummy byte -> SPI TXBUF (generate clock)
    DMA_setSrcAddress(DMA_CHANNEL_5, (uint32_t)&spi_dma_dummy_tx, DMA_DIRECTION_UNCHANGED);
    DMA_setDstAddress(DMA_CHANNEL_5, EUSCI_A_SPI_getTransmitBufferAddress(EUSCI_A3_BASE), DMA_DIRECTION_UNCHANGED);
    DMA5SZ = buffer_len - 1; // because we'll manually send first byte via CPU, which means we'll manually kick out the transfer
    DMA5CTL = DMADT_0 | DMASRCBYTE | DMADSTBYTE | DMASRCINCR_0 | DMADSTINCR_0 | DMAEN;

    // Kick-start SPI transfer (first byte)
    // while (!(UCA3IFG & UCTXIFG));
    UCA3TXBUF = 0x00;
    // No blocking wait here, this function is just called for kick-start DMA
#if SPI_DEBUG_MODE == SPI_DEBUG_MODE_ON
    uint32_t ed_dma_register = get_current_tick(LOW_RES_CLK);
    tick_dma_register += (ed_dma_register - st_dma_register);
#endif
}

void spi_recv_dma_wait()
{
#if SPI_DEBUG_MODE == SPI_DEBUG_MODE_ON
    uint32_t st_dma_wait = get_current_tick(LOW_RES_CLK);
#endif
    volatile uint32_t timeout = SPI_DMA_TIMEOUT_COUNT;
    while (!spi_dma_done) {
        if (--timeout == 0) {
            SET_BREAKPOINT(BP_ERROR);
            return;
        }
    }

    // Ensure last byte fully shifted out
    while (UCA3STATW & UCBUSY);

    // Clear RX flag again for next transaction
    UCA3IFG &= ~UCRXIFG;
#if SPI_DEBUG_MODE == SPI_DEBUG_MODE_ON
    uint32_t ed_dma_wait = get_current_tick(LOW_RES_CLK);
    tick_dma_wait += (ed_dma_wait - st_dma_wait);
#endif
}
#endif
#endif

int spi_send_recv(const uint8_t* send_buffer, uint8_t *recv_buffer, size_t buffer_len)
{
    int i;
    for (i = 0; i < buffer_len; ++i)
    {
        while (!(UCA3IFG & UCTXIFG));
        UCA3TXBUF = send_buffer[i];
        while (!(UCA3IFG & UCRXIFG));
        recv_buffer[i] = UCA3RXBUF;
    }

    return 0;
}
#endif
