/* apdu.c - bit stream ISO7816 parser
 * uses apdu_split to parse APDU bytes -> packets.
 *
 * (C) 2013 by Tom Schouten <tom@zwizwa.be>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 
 *  as published by the Free Software Foundation
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* TODO:
   - fix hardcoded ATR structure.
*/


#include <stdlib.h>
#include <stdio.h>
#include "apdu.h"


// APDU splitter code from SIMtrace
#include "apdu_split.h"


/* ISO7816 interface parser.
   in:  I/O VCC RST
   out: APDU packets */

#define PIN_IO 0
#define PIN_RST 1
#define PIN_VCC 2

enum sm_state {
    sm_idle = 1,
    sm_sample = 2,
    sm_break = 3,
};


// Obtain baud rate as Fi/Di.
const int Di[] = {-1,  1,  2,  4,
                   8, 16, 32, 64,
                  12, 20, -1, -1,
                  -1, -1, -1, -1};


const int Fi[] = { 372,  372,  558,  744,
                  1116, 1488, 1860,   -1,
                    -1,  512,  768, 1024,
                  1536, 2048,   -1,   -1};


struct apdu_state;
typedef void (*sm_byte) (struct apdu_state *s, u8 byte);

struct apdu_state {
    uint f_sample;
    uint f_clock;

    uint clock_div;  // master_clock / baud_rate
    uint last_bus;

    /* APDU splitter */
    struct apdu_split *apdu_split;

    /* Bit-level parser state. */
    enum sm_state state;     // current state
    uint delay;      // clock ticks to next action
    u8   bits_data;
    uint bits_count;
    u8   bits_parity;

    /* Byte-level parser */
    sm_byte parse_byte;

    /* ATR parser */
    int atr_bytes;
    u8 atr_T0;
    u8 atr_TAi;
    u8 atr_TDi;

};


#define LOG(...) fprintf(stderr, __VA_ARGS__)

static void apdu_div(struct apdu_state *s, uint div) {
    s->clock_div = (s->f_sample * div) / s->f_clock;
}



static void apdu_cb(u8 *buf, uint len, void *ctx) {
    LOG("APDU:");
    uint i;
    for (i=0; i<len; i++) LOG(" %02x", buf[i]);
    LOG("\n");
}

static void apdu_parse_byte_apdu(struct apdu_state *s, u8 byte) {
    // LOG("%02x ", byte);
    apdu_split_in(s->apdu_split, &byte, 1);
}


/* Parse card ATR (answer to reset)
   GOAL: find clock divisor (baud rate) for further communication
*/

static void apdu_parse_byte_atr(struct apdu_state *s, u8 byte) {
    switch(s->atr_bytes) {
    case 0:
        LOG("ATR:");
        if (0x3B != byte) {
            LOG("ATR: unexpexted TS %02x\n", byte);
            exit(1);
        }
        break;
    case 1:
        s->atr_T0 = byte;
        // Currently hardcoded to only expect TAi and TDi, and 8 historical bytes.
        if (0x98 != byte) {
            LOG("ATR: unexpected T0 %02x\n", byte);
            exit(1);
        }
        break;
    case 2: s->atr_TAi = byte; break;
    case 3: s->atr_TDi = byte; break;
    default:
        break;
    }
    s->atr_bytes++;
    LOG(" %02x", byte);


    /* Got ATR + PTS + PTS ack
       -> switch to higher baud rate and start parsing APDU packets. */
    if (s->atr_bytes >=    // FIXME: hardcoded sizes
        12 /* ATR */ +
        8  /* PTS + ack */) {
        // http://en.wikipedia.org/wiki/Answer_to_reset#ATR_in_asynchronous_transmission
        uint i_Fi = s->atr_TAi >> 4;
        uint i_Di = s->atr_TAi & 0x0F;
        uint div = Fi[i_Fi] / Di[i_Di];
        apdu_div(s, div);         // new baud rate
        s->parse_byte = apdu_parse_byte_apdu; // new byte parser
        apdu_split_reset(s->apdu_split);
        LOG(" -> Clock div %d (%d)\n", div, (s->clock_div));
    }
}


