// Stubs for running SIMtrace AT91 firmware on host.

#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include "simtrace_usb.h"
#include <stdio.h>
#define DEBUGPCRF(...) fprintf(stderr, __VA_ARGS__)
#define DEBUGPCR DEBUGPCRF
#define __ramfunc
struct req_ctx {
	unsigned char *data;
	int tot_len;
	int size;
};
static inline struct req_ctx *req_ctx_find_get(int a, int b, int c) { return NULL; }

struct usart {
    int US_CR;
    int US_CSR;
    int US_RHR;
    int US_FIDI;
    int US_NER;
    int US_IER;
    int US_MR;
    int US_BRGR;
    int US_IDR;
    int US_RTOR;
    int US_TTGR;
};

struct usart _usart;
typedef struct usart *AT91PS_USART;
#define AT91C_BASE_US0 &_usart;


enum stub_const {
    RCTX_STATE_FREE,
    RCTX_STATE_LIBRFID_BUSY,
    RCTX_STATE_UDP_EP2_PENDING,

    AT91C_US_RXDIS,
    AT91C_US_RSTRX,
    AT91C_US_RXEN,
    AT91C_US_STTTO,
    AT91C_US_RXRDY,
    AT91C_US_TXRDY,
    AT91C_US_PARE,
    AT91C_US_FRAME,
    AT91C_US_OVRE,
    AT91C_US_RSTSTA,
    AT91C_US_INACK,
    AT91C_US_NACK,
    AT91C_US_ITERATION,
    AT91C_US_RSTNACK,
    AT91C_BASE_PIOA,
    AT91C_US_USMODE_ISO7816_0,
    AT91C_US_CLKS_CLOCK,
    AT91C_US_CHRL_8_BITS,
    AT91C_US_NBSTOP_1_BIT,
    AT91C_US_CKLO,
    AT91C_US_CLKS_EXT,
    AT91C_BASE_AIC,
    AT91C_ID_US0,
    AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL,
    AT91C_US_TXDIS,
    AT91C_US_RSTTX,

    SIMTRACE_PIO_nRST,
    SIMTRACE_PIO_IO,
    SIMTRACE_PIO_CLK,

    OPENPCD_IRQ_PRIO_USART,

};





