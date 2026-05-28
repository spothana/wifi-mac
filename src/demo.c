/* demo.c - runs all four coordination functions with verbose traces so a
 * student can watch each mechanism behave and compare them on one screen. */
#include "dcf.h"
#include "dcf_sched.h"
#include "pcf.h"
#include "edca.h"
#include "edca_sched.h"
#include "hcca.h"
#include "rts_cts.h"
#include "aggregation.h"
#include "block_ack.h"
#include "seq_num.h"
#include "rate_control.h"
#include "tx_constraints.h"
#include <stdio.h>
#include <string.h>

static void rule(const char *t) {
    printf("\n========================================================\n");
    printf(" %s\n", t);
    printf("========================================================\n");
}

int main(void) {
    rule("DCF - Distributed Coordination Function (CSMA/CA), 4 stations");
    dcf_station_t d[4];
    for (int i = 0; i < 4; i++) dcf_init(&d[i], i, 3, 7 + i * 13);
    dcf_result_t dr = dcf_run(d, 4, 100000, true);
    printf("  -> %u successes, %u collisions, clock=%llu us\n",
           dr.total_success, dr.total_collisions,
           (unsigned long long)dr.clock_us);

    rule("PCF - Point Coordination Function (polled CFP), 4 stations");
    pcf_station_t p[4] = {
        { .id = 0, .pending = 2, .responds = true  },
        { .id = 1, .pending = 1, .responds = false },
        { .id = 2, .pending = 3, .responds = true  },
        { .id = 3, .pending = 1, .responds = true  },
    };
    pcf_result_t pr = pcf_run_cfp(p, 4, 1000000, true);
    printf("  -> %u frames, %u polls, %u nulls, clock=%llu us\n",
           pr.frames_delivered, pr.polls_issued, pr.null_responses,
           (unsigned long long)pr.clock_us);

    rule("EDCA - HCF contention-based, prioritized ACs, 3 stations");
    edca_station_t e[3];
    for (int i = 0; i < 3; i++) {
        uint32_t pend[AC_COUNT] = {0};
        pend[AC_VO] = 2; pend[AC_BE] = 2; pend[AC_BK] = 2;
        edca_init(&e[i], i, pend, 500 + i * 17);
    }
    edca_result_t er = edca_run(e, 3, 1000000, true);
    printf("  -> VO=%u VI=%u BE=%u BK=%u | ext-coll=%u int-coll=%u clock=%llu us\n",
           er.success[AC_VO], er.success[AC_VI], er.success[AC_BE],
           er.success[AC_BK], er.external_collisions, er.internal_collisions,
           (unsigned long long)er.clock_us);

    rule("HCCA - HCF controlled-access, TXOP grants by priority, 3 streams");
    usec_t one = DATA_TX_US + SIFS_US + ACK_TX_US;
    hcca_stream_t h[3] = {
        { .id = 0, .txop_grant_us = one * 2, .pending = 4, .priority = 2 },
        { .id = 1, .txop_grant_us = one * 2, .pending = 2, .priority = 8 },
        { .id = 2, .txop_grant_us = one * 2, .pending = 3, .priority = 5 },
    };
    hcca_result_t hr = hcca_run_cap(h, 3, 1000000, true);
    printf("  -> %u frames, %u polls, granted-TXOP=%llu us, clock=%llu us\n",
           hr.frames_delivered, hr.polls_issued,
           (unsigned long long)hr.granted_txop_total_us,
           (unsigned long long)hr.clock_us);

    /* ---- Segment 3: RTS/CTS dynamic bandwidth fallback ---- */
    rule("RTS/CTS - dynamic 160 MHz with upper 80 MHz busy");
    rts_request_t rts = { CHW_160, BW_DYNAMIC, true, true };
    subchannel_map_t sc; memset(&sc, 0, sizeof sc);
    sc.n_subch = 8;
    for (int i = 0; i < 8; i++) sc.cca_idle[i] = (i < 4); /* upper half busy */
    cts_response_t cts = cts_compute(&rts, &sc, true);
    printf("  requested=160 MHz, mode=dynamic -> CTS valid=%d width=%d MHz (%s)\n",
           cts.valid, (int)cts.granted_width, cts.reason);

    /* ---- Segment 4: A-MPDU group exponent + spacing ---- */
    rule("Aggregation - group A-MPDU exponent & MPDU spacing");
    uint32_t exps[] = { 7, 3, 5, 6 };
    uint32_t ge = ampdu_group_exponent(exps, 4);
    printf("  peer exponents {7,3,5,6} -> group exponent=%u, max len=%llu bytes\n",
           ge, (unsigned long long)ampdu_max_len_from_exponent(ge));
    printf("  min MPDU spacing @ 100 Mb/s=%u B, @ 1000 Mb/s=%u B (MMSF=2)\n",
           ampdu_min_mpdu_spacing_bytes(1, 2, 100),
           ampdu_min_mpdu_spacing_bytes(1, 2, 1000));

    /* ---- Segment 5: Block Ack window by STA type ---- */
    rule("Block Ack - transmission window by endpoint capability");
    ba_endpoint_t eht = { STA_EHT, true, true, true };
    ba_endpoint_t he  = { STA_HE,  true, true, true };
    ba_endpoint_t leg = { STA_NONHT, false, false, false };
    printf("  EHT<->EHT window=%u | EHT<->HE window=%u | HE<->legacy window=%u\n",
           ba_max_window(&eht, &eht), ba_max_window(&eht, &he),
           ba_max_window(&he, &leg));
    printf("  protected BA EHT<->EHT allowed=%d\n",
           ba_protected_allowed(&eht, &eht));

    /* ---- Segment 6: duplicate detection ---- */
    rule("Sequence numbers - duplicate detection cache");
    dup_cache_t dc; dup_cache_init(&dc);
    uint64_t addr = 0x001122334455ull;
    printf("  first frame (seq=10,retry=0) duplicate? %d\n",
           dup_cache_is_duplicate(&dc, addr, 0, 10, 0, false));
    printf("  retransmit (seq=10,retry=1) duplicate? %d\n",
           dup_cache_is_duplicate(&dc, addr, 0, 10, 0, true));

    /* ---- Segment 7: non-HT basic rate conversion ---- */
    rule("Rate control - non-HT basic rate conversion");
    uint32_t basic[] = { 6, 12, 24 };
    uint32_t ref = nonht_reference_rate(4);
    printf("  MCS 4 -> reference %u Mb/s -> response basic rate %u Mb/s\n",
           ref, basic_rate_at_or_below(ref, basic, 3));

    /* ---- Segment 8: TWT + NSTR ---- */
    rule("TX constraints - TWT modes and NSTR limitation");
    printf("  TWT Demand (params match)=%d, Demand (mismatch)=%d, Request=%d\n",
           twt_negotiation_succeeds(TWT_DEMAND, true),
           twt_negotiation_succeeds(TWT_DEMAND, false),
           twt_negotiation_succeeds(TWT_REQUEST, false));
    nstr_context_t nstr = { true, true, true, true };
    printf("  NSTR-limited (all conditions met)=%d ; EHT PPDU 6000us ok=%d\n",
           nstr_limited(&nstr), eht_ppdu_duration_ok(6000));

    return 0;
}
