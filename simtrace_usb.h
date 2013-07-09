#ifndef SIMTRACE_USB_H
#define SIMTRACE_USB_H

//#include <stdint.h>

/* this is kept compatible with OpenPCD protocol */
struct simtrace_hdr {
	u_int8_t cmd;
	u_int8_t flags;
	u_int8_t res[2];
	u_int8_t data[0];
} __attribute__ ((packed));

enum simtrace_usb_msgt {
	SIMTRACE_MSGT_NULL,
	SIMTRACE_MSGT_DATA,
	SIMTRACE_MSGT_RESET,		/* reset was asserted, no more data */
	SIMTRACE_MSGT_STATS,		/* statistics */
};

/* flags for MSGT_DATA */
#define SIMTRACE_FLAG_ATR		0x01	/* ATR immediately after reset */
#define SIMTRACE_FLAG_WTIME_EXP		0x04	/* work waiting time expired */
#define SIMTRACE_FLAG_PPS_FIDI		0x08	/* Fi/Di values in res[2] */

struct simtrace_stats {
	u_int32_t no_rctx;
	u_int32_t rctx_sent;
	u_int32_t rst;
	u_int32_t pps;
	u_int32_t bytes;
	u_int32_t parity_err;
	u_int32_t frame_err;
	u_int32_t overrun;
} stats;

#endif /* SIMTRACE_USB_H */
