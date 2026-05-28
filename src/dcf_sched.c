/* dcf_sched.c - see dcf_sched.h */
#include "dcf_sched.h"
#include <stdio.h>

static bool any_active(dcf_station_t *st, int n) {
    for (int i = 0; i < n; i++)
        if (st[i].state != DCF_DONE && st[i].state != DCF_IDLE)
            return true;
    return false;
}

dcf_result_t dcf_run(dcf_station_t *st, int n, uint32_t max_rounds, bool verbose)
{
    dcf_result_t r = {0};

    while (any_active(st, n) && r.rounds < max_rounds) {
        r.rounds++;

        /* DIFS of medium-idle precedes each contention round. */
        r.clock_us += DIFS_US;

        /* Find the minimum nonzero-or-zero backoff among contenders. The
         * medium stays idle for that many slots, then the winners transmit. */
        uint32_t min_bo = 0xFFFFFFFFu;
        for (int i = 0; i < n; i++) {
            if (st[i].state == DCF_DIFS_WAIT || st[i].state == DCF_BACKOFF) {
                st[i].state = DCF_BACKOFF;
                if (st[i].backoff < min_bo) min_bo = st[i].backoff;
            }
        }
        if (min_bo == 0xFFFFFFFFu) break; /* nobody ready */

        /* Idle slots elapse while everyone decrements toward the winner. */
        r.clock_us += (usec_t)min_bo * SLOT_TIME_US;

        /* Decrement all backoffs by min_bo; those hitting 0 transmit. */
        int txers = 0, winner = -1;
        for (int i = 0; i < n; i++) {
            if (st[i].state != DCF_BACKOFF) continue;
            st[i].backoff -= min_bo;
            if (st[i].backoff == 0) { txers++; winner = i; }
        }

        if (txers == 1) {
            /* Successful exchange: DATA + SIFS + ACK. */
            r.clock_us += DATA_TX_US + SIFS_US + ACK_TX_US;
            r.total_success++;
            if (verbose)
                printf("[%8llu us] STA %d TX ok  (cw=%u retries=%u)\n",
                       (unsigned long long)r.clock_us, st[winner].id,
                       st[winner].cw, st[winner].retries);
            dcf_on_tx_result(&st[winner], true);
        } else {
            /* Collision: the medium is busy for a DATA time, no ACK. */
            r.clock_us += DATA_TX_US;
            r.total_collisions++;
            if (verbose)
                printf("[%8llu us] COLLISION among %d stations\n",
                       (unsigned long long)r.clock_us, txers);
            for (int i = 0; i < n; i++)
                if (st[i].state == DCF_BACKOFF && st[i].backoff == 0)
                    dcf_on_tx_result(&st[i], false);
        }
    }
    return r;
}
