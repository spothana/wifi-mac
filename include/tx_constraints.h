/* tx_constraints.h - TXOP/PPDU duration, TWT, and NSTR limitations.
 *
 * SEGMENT 8 (Managing Transmission Constraints and Timing).
 * SPEC: 802.11-2020 Clause 10 / 802.11ax TWT / 802.11be MLO & NSTR.
 *
 *  - aPPDUMaxTime: an EHT STA must not transmit an EHT PPDU longer than this.
 *  - TWT (Target Wake Time): negotiation modes Request / Suggest / Demand.
 *  - NSTR (Non-Simultaneous Tx/Rx): a STA in an MLD with an NSTR link pair
 *    cannot Tx on one link while Rx on the paired link under given conditions.
 */
#ifndef TX_CONSTRAINTS_H
#define TX_CONSTRAINTS_H

#include "mac_common.h"

/* aPPDUMaxTime for EHT, in microseconds (802.11be default). */
#define A_PPDU_MAX_TIME_US 5484u

/* True if an EHT PPDU of the given duration is permitted. */
static inline bool eht_ppdu_duration_ok(usec_t ppdu_us) {
    return ppdu_us <= A_PPDU_MAX_TIME_US;
}

/* ---- TWT negotiation -------------------------------------------------- */
typedef enum {
    TWT_REQUEST = 0, /* leaves parameters to the responding STA            */
    TWT_SUGGEST,     /* offers preferred parameters, accepts alternatives  */
    TWT_DEMAND,      /* accepts ONLY the indicated parameters              */
} twt_setup_command_t;

/* Given the requester's command and the parameters the responder proposes,
 * decide whether the negotiation succeeds. `params_match` means the responder's
 * proposed parameters equal the requester's indicated ones. */
bool twt_negotiation_succeeds(twt_setup_command_t cmd, bool params_match);

/* ---- NSTR limitation -------------------------------------------------- */
typedef struct {
    bool affiliated_with_mld;    /* STA is affiliated with an MLD           */
    bool mld_has_nstr_pair;      /* the MLD has >=1 NSTR link pair          */
    bool received_rts_on_pair;   /* got an RTS on a link of that pair       */
    bool sibling_holds_txop;     /* another STA in same MLD is TXOP holder/ */
                                 /* responder on the OTHER link of the pair */
} nstr_context_t;

/* A STA is NSTR-limited (must not respond) iff ALL conditions hold
 * (segment 8 evidence). */
bool nstr_limited(const nstr_context_t *c);

#endif /* TX_CONSTRAINTS_H */
