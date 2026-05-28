/* rate_control.c - see rate_control.h */
#include "rate_control.h"

static bool recipient_supports(const recipient_caps_t *r,
                               const mcs_tuple_t *t) {
    for (int i = 0; i < r->n; i++)
        if (r->tuples[i].mcs == t->mcs && r->tuples[i].nss == t->nss)
            return true;
    return false;
}

bool rate_common_to_all(const recipient_caps_t *recips, int n_recips,
                        mcs_tuple_t *out) {
    if (n_recips <= 0) return false;
    bool found = false;
    mcs_tuple_t best = {0, 0, 0};

    /* Candidate tuples are those of the first recipient; for each, check that
     * every other recipient also supports it, then keep the highest rate. */
    const recipient_caps_t *r0 = &recips[0];
    for (int i = 0; i < r0->n; i++) {
        const mcs_tuple_t *cand = &r0->tuples[i];
        bool all = true;
        for (int j = 1; j < n_recips; j++)
            if (!recipient_supports(&recips[j], cand)) { all = false; break; }
        if (all && (!found || cand->rate_mbps > best.rate_mbps)) {
            best = *cand; found = true;
        }
    }
    if (found) *out = best;
    return found;
}

uint32_t nonht_reference_rate(uint32_t mcs) {
    /* Small teaching lookup: a monotonic mapping from MCS index to a non-HT
     * OFDM reference rate (6..54 Mbit/s). Real tables are PHY-specific. */
    static const uint32_t table[] = { 6, 12, 18, 24, 36, 48, 54, 54, 54, 54 };
    if (mcs >= sizeof(table)/sizeof(table[0]))
        return table[sizeof(table)/sizeof(table[0]) - 1];
    return table[mcs];
}

uint32_t basic_rate_at_or_below(uint32_t reference_mbps,
                                const uint32_t *basic_rates, int n) {
    uint32_t best = 0;
    for (int i = 0; i < n; i++)
        if (basic_rates[i] <= reference_mbps && basic_rates[i] > best)
            best = basic_rates[i];
    return best; /* 0 == none qualifies */
}
