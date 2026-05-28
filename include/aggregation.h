/* aggregation.h - A-MSDU / A-MPDU aggregation limits and spacing.
 *
 * SEGMENT 4 (Optimizing Throughput via Frame Aggregation).
 * SPEC: 802.11-2020 Clause 9.7 (A-MPDU) / 9.3.2.2 (A-MSDU).
 *
 * Two aggregation methods reduce per-frame overhead:
 *   A-MSDU : multiple MSDUs in one MAC frame; bounded by the Maximum A-MSDU
 *            Length field in the peer's capabilities.
 *   A-MPDU : multiple MPDUs in one PPDU; max length encoded as an EXPONENT in
 *            HT/VHT/HE/EHT (etc.) Capabilities elements.
 *
 * Two rules modeled here:
 *   - For GROUP-addressed data, the usable max A-MPDU length exponent is the
 *     MINIMUM among all associated/peer STAs (so every recipient can cope).
 *   - Minimum spacing between consecutive MPDUs in an A-MPDU is derived from
 *     the Minimum MPDU Start Spacing field, the MPDU MU Spacing Factor (MMSF),
 *     and the PHY data rate.
 */
#ifndef AGGREGATION_H
#define AGGREGATION_H

#include "mac_common.h"

/* A-MPDU maximum length from an exponent field. Different element families use
 * slightly different base formulas; we use the common HT/VHT/HE form:
 *   max_len = 2^(13 + exponent) - 1   (bytes)
 * EHT extends the exponent range; the same shape is used for teaching. */
static inline uint64_t ampdu_max_len_from_exponent(uint32_t exponent) {
    return ((uint64_t)1 << (13 + exponent)) - 1;
}

/* Group-addressed rule: the exponent actually used is the MINIMUM among all
 * recipients, so the aggregate fits the least-capable STA. */
uint32_t ampdu_group_exponent(const uint32_t *peer_exponents, int n_peers);

/* Whether an A-MSDU of `amsdu_len` bytes is allowed given the peer's advertised
 * Maximum A-MSDU Length capability (in bytes). */
static inline bool amsdu_allowed(uint32_t amsdu_len, uint32_t peer_max_amsdu) {
    return amsdu_len <= peer_max_amsdu;
}

/* Minimum MPDU Start Spacing field encodes a base spacing in microseconds-ish
 * units; the effective minimum spacing in BYTES of padding scales with the PHY
 * data rate. We model the teaching relationship:
 *   min_spacing_bytes = ceil( min_start_spacing_us * phy_rate_mbps / 8 ) * MMSF
 * which captures "more padding at higher rates" without per-PPDU PHY detail. */
uint32_t ampdu_min_mpdu_spacing_bytes(uint32_t min_start_spacing_us,
                                      uint32_t mmsf,
                                      uint32_t phy_rate_mbps);

#endif /* AGGREGATION_H */
