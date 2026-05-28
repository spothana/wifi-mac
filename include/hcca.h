/* hcca.h - HCF Controlled Channel Access (802.11-2020 Clause 10.22.3)
 *
 * HCCA is the polled, contention-free half of HCF and the QoS-aware successor
 * to PCF. The Hybrid Coordinator (HC), in the AP, grants polled TXOPs using
 * QoS CF-Poll frames. Key differences from PCF:
 *   - The HC can gain control after a PIFS during BOTH the CFP and the CP
 *     (it can start a Controlled Access Phase, CAP, almost any time).
 *   - Polls grant an explicit TXOP DURATION (from the stream's TSPEC), not just
 *     a single frame. A station may send several frames within its granted TXOP.
 *   - Scheduling is driven by per-stream QoS requirements (mean data rate,
 *     delay bound) rather than a flat round-robin.
 *
 * This module models an HC granting each admitted traffic stream a TXOP sized
 * from its TSPEC and tallies delivered frames and medium time.
 */
#ifndef HCCA_H
#define HCCA_H

#include "mac_common.h"

typedef struct {
    int      id;
    /* TSPEC-derived: how much medium time the HC grants per poll, and how
     * many frames the station has buffered to send in granted TXOPs. */
    usec_t   txop_grant_us;
    uint32_t pending;
    uint32_t priority;   /* higher = scheduled earlier (delay bound proxy)  */
    uint32_t delivered;
} hcca_stream_t;

typedef struct {
    usec_t   clock_us;
    uint32_t polls_issued;
    uint32_t frames_delivered;
    usec_t   granted_txop_total_us;
} hcca_result_t;

/* Run one Controlled Access Phase: the HC seizes the medium (PIFS), then polls
 * admitted streams in priority order, granting each a TXOP. cap_max_us bounds
 * the CAP. Streams are polled until drained or the CAP budget is exhausted. */
hcca_result_t hcca_run_cap(hcca_stream_t *streams, int n,
                           usec_t cap_max_us, bool verbose);

#endif /* HCCA_H */
