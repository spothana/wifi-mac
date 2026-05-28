/* rate_control.h - multi-recipient rate selection & non-HT rate conversion.
 *
 * SEGMENT 7 (Rate Control and Multi-Station Communication).
 * SPEC: 802.11-2020 Clause 10.6 (rate selection) / non-HT reference rate.
 *
 * Two mechanisms:
 *  1. Multi-recipient control frames (Trigger, Multi-STA BlockAck, NDP
 *     Announcement) must use a rate/MCS/NSS tuple that EVERY recipient
 *     supports -- otherwise some STA misses the signaling.
 *  2. Non-HT basic rate conversion: to pick a response-frame rate for a legacy
 *     peer, the originating MCS is mapped to a non-HT *reference rate*, and the
 *     responder picks the highest rate in the BSSBasicRateSet that is <= that
 *     reference rate.
 */
#ifndef RATE_CONTROL_H
#define RATE_CONTROL_H

#include "mac_common.h"

/* A supported (MCS, NSS) tuple, plus a nominal PHY rate in Mbit/s used for
 * comparison. For teaching we carry the rate directly. */
typedef struct {
    uint32_t mcs;
    uint32_t nss;
    uint32_t rate_mbps;
} mcs_tuple_t;

/* Each recipient advertises a set of supported tuples. */
typedef struct {
    const mcs_tuple_t *tuples;
    int                n;
} recipient_caps_t;

/* Choose the highest-rate (MCS,NSS) tuple supported by ALL recipients. Returns
 * true and fills `out` on success; false if no common tuple exists. */
bool rate_common_to_all(const recipient_caps_t *recips, int n_recips,
                        mcs_tuple_t *out);

/* Map an arbitrary MCS to its non-HT reference rate (Mbit/s). This is a small
 * teaching lookup approximating the spec's mapping table. */
uint32_t nonht_reference_rate(uint32_t mcs);

/* Given a non-HT reference rate and the BSSBasicRateSet (sorted ascending or
 * not), return the highest basic rate that is <= the reference rate. Returns 0
 * if none qualifies. */
uint32_t basic_rate_at_or_below(uint32_t reference_mbps,
                                const uint32_t *basic_rates, int n);

#endif /* RATE_CONTROL_H */
