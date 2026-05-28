/* edca.h - Enhanced Distributed Channel Access (802.11-2020 Clause 10.22.2)
 *
 * EDCA is the contention-based half of HCF. Each station runs FOUR independent
 * backoff entities, one per Access Category (AC_BK/BE/VI/VO), each with its own
 * AIFSN, CWmin, CWmax and TXOP limit (Table 10-1). ACs with smaller AIFS and
 * CW win the medium more often, giving statistical QoS prioritization.
 *
 * Two collision types matter:
 *   - INTERNAL (virtual) collision: two ACs in the SAME station reach zero in
 *     the same slot. The higher-priority AC "wins" and transmits; the lower one
 *     behaves as if it suffered an external collision (10.22.2.3).
 *   - EXTERNAL collision: ACs in DIFFERENT stations transmit in the same slot.
 *
 * The scheduler in edca_sched.c models both.
 */
#ifndef EDCA_H
#define EDCA_H

#include "mac_common.h"
#include "rng.h"

typedef struct {
    bool     active;
    uint32_t cw;
    uint32_t backoff;       /* in slots, measured AFTER the AIFS            */
    uint32_t retries;
    uint32_t pending;       /* frames queued in this AC                     */
    uint32_t delivered;
    uint32_t internal_coll; /* virtual collisions lost to a higher AC       */
    uint32_t external_coll;
} edca_ac_t;

typedef struct {
    int       id;
    edca_ac_t ac[AC_COUNT];
    rng_t     rng;
} edca_station_t;

void edca_init(edca_station_t *s, int id,
               const uint32_t pending_per_ac[AC_COUNT], uint64_t seed);

void edca_choose_backoff(edca_station_t *s, access_category_t ac);

/* Update one AC after its transmission attempt resolved. */
void edca_on_result(edca_station_t *s, access_category_t ac, bool success);

#endif /* EDCA_H */
