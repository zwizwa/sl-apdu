/* simtrace - main program for the host PC
 *
 * (C) 2010 by Harald Welte <hwelte@hmw-consulting.de>
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

#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/types.h>

#include "apdu_split.h"

enum iso7816_apdu_state {
	APDU_S_CLA,
	APDU_S_INS,
	APDU_S_P1,
	APDU_S_P2,
	APDU_S_P3,
	APDU_S_DATA,
	APDU_S_DATA_SINGLE,
	APDU_S_SW1,
	APDU_S_SW2,
};

const char *state_names[] = {
	[APDU_S_CLA]	= "CLA",
	[APDU_S_INS]	= "INS",
	[APDU_S_P1]	= "P1",
	[APDU_S_P2]	= "P2",
	[APDU_S_P3]	= "P3",
	[APDU_S_DATA]	= "DATA",
	[APDU_S_SW1]	= "SW1",
	[APDU_S_SW2]	= "SW2",
};

struct apdu_split {
	apdu_cb_t *apdu_out_cb;
	void *user_data;

	enum iso7816_apdu_state state;
	uint8_t apdu_ins;
	unsigned int apdu_len;
	unsigned int apdu_data_remaining;
	uint8_t apdu_buf[(2<<16)];
};

/* wrapper function to catch apdu_buf overflows */
static void apdu_buf_append(struct apdu_split *as, uint8_t ch)
{
	assert(as->apdu_len < sizeof(as->apdu_buf));
	as->apdu_buf[as->apdu_len++] = ch;
}

static void set_state(struct apdu_split *as, enum iso7816_apdu_state new_state)
{
	switch (new_state) {
	case APDU_S_CLA:
		as->apdu_len = 0;
		memset(as->apdu_buf, 0, sizeof(as->apdu_buf));
		break;
	}

	if (as->state == new_state)
		return;

	//printf("APDU split state %s -> %s\n", state_names[as->state], state_names[new_state]);

	as->state = new_state;
}

static void apdu_split_inbyte(struct apdu_split *as, uint8_t ch)
{
	switch (as->state) {
	case APDU_S_INS:
		as->apdu_ins = ch;
	case APDU_S_CLA:
	case APDU_S_P1:
	case APDU_S_P2:
		apdu_buf_append(as, ch);
		set_state(as, as->state+1);
		break;
	case APDU_S_P3:
		apdu_buf_append(as, ch);
		as->apdu_data_remaining = (ch == 0 ? 256 : ch);
		set_state(as, APDU_S_SW1);
		break;
	case APDU_S_DATA:
		apdu_buf_append(as, ch);
		as->apdu_data_remaining--;
		if (as->apdu_data_remaining == 0)
			set_state(as, APDU_S_SW1);
		break;
	case APDU_S_DATA_SINGLE:
		apdu_buf_append(as, ch);
		as->apdu_data_remaining--;
		set_state(as, APDU_S_SW1);
		break;
	case APDU_S_SW1:
		/* check for NULL / waiting time extension */
		if (ch == 0x60) {
			//printf("NULL");
		} else
		/* check for 'all remaining' type ACK */
		if (ch == as->apdu_ins || ch == as->apdu_ins + 1 ||
		    ch == ~(as->apdu_ins+1))  {
			//printf("ACK");
			set_state(as, APDU_S_DATA);
		} else
		/* check for 'only next byte' type ACK */
		if (ch == ~(as->apdu_ins)) {
			set_state(as, APDU_S_DATA_SINGLE);
		} else {
			/* must be SW1 */
			apdu_buf_append(as, ch);
			set_state(as, APDU_S_SW2);
		}
		break;
	case APDU_S_SW2:
		apdu_buf_append(as, ch);
		//printf("APDU: %s\n", osmo_hexdump(as->apdu_buf, as->apdu_len));
		as->apdu_out_cb(as->apdu_buf, as->apdu_len, as->user_data);
		set_state(as, APDU_S_CLA);
		break;
	}
		
}

/* public API */

struct apdu_split *apdu_split_init(apdu_cb_t *apdu_out_cb, void *user_data)
{
	struct apdu_split *as;

	as = malloc(sizeof(*as));
	if (!as)
		return NULL;

	memset(as, 0, sizeof(*as));
	as->apdu_out_cb = apdu_out_cb;
	as->user_data = user_data;

	return as;
}

int apdu_split_reset(struct apdu_split *as)
{
	set_state(as, APDU_S_CLA);
}

void apdu_split_boundary(struct apdu_split *as)
{
	printf("BOUNDARY\n");
	as->apdu_out_cb(as->apdu_buf, as->apdu_len, as->user_data);
	set_state(as, APDU_S_CLA);
}

void apdu_split_in(struct apdu_split *as, uint8_t *buf, int len)
{
	while (len--)
		apdu_split_inbyte(as, *buf++);
}
