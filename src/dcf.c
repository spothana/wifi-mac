/* dcf.c - see dcf.h */
#include "dcf.h"

void dcf_init(dcf_station_t *s, int id, uint32_t frames, uint64_t seed) {
    s->id            = id;
    s->state         = frames ? DCF_DIFS_WAIT : DCF_IDLE;
    s->cw            = DCF_CW_MIN;
    s->retries       = 0;
    s->frames_queued = frames;
    s->frames_sent   = 0;
    s->collisions    = 0;
    rng_seed(&s->rng, seed);
    dcf_choose_backoff(s);
}

void dcf_choose_backoff(dcf_station_t *s) {
    /* Backoff slots uniformly distributed in [0, CW]  (Clause 10.3.3) */
    s->backoff = rng_uniform_incl(&s->rng, s->cw);
}

void dcf_on_tx_result(dcf_station_t *s, bool success) {
    if (success) {
        s->frames_sent++;
        s->frames_queued--;
        s->cw      = DCF_CW_MIN;   /* reset CW on success (10.3.3)       */
        s->retries = 0;
        if (s->frames_queued == 0) {
            s->state = DCF_DONE;
            return;
        }
    } else {
        s->collisions++;
        s->retries++;
        /* Binary exponential backoff: CW = min(2*(CW+1)-1, CWmax). */
        uint32_t next = (s->cw + 1u) * 2u - 1u;
        s->cw = next > DCF_CW_MAX ? DCF_CW_MAX : next;
        if (s->retries > MAX_RETRY) {
            /* Frame dropped after retry limit (dot11ShortRetryLimit). */
            s->frames_queued--;
            s->cw = DCF_CW_MIN;
            s->retries = 0;
            if (s->frames_queued == 0) { s->state = DCF_DONE; return; }
        }
    }
    s->state = DCF_DIFS_WAIT;
    dcf_choose_backoff(s);
}
