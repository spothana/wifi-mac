/* edca_triggers.h - EDCA backoff-procedure invocation triggers.
 *
 * SEGMENT 2 (Prioritizing Traffic with EDCA).
 * SPEC: 802.11-2020 Clause 10.22.2.2 (EDCA backoff procedure).
 *
 * The backoff procedure for an Access Category is invoked on specific events.
 * This module models the trigger conditions as an explicit predicate so a
 * student can see exactly *when* an EDCAF (EDCA Function) starts a backoff,
 * separate from the contention dynamics modeled in edca_sched.c.
 *
 * The three modeled trigger families (from the evidence):
 *   1. A new MPDU is queued for an AC (MA-UNITDATA.request) under specific
 *      queue/counter/medium conditions.
 *   2. A TXOP completes.
 *   3. An internal collision occurs and this AC is the lower-priority loser.
 */
#ifndef EDCA_TRIGGERS_H
#define EDCA_TRIGGERS_H

#include "mac_common.h"

/* Carrier-sense / medium state inputs (segment 2 evidence). */
typedef struct {
    bool physical_cs_busy;   /* Physical Carrier Sense says busy            */
    bool virtual_cs_busy;    /* Virtual CS (NAV) says busy                  */
    bool txnav_nonzero;      /* non-zero TXNAV timer                        */
    bool rav_nonzero;        /* non-zero RAV (mesh, dot11MCCAActivated)     */
    bool mcca_activated;     /* dot11MCCAActivated for this (mesh) STA      */
} medium_state_t;

/* Is the medium considered busy for backoff-trigger purposes? The RAV term
 * only counts for a mesh STA with MCCA activated. */
static inline bool medium_busy(const medium_state_t *m) {
    bool busy = m->physical_cs_busy || m->virtual_cs_busy || m->txnav_nonzero;
    if (m->mcca_activated) busy = busy || m->rav_nonzero;
    return busy;
}

/* State of one AC's queue relevant to the new-MPDU trigger. */
typedef struct {
    bool     this_ac_now_nonempty; /* the AC queue became non-empty         */
    bool     other_ac_queues_empty;/* all OTHER queues for that AC are empty*/
    uint32_t backoff_counter;      /* current backoff counter for the AC    */
} ac_queue_state_t;

/* Trigger 1: receipt of an MA-UNITDATA.request queuing an MPDU for an AC.
 * Per the evidence, ALL of the following must hold:
 *   - the specific AC queue is now non-empty,
 *   - all other queues for that AC are empty,
 *   - the backoff counter for that AC is 0,
 *   - the medium is busy.
 */
static inline bool edca_trigger_new_mpdu(const ac_queue_state_t *q,
                                         const medium_state_t *m) {
    return q->this_ac_now_nonempty
        && q->other_ac_queues_empty
        && q->backoff_counter == 0
        && medium_busy(m);
}

/* Trigger 2: a TXOP has just completed -> invoke backoff for the next access. */
static inline bool edca_trigger_txop_complete(bool txop_just_completed) {
    return txop_just_completed;
}

/* Trigger 3: internal collision between two EDCAF instances in the same STA;
 * the lower-priority AC must back off. Returns true for the LOSER. */
static inline bool edca_trigger_internal_collision(access_category_t self,
                                                   access_category_t other,
                                                   bool both_fired) {
    /* Higher enum value = higher priority (VO=3 > VI=2 > BE=1 > BK=0). The
     * lower-priority AC is the one that must invoke backoff. */
    return both_fired && self < other;
}

#endif /* EDCA_TRIGGERS_H */
