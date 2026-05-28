/* pcf.c - see pcf.h */
#include "pcf.h"
#include <stdio.h>

pcf_result_t pcf_run_cfp(pcf_station_t *list, int n,
                         usec_t cfp_max_us, bool verbose)
{
    pcf_result_t r = {0};

    /* The PC seizes the medium a PIFS after the medium goes idle, then sends
     * the Beacon that starts the CFP (10.4.3.2). PIFS < DIFS guarantees the
     * PC wins against contending DCF stations. */
    r.clock_us += PIFS_US + BEACON_TX_US;
    if (verbose)
        printf("[%8llu us] PCF: Beacon, CFP begins\n",
               (unsigned long long)r.clock_us);

    /* Round-robin the polling list. The PC may piggyback CF-Poll on data, but
     * we model the canonical "CF-Poll then response" exchange separated by
     * SIFS, which is the inter-frame spacing used inside the CFP. */
    bool work_remaining = true;
    while (work_remaining) {
        work_remaining = false;
        for (int i = 0; i < n; i++) {
            if (list[i].pending == 0) continue;
            work_remaining = true;

            /* CFPMaxDuration check before issuing the next poll. */
            usec_t cost = SIFS_US + POLL_TX_US;       /* poll */
            if (r.clock_us + cost > cfp_max_us) {
                r.cfp_truncated = true;
                if (verbose)
                    printf("[%8llu us] PCF: CFPMaxDuration reached, truncating\n",
                           (unsigned long long)r.clock_us);
                goto end_cfp;
            }

            r.clock_us += cost;
            r.polls_issued++;

            if (list[i].responds && list[i].pending > 0) {
                /* SIFS then a data frame, then SIFS+ACK (CF-Ack). */
                r.clock_us += SIFS_US + DATA_TX_US + SIFS_US + ACK_TX_US;
                list[i].pending--;
                list[i].delivered++;
                r.frames_delivered++;
                if (verbose)
                    printf("[%8llu us] PCF: STA %d polled -> data ok (pending=%u)\n",
                           (unsigned long long)r.clock_us, list[i].id,
                           list[i].pending);
            } else {
                /* No response within SIFS: PC polls the next station after
                 * a PIFS (10.4.4.3). Counts as a null response. The PC does
                 * not keep re-polling a silent station within this CFP, so we
                 * drop it from the remaining poll list by zeroing its queue. */
                r.clock_us += PIFS_US;
                r.null_responses++;
                list[i].pending = 0;
                if (verbose)
                    printf("[%8llu us] PCF: STA %d polled -> null (skipped)\n",
                           (unsigned long long)r.clock_us, list[i].id);
            }
        }
    }

end_cfp:
    /* PC ends the CFP with a CF-End frame after a SIFS. */
    r.clock_us += SIFS_US + ACK_TX_US;
    if (verbose)
        printf("[%8llu us] PCF: CF-End, CFP closes\n",
               (unsigned long long)r.clock_us);
    return r;
}
