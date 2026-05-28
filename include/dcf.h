/* dcf.h - Distributed Coordination Function (802.11-2020 Clause 10.3)
 *
 * Models the basic CSMA/CA access method with binary exponential backoff.
 * A station is a small state machine; the channel model (channel.h) decides
 * collisions when two or more stations transmit in the same slot.
 */
#ifndef DCF_H
#define DCF_H

#include "mac_common.h"
#include "rng.h"

typedef enum {
    DCF_IDLE,        /* no frame queued                                   */
    DCF_DIFS_WAIT,   /* sensing channel idle for a DIFS before backoff    */
    DCF_BACKOFF,     /* counting down the backoff slot counter            */
    DCF_TX,          /* transmitting a data frame this slot               */
    DCF_WAIT_ACK,    /* waiting SIFS + ACK                                */
    DCF_DONE         /* all queued frames delivered                       */
} dcf_state_t;

typedef struct {
    int          id;
    dcf_state_t  state;
    uint32_t     cw;            /* current contention window               */
    uint32_t     backoff;       /* remaining backoff slots                 */
    uint32_t     retries;       /* retry count for the head-of-line frame  */
    uint32_t     frames_queued; /* frames still to send                    */
    uint32_t     frames_sent;   /* successfully acked frames               */
    uint32_t     collisions;    /* collision events observed               */
    rng_t        rng;
} dcf_station_t;

void dcf_init(dcf_station_t *s, int id, uint32_t frames, uint64_t seed);

/* Pick a fresh backoff in [0, CW]; called on a new frame or after collision. */
void dcf_choose_backoff(dcf_station_t *s);

/* Called by the scheduler when a transmission attempt resolved.
 * success=true  -> reset CW to CWmin, advance to next frame
 * success=false -> double CW (capped), increment retries, redraw backoff */
void dcf_on_tx_result(dcf_station_t *s, bool success);

#endif /* DCF_H */
