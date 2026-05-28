/* edca_sched.h - scheduler for EDCA contention across stations and ACs.
 *
 * Each backlogged AC has an "effective countdown" = AIFSN[AC] + backoff[AC]
 * (in slot units, relative to the end of the previous busy medium). The AC(s)
 * with the smallest countdown reach the medium first. Among same-station ACs
 * tied at the minimum, the higher-priority AC wins and the others take an
 * internal (virtual) collision. Across stations, ties are external collisions.
 */
#ifndef EDCA_SCHED_H
#define EDCA_SCHED_H

#include "edca.h"

typedef struct {
    usec_t   clock_us;
    uint32_t success[AC_COUNT];
    uint32_t external_collisions;
    uint32_t internal_collisions;
    uint32_t rounds;
} edca_result_t;

edca_result_t edca_run(edca_station_t *stations, int n,
                       uint32_t max_rounds, bool verbose);

#endif /* EDCA_SCHED_H */
