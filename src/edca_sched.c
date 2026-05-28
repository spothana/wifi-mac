/* edca_sched.c - see edca_sched.h */
#include "edca_sched.h"
#include <stdio.h>

/* AIFSN measured in slots relative to the same reference. Lower AIFSN => the
 * AC begins decrementing its backoff sooner, so its effective countdown is
 * smaller. We treat (AIFSN + backoff) as the slots until this AC fires. */
static uint32_t countdown(const edca_station_t *s, int a) {
    return EDCA_DEFAULT[a].aifsn + s->ac[a].backoff;
}

static bool any_active(const edca_station_t *st, int n) {
    for (int i = 0; i < n; i++)
        for (int a = 0; a < AC_COUNT; a++)
            if (st[i].ac[a].active) return true;
    return false;
}

edca_result_t edca_run(edca_station_t *st, int n,
                       uint32_t max_rounds, bool verbose)
{
    edca_result_t r = {0};

    while (any_active(st, n) && r.rounds < max_rounds) {
        r.rounds++;

        /* Find global minimum effective countdown among active ACs. */
        uint32_t min_cd = 0xFFFFFFFFu;
        for (int i = 0; i < n; i++)
            for (int a = 0; a < AC_COUNT; a++)
                if (st[i].ac[a].active) {
                    uint32_t cd = countdown(&st[i], a);
                    if (cd < min_cd) min_cd = cd;
                }
        if (min_cd == 0xFFFFFFFFu) break;

        /* Idle time: SIFS-equivalent reference + min_cd slots elapse. We bill
         * the medium-idle slots; the AIFS portion is folded into the slots. */
        r.clock_us += (usec_t)min_cd * SLOT_TIME_US + SIFS_US;

        /* Collect contenders reaching the medium this round. */
        int  tx_station[AC_COUNT * 8]; /* generous bound */
        int  tx_ac[AC_COUNT * 8];
        int  txn = 0;
        for (int i = 0; i < n && txn < (int)(AC_COUNT*8); i++)
            for (int a = 0; a < AC_COUNT; a++)
                if (st[i].ac[a].active && countdown(&st[i], a) == min_cd) {
                    tx_station[txn] = i; tx_ac[txn] = a; txn++;
                }

        /* Resolve internal (virtual) collisions: within each station keep only
         * the highest-priority AC; lower ACs take an internal collision. */
        for (int i = 0; i < n; i++) {
            int best = -1;
            for (int k = 0; k < txn; k++)
                if (tx_station[k] == i &&
                    (best < 0 || tx_ac[k] > best)) best = tx_ac[k];
            if (best < 0) continue;
            for (int k = 0; k < txn; k++)
                if (tx_station[k] == i && tx_ac[k] != best) {
                    st[i].ac[tx_ac[k]].internal_coll++;
                    r.internal_collisions++;
                    if (verbose)
                        printf("[%8llu us] STA %d internal coll: %s yields to %s\n",
                               (unsigned long long)r.clock_us, i,
                               ac_name(tx_ac[k]), ac_name(best));
                    edca_on_result(&st[i], (access_category_t)tx_ac[k], false);
                    tx_station[k] = -1; /* removed from external contention   */
                }
        }

        /* Count surviving external transmitters. */
        int survivors = 0, win_i = -1, win_a = -1;
        for (int k = 0; k < txn; k++)
            if (tx_station[k] >= 0) { survivors++; win_i = tx_station[k]; win_a = tx_ac[k]; }

        if (survivors == 1) {
            usec_t txop = EDCA_DEFAULT[win_a].txop_limit_us;
            /* A TXOP can carry multiple frames; model at least one DATA+ACK,
             * and bound extra frames by the TXOP limit for higher ACs. */
            usec_t one = DATA_TX_US + SIFS_US + ACK_TX_US;
            usec_t used = one;
            edca_on_result(&st[win_i], (access_category_t)win_a, true);
            r.success[win_a]++;
            while (txop > used + one && st[win_i].ac[win_a].active) {
                used += one;
                edca_on_result(&st[win_i], (access_category_t)win_a, true);
                r.success[win_a]++;
            }
            r.clock_us += used;
            if (verbose)
                printf("[%8llu us] STA %d TX ok on %s (TXOP used %llu us)\n",
                       (unsigned long long)r.clock_us, win_i, ac_name(win_a),
                       (unsigned long long)used);
        } else if (survivors >= 2) {
            r.clock_us += DATA_TX_US;
            r.external_collisions++;
            if (verbose)
                printf("[%8llu us] EXTERNAL collision among %d ACs\n",
                       (unsigned long long)r.clock_us, survivors);
            for (int k = 0; k < txn; k++)
                if (tx_station[k] >= 0)
                    edca_on_result(&st[tx_station[k]],
                                   (access_category_t)tx_ac[k], false);
        }
    }
    return r;
}
