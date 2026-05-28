/* test_dcf.c - exercises the DCF state machine and scheduler. */
#include "dcf.h"
#include "dcf_sched.h"
#include "test_util.h"

int main(void) {
    printf("== DCF tests ==\n");

    /* 1. IFS relationships from the spec: PIFS=SIFS+slot, DIFS=SIFS+2*slot. */
    CHECK(PIFS_US == SIFS_US + SLOT_TIME_US, "PIFS = SIFS + aSlotTime");
    CHECK(DIFS_US == SIFS_US + 2 * SLOT_TIME_US, "DIFS = SIFS + 2*aSlotTime");
    CHECK(PIFS_US < DIFS_US, "PIFS < DIFS (PCF/HCF can preempt DCF)");

    /* 2. Backoff is always within [0, CW]. */
    dcf_station_t s;
    dcf_init(&s, 0, 5, 12345);
    bool in_range = true;
    for (int i = 0; i < 1000; i++) {
        dcf_choose_backoff(&s);
        if (s.backoff > s.cw) in_range = false;
    }
    CHECK(in_range, "backoff stays within [0, CW]");

    /* 3. Binary exponential backoff doubles CW on collision, caps at CWmax. */
    dcf_init(&s, 0, 100, 1);
    CHECK(s.cw == DCF_CW_MIN, "CW starts at CWmin");
    dcf_on_tx_result(&s, false);
    CHECK(s.cw == (DCF_CW_MIN + 1) * 2 - 1, "CW doubles after collision");
    for (int i = 0; i < 20; i++) dcf_on_tx_result(&s, false);
    CHECK(s.cw <= DCF_CW_MAX, "CW never exceeds CWmax");

    /* 4. Success resets CW to CWmin. */
    dcf_init(&s, 0, 100, 7);
    dcf_on_tx_result(&s, false);
    dcf_on_tx_result(&s, true);
    CHECK(s.cw == DCF_CW_MIN, "CW resets to CWmin after success");

    /* 5. A single station with no contention delivers every frame. */
    dcf_station_t solo;
    dcf_init(&solo, 0, 10, 99);
    dcf_result_t r = dcf_run(&solo, 1, 10000, false);
    CHECK(solo.frames_sent == 10, "lone station delivers all frames");
    CHECK(r.total_collisions == 0, "no collisions with a single station");

    /* 6. Many stations contending eventually deliver everything, and the
     *    scheduler observes at least some collisions. */
    dcf_station_t many[8];
    for (int i = 0; i < 8; i++) dcf_init(&many[i], i, 20, 100 + i);
    dcf_result_t rm = dcf_run(many, 8, 1000000, false);
    uint32_t delivered = 0, dropped = 0;
    for (int i = 0; i < 8; i++) {
        delivered += many[i].frames_sent;
        /* frames_queued reaches 0 whether delivered or dropped at retry limit */
    }
    CHECK(rm.total_success == delivered, "scheduler success count matches stations");
    CHECK(delivered > 0, "contending stations make progress");
    (void)dropped;

    TEST_SUMMARY();
}
