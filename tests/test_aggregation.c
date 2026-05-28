/* test_aggregation.c - segment 4 (A-MSDU / A-MPDU) */
#include "aggregation.h"
#include "test_util.h"

int main(void) {
    printf("== Frame aggregation ==\n");

    /* A-MPDU max length grows with the exponent: 2^(13+e)-1. */
    CHECK(ampdu_max_len_from_exponent(0) == (1u << 13) - 1,
          "exponent 0 -> 8191 bytes");
    CHECK(ampdu_max_len_from_exponent(3) == (1u << 16) - 1,
          "exponent 3 -> 65535 bytes");
    CHECK(ampdu_max_len_from_exponent(7) > ampdu_max_len_from_exponent(3),
          "higher exponent -> larger max length");

    /* Group-addressed: the MINIMUM exponent among peers governs. */
    uint32_t peers[] = { 7, 3, 5, 2, 6 };
    CHECK(ampdu_group_exponent(peers, 5) == 2,
          "group exponent is the minimum among peers");
    uint32_t one[] = { 4 };
    CHECK(ampdu_group_exponent(one, 1) == 4, "single peer governs itself");

    /* A-MSDU allowed iff within the peer's advertised maximum. */
    CHECK(amsdu_allowed(3000, 3839), "A-MSDU within 3839-byte cap allowed");
    CHECK(!amsdu_allowed(8000, 3839), "oversized A-MSDU rejected");

    /* MPDU spacing scales with PHY rate and MMSF; more padding at higher rate. */
    uint32_t lo = ampdu_min_mpdu_spacing_bytes(1, 1, 100);  /* 100 Mb/s */
    uint32_t hi = ampdu_min_mpdu_spacing_bytes(1, 1, 1000); /* 1 Gb/s   */
    CHECK(hi > lo, "spacing padding grows with PHY rate");
    uint32_t base = ampdu_min_mpdu_spacing_bytes(1, 1, 800);
    uint32_t x2   = ampdu_min_mpdu_spacing_bytes(1, 2, 800);
    CHECK(x2 == 2 * base, "MMSF multiplies the spacing");

    TEST_SUMMARY();
}
