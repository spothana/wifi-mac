/* test_rate_control.c - segment 7 (multi-recipient rate + non-HT conversion) */
#include "rate_control.h"
#include "test_util.h"

int main(void) {
    printf("== Rate control ==\n");

    /* Multi-recipient: choose the highest (MCS,NSS) supported by ALL. */
    mcs_tuple_t a[] = { {0,1,6}, {2,1,18}, {4,2,72} };
    mcs_tuple_t b[] = { {0,1,6}, {2,1,18} };           /* lacks {4,2}      */
    mcs_tuple_t c[] = { {2,1,18}, {0,1,6}, {7,2,200} };
    recipient_caps_t recips[] = {
        { a, 3 }, { b, 2 }, { c, 3 },
    };
    mcs_tuple_t chosen;
    bool ok = rate_common_to_all(recips, 3, &chosen);
    CHECK(ok, "a common tuple exists");
    CHECK(chosen.mcs == 2 && chosen.nss == 1,
          "common highest tuple is the one all three support");

    /* No common tuple -> failure. */
    mcs_tuple_t d[] = { {9,4,500} };
    recipient_caps_t none[] = { { a, 3 }, { d, 1 } };
    mcs_tuple_t dummy;
    CHECK(!rate_common_to_all(none, 2, &dummy), "no common tuple -> false");

    /* Non-HT reference rate mapping is monotonic-ish and bounded. */
    CHECK(nonht_reference_rate(0) == 6, "MCS 0 -> 6 Mb/s reference");
    CHECK(nonht_reference_rate(6) == 54, "high MCS saturates at 54 Mb/s");
    CHECK(nonht_reference_rate(100) == 54, "out-of-range MCS clamps to 54");

    /* Basic-rate selection: highest basic rate <= the reference. */
    uint32_t basic[] = { 6, 12, 24 };
    CHECK(basic_rate_at_or_below(18, basic, 3) == 12,
          "picks 12 (<=18, highest qualifying)");
    CHECK(basic_rate_at_or_below(54, basic, 3) == 24,
          "picks 24 when all qualify");
    CHECK(basic_rate_at_or_below(5, basic, 3) == 0,
          "none qualify when reference below all basic rates");

    TEST_SUMMARY();
}
