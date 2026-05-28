/* pcf.h - Point Coordination Function (802.11-2020 Clause 10.4)
 *
 * The Point Coordinator (PC), co-located with the AP, gains control of the
 * medium after a PIFS (shorter than DIFS, so it wins over DCF stations) and
 * runs a Contention-Free Period (CFP). During the CFP the PC polls stations
 * from its polling list round-robin. Each polled station may send one frame
 * in response to a CF-Poll. The CFP is bounded by CFPMaxDuration and ends with
 * a CF-End. A Contention Period (CP) governed by DCF follows.
 *
 * This module models a single CFP: beacon -> repeated (CF-Poll, response)
 * -> CF-End, accumulating the virtual clock and per-station delivery.
 */
#ifndef PCF_H
#define PCF_H

#include "mac_common.h"

typedef struct {
    int      id;
    uint32_t pending;   /* frames the station has to send when polled    */
    uint32_t delivered; /* frames delivered during the CFP               */
    bool     responds;  /* false models a station with nothing to send   */
} pcf_station_t;

typedef struct {
    usec_t   clock_us;
    uint32_t polls_issued;
    uint32_t null_responses; /* polls that returned no data               */
    uint32_t frames_delivered;
    bool      cfp_truncated;  /* hit CFPMaxDuration before list exhausted  */
} pcf_result_t;

/* Run one Contention-Free Period over the polling list.
 * cfp_max_us bounds the CFP (CFPMaxDuration). verbose prints a trace. */
pcf_result_t pcf_run_cfp(pcf_station_t *list, int n,
                         usec_t cfp_max_us, bool verbose);

#endif /* PCF_H */
