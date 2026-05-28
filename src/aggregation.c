/* aggregation.c - see aggregation.h */
#include "aggregation.h"

uint32_t ampdu_group_exponent(const uint32_t *peer_exponents, int n_peers) {
    if (n_peers <= 0) return 0;
    uint32_t min_e = peer_exponents[0];
    for (int i = 1; i < n_peers; i++)
        if (peer_exponents[i] < min_e) min_e = peer_exponents[i];
    return min_e; /* least-capable recipient governs (segment 4 evidence). */
}

uint32_t ampdu_min_mpdu_spacing_bytes(uint32_t min_start_spacing_us,
                                      uint32_t mmsf,
                                      uint32_t phy_rate_mbps) {
    if (mmsf == 0) mmsf = 1;
    /* bytes of payload transmitted during the start-spacing interval at the
     * given PHY rate: us * Mbit/s / 8 = us * Mbyte/s. Round up. */
    uint64_t bytes = ((uint64_t)min_start_spacing_us * phy_rate_mbps + 7) / 8;
    return (uint32_t)(bytes * mmsf);
}
