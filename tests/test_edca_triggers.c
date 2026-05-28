/* test_edca_triggers.c - segment 1 (taxonomy) + segment 2 (backoff triggers) */
#include "mac_arch.h"
#include "edca_triggers.h"
#include "test_util.h"

int main(void) {
    printf("== MAC architecture + EDCA backoff triggers ==\n");

    /* Segment 1: the coordination-function taxonomy is well-formed. */
    CHECK(CF_TABLE[CF_DCF].contention_based && !CF_TABLE[CF_DCF].qos_aware,
          "DCF is contention-based, not QoS-aware");
    CHECK(CF_TABLE[CF_EDCA].contention_based && CF_TABLE[CF_EDCA].qos_aware,
          "EDCA is contention-based and QoS-aware");
    CHECK(!CF_TABLE[CF_HCCA].contention_based && CF_TABLE[CF_HCCA].qos_aware,
          "HCCA is polled and QoS-aware");
    CHECK(!CF_TABLE[CF_PCF].contention_based && !CF_TABLE[CF_PCF].qos_aware,
          "PCF is polled, not QoS-aware");

    /* Segment 2, medium_busy: any of PHY CS / virtual CS / TXNAV -> busy. */
    medium_state_t m = {0};
    CHECK(!medium_busy(&m), "all-clear medium is idle");
    m.physical_cs_busy = true;
    CHECK(medium_busy(&m), "physical CS busy -> medium busy");
    m = (medium_state_t){0}; m.txnav_nonzero = true;
    CHECK(medium_busy(&m), "non-zero TXNAV -> medium busy");

    /* RAV only counts for a mesh STA with MCCA activated. */
    m = (medium_state_t){0}; m.rav_nonzero = true; m.mcca_activated = false;
    CHECK(!medium_busy(&m), "RAV ignored when MCCA not activated");
    m.mcca_activated = true;
    CHECK(medium_busy(&m), "RAV counts when MCCA activated");

    /* Trigger 1: new MPDU queued -- ALL four conditions required. */
    medium_state_t busy = { .physical_cs_busy = true };
    ac_queue_state_t q = { .this_ac_now_nonempty = true,
                           .other_ac_queues_empty = true,
                           .backoff_counter = 0 };
    CHECK(edca_trigger_new_mpdu(&q, &busy), "new-MPDU trigger fires when all hold");

    q.backoff_counter = 3;
    CHECK(!edca_trigger_new_mpdu(&q, &busy), "no trigger when backoff counter != 0");
    q.backoff_counter = 0; q.other_ac_queues_empty = false;
    CHECK(!edca_trigger_new_mpdu(&q, &busy), "no trigger when other AC queues non-empty");
    q.other_ac_queues_empty = true;
    medium_state_t idle = {0};
    CHECK(!edca_trigger_new_mpdu(&q, &idle), "no trigger when medium idle");

    /* Trigger 2: TXOP completion. */
    CHECK(edca_trigger_txop_complete(true), "TXOP completion triggers backoff");
    CHECK(!edca_trigger_txop_complete(false), "no trigger without TXOP completion");

    /* Trigger 3: internal collision -- the LOWER-priority AC backs off. */
    CHECK(edca_trigger_internal_collision(AC_BE, AC_VO, true),
          "lower-priority AC_BE backs off vs AC_VO");
    CHECK(!edca_trigger_internal_collision(AC_VO, AC_BE, true),
          "higher-priority AC_VO does not back off vs AC_BE");
    CHECK(!edca_trigger_internal_collision(AC_BE, AC_VO, false),
          "no internal-collision trigger if they didn't both fire");

    TEST_SUMMARY();
}
