/* test_pcf.c - exercises the PCF contention-free period. */
#include "pcf.h"
#include "test_util.h"

int main(void) {
    printf("== PCF tests ==\n");

    /* 1. A CFP with all stations responding delivers everything, no collisions
     *    possible because access is poll-driven. */
    pcf_station_t list[4] = {
        { .id = 0, .pending = 3, .responds = true },
        { .id = 1, .pending = 2, .responds = true },
        { .id = 2, .pending = 4, .responds = true },
        { .id = 3, .pending = 1, .responds = true },
    };
    pcf_result_t r = pcf_run_cfp(list, 4, 1000000, false);
    CHECK(r.frames_delivered == 10, "PCF delivers all polled frames");
    CHECK(!r.cfp_truncated, "CFP completes within a generous budget");
    CHECK(r.null_responses == 0, "no null responses when all stations respond");

    /* 2. Polling is contention-free: every delivered frame cost exactly one
     *    poll, so polls_issued >= frames_delivered. */
    CHECK(r.polls_issued >= r.frames_delivered, "each frame followed a poll");

    /* 3. A non-responding station yields a null response, not a hang. */
    pcf_station_t mixed[2] = {
        { .id = 0, .pending = 1, .responds = true  },
        { .id = 1, .pending = 1, .responds = false }, /* polled but silent */
    };
    pcf_result_t rm = pcf_run_cfp(mixed, 2, 1000000, false);
    CHECK(rm.null_responses >= 1, "silent station produces a null response");
    CHECK(rm.frames_delivered == 1, "only the responding station delivers");

    /* 4. CFPMaxDuration truncates a long CFP. With a tiny budget, not all
     *    frames fit. */
    pcf_station_t big[3] = {
        { .id = 0, .pending = 50, .responds = true },
        { .id = 1, .pending = 50, .responds = true },
        { .id = 2, .pending = 50, .responds = true },
    };
    pcf_result_t rb = pcf_run_cfp(big, 3, 2000 /* us */, false);
    CHECK(rb.cfp_truncated, "small CFPMaxDuration truncates the CFP");
    CHECK(rb.frames_delivered < 150, "truncated CFP delivers fewer frames");

    /* 5. The CFP starts after a PIFS (shorter than DIFS) so the PC preempts
     *    DCF stations -- a structural property we assert on the constants. */
    CHECK(PIFS_US < DIFS_US, "PC's PIFS beats DCF's DIFS");

    TEST_SUMMARY();
}
