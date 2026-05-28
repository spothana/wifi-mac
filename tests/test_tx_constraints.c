/* test_tx_constraints.c - segment 8 (PPDU duration, TWT, NSTR) */
#include "tx_constraints.h"
#include "test_util.h"

int main(void) {
    printf("== Transmission constraints ==\n");

    /* aPPDUMaxTime bound on EHT PPDUs. */
    CHECK(eht_ppdu_duration_ok(A_PPDU_MAX_TIME_US), "PPDU exactly at limit is OK");
    CHECK(eht_ppdu_duration_ok(1000), "short PPDU OK");
    CHECK(!eht_ppdu_duration_ok(A_PPDU_MAX_TIME_US + 1), "over-limit PPDU rejected");

    /* TWT negotiation modes. */
    CHECK(twt_negotiation_succeeds(TWT_REQUEST, false),
          "Request mode resolves regardless of parameter match");
    CHECK(twt_negotiation_succeeds(TWT_SUGGEST, false),
          "Suggest mode accepts alternatives");
    CHECK(twt_negotiation_succeeds(TWT_DEMAND, true),
          "Demand mode succeeds only when parameters match");
    CHECK(!twt_negotiation_succeeds(TWT_DEMAND, false),
          "Demand mode fails when parameters don't match");

    /* NSTR: limited only when ALL conditions hold. */
    nstr_context_t all = { true, true, true, true };
    CHECK(nstr_limited(&all), "all conditions -> NSTR-limited");

    nstr_context_t no_rts = { true, true, false, true };
    CHECK(!nstr_limited(&no_rts), "no RTS on the pair -> not limited");

    nstr_context_t no_sibling = { true, true, true, false };
    CHECK(!nstr_limited(&no_sibling), "no sibling TXOP holder -> not limited");

    nstr_context_t no_mld = { false, true, true, true };
    CHECK(!nstr_limited(&no_mld), "not MLD-affiliated -> not limited");

    nstr_context_t no_pair = { true, false, true, true };
    CHECK(!nstr_limited(&no_pair), "MLD without NSTR pair -> not limited");

    TEST_SUMMARY();
}