static void apdu_reset(struct apdu_state *s) {
    // bit parser
    /* The ISO-7816 standard states that the ATR will be sent at 372
       clock cycles per bit.  */
    apdu_div(s, 372);
    s->state = sm_break;

    // byte parser
    s->parse_byte = apdu_parse_byte_atr;
    s->atr_bytes = 0;
}


// For logging: switch state.
static inline void apdu_state(struct apdu_state *s,
                              enum sm_state state) {
    if (0) {
        switch(state) {
        case sm_idle:   LOG("I"); break;
        case sm_sample: LOG("S"); break;
        case sm_break:  LOG("B"); break;
        default: break;
        }
    }
    s->state = state;
}


/* card PAD

+-----------------\
|                  \
|                  |
| GND VPP I/O (NC) |
|                  |
| VCC RST CLK (NC) |
+------------------+

*/
#define PIN(n) (1 << (n))
#define READ_PIN(pins,n) (0 != ((pins) & PIN(n)))

static inline void apdu_tick(struct apdu_state *s, uint bus) {
    // need to monitor all pins to correctly track state
    int io  = READ_PIN(bus, PIN_IO);
    int rst = READ_PIN(bus, PIN_RST);
    int vcc = READ_PIN(bus, PIN_VCC);

    uint xbus = bus ^ (s->last_bus); // changes
    int xrst = READ_PIN(xbus, PIN_RST);
    int xvcc = READ_PIN(xbus, PIN_VCC);

    // Monitor VCC
    if (xvcc) {
        LOG("VCC: %d\n", vcc);
    }

    // Monitor RST
    if (xrst) {
        LOG("RST: %d\n", rst);
    }

    // Normal operation
    if (vcc && rst) {
        switch (s->state) {
        case sm_idle:
            if (io) break; // still idle

            // Skip start bit and sample first bit in the middle.
            apdu_state(s, sm_sample);
            s->bits_data = 0;
            s->bits_count = 0;
            s->bits_parity = 0;
            s->delay = s->clock_div + (s->clock_div / 2);
            break;

        case sm_break:
            if (!io) break; // still break
            apdu_state(s, sm_idle);
            break;

        case sm_sample:
            if (s->delay > 0) {
                s->delay--;
            }
            else {
                switch(s->bits_count) {
                case 9: // STOP BIT
                    if (!io) {
                        LOG("F");
                        // apdu_state(s, sm_break);  // FIXME
                    }
                    // Push to byte-level parser.
                    if (s->parse_byte) s->parse_byte(s, s->bits_data);
                    apdu_state(s, sm_idle);
                    break;
                case 8:  // PARITY BIT
                    if (io != s->bits_parity) {
                        LOG("P");
                        // ignore it
                    }
                    break;
                default: // DATA BIT
                    // Store and and schedule next sample.
                    // LOG("%c", io ? 'x' : '_');
                    s->bits_parity ^= io;
                    s->bits_data |= (io & 1) << s->bits_count;
                    break;
                }
                s->bits_count++;
                s->delay = s->clock_div;
            }
            break;
        default:
            break;
        }
    }
    else {
        /* No VCC or RST: Keep parser state machines in reset. */
        apdu_reset(s);
    }
    s->last_bus = bus;
}

/* Pick a single channel out of 8 channel input data and push to analyzer. */
void apdu_push8(struct apdu_state *s, const u8 *data, uint size){
    int i;
    for (i=0; i<size; i++) {
        apdu_tick(s, data[i]);
    }
}


struct apdu_state *apdu_new(uint f_sample /* logic analyzer sample frequency */,
                            uint f_clock  /* clock freq passed to card */) {

    struct apdu_state *s = calloc(1,sizeof(s));
    s->state    = sm_break;
    s->f_clock  = f_clock;
    s->f_sample = f_sample;

    s->apdu_split = apdu_split_init(apdu_cb, s);

    apdu_reset(s);

    return s;
}
