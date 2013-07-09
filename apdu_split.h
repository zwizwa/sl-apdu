#ifndef _APDU_SPLIT_H
#define _APDU_SPLIT_H

#include <stdint.h>

struct apdu_split;

typedef void apdu_cb_t(uint8_t *buf, unsigned int len, void *user_data);

struct apdu_split *apdu_split_init(apdu_cb_t *apdu_out_cb, void *user_data);

int apdu_split_reset(struct apdu_split *as);
void apdu_split_in(struct apdu_split *as, uint8_t *buf, int len);
void apdu_split_boundary(struct apdu_split *as);

#endif
