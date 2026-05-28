/* dcf_sched.h - slotted CSMA/CA scheduler that runs a set of DCF stations.
 *
 * Teaching model: time advances in backoff slots. In each contention round
 * every backlogged station counts its backoff down to zero; the station(s)
 * reaching zero in the same slot transmit. One transmitter -> success after
 * SIFS+ACK; two or more -> collision. The virtual clock accumulates the real
 * us cost (DIFS, slots, DATA, SIFS, ACK) so throughput numbers are meaningful.
 */
#ifndef DCF_SCHED_H
#define DCF_SCHED_H

#include "dcf.h"

typedef struct {
    usec_t   clock_us;
    uint32_t total_success;
    uint32_t total_collisions;
    uint32_t rounds;
} dcf_result_t;

/* Run until every station reaches DCF_DONE or max_rounds is hit.
 * Set verbose=true to print a slot-by-slot trace for classroom use. */
dcf_result_t dcf_run(dcf_station_t *stations, int n,
                     uint32_t max_rounds, bool verbose);

#endif /* DCF_SCHED_H */
