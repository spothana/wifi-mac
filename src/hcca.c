/* hcca.c - see hcca.h */
#include "hcca.h"
#include <stdio.h>
#include <stdlib.h>

/* Schedule highest-priority streams first (delay-bound proxy). qsort cmp. */
static int by_priority_desc(const void *a, const void *b) {
    const hcca_stream_t *x = a, *y = b;
    if (x->priority != y->priority)
        return (int)y->priority - (int)x->priority;
    return x->id - y->id; /* stable-ish tie-break */
}

hcca_result_t hcca_run_cap(hcca_stream_t *streams, int n,
                           usec_t cap_max_us, bool verbose)
{
    hcca_result_t r = {0};

    /* HC gains the medium after a PIFS to start the Controlled Access Phase. */
    r.clock_us += PIFS_US;
    if (verbose)
        printf("[%8llu us] HCCA: HC seizes medium, CAP begins\n",
               (unsigned long long)r.clock_us);

    qsort(streams, (size_t)n, sizeof(streams[0]), by_priority_desc);

    bool work = true;
    while (work) {
        work = false;
        for (int i = 0; i < n; i++) {
            if (streams[i].pending == 0) continue;

            /* Issue a QoS CF-Poll granting a TXOP. Check CAP budget first. */
            usec_t poll_cost = SIFS_US + POLL_TX_US;
            if (r.clock_us + poll_cost + SIFS_US > cap_max_us) {
                if (verbose)
                    printf("[%8llu us] HCCA: CAP budget exhausted\n",
                           (unsigned long long)r.clock_us);
                goto end_cap;
            }

            r.clock_us += poll_cost;
            r.polls_issued++;

            /* The station transmits frames back-to-back (SIFS-separated)
             * until its granted TXOP is used up or its queue drains. */
            usec_t txop_used = 0;
            usec_t one = DATA_TX_US + SIFS_US + ACK_TX_US;
            usec_t grant = streams[i].txop_grant_us;
            while (streams[i].pending > 0 && txop_used + one <= grant) {
                /* Don't overrun the CAP itself. */
                if (r.clock_us + SIFS_US + one > cap_max_us) {
                    if (verbose)
                        printf("[%8llu us] HCCA: CAP budget cut TXOP for STA %d\n",
                               (unsigned long long)r.clock_us, streams[i].id);
                    goto end_cap;
                }
                r.clock_us += SIFS_US + one;
                txop_used  += one;
                streams[i].pending--;
                streams[i].delivered++;
                r.frames_delivered++;
            }
            r.granted_txop_total_us += txop_used;
            if (verbose)
                printf("[%8llu us] HCCA: STA %d granted TXOP, used %llu us, "
                       "pending=%u\n", (unsigned long long)r.clock_us,
                       streams[i].id, (unsigned long long)txop_used,
                       streams[i].pending);
            if (streams[i].pending > 0) work = true; /* needs another poll  */
        }
    }

end_cap:
    if (verbose)
        printf("[%8llu us] HCCA: CAP ends\n",
               (unsigned long long)r.clock_us);
    return r;
}
