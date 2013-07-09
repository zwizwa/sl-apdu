#ifndef _APDU_H_
#define _APDU_H_

#include "util.h"

#ifdef __cplusplus
extern "C" {
#endif

    struct apdu_state;
    /* Convenience: Push 8 channel binary data to analyzer. */
    void apdu_push8(struct apdu_state *s, const u8 *data, uint size);

    struct apdu_state *apdu_new(uint f_sample /* logic analyzer sample frequency */,
                                uint f_clock  /* clock freq passed to card */);

#ifdef __cplusplus
}
#endif

#endif //
