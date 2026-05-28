/* test_hcca.c - exercises HCCA controlled access and TXOP grants. */
#include "hcca.h"
#include "test_util.h"

int main(void) {
    printf("== HCCA (HCF controlled-access) tests ==\n");

    usec_t one = DATA_TX_US + SIFS_US + ACK_TX_US;

    /* 1. With a generous CAP, the HC drains every stream's queue. */
    hcca_stream_t s[3] = {
        { .id = 0, .txop_grant_us = one * 4, .pending = 6, .priority = 2 },
        { .id = 1, .txop_grant_us = one * 4, .pending = 3, .priority = 5 },
        { .id = 2, .txop_grant_us = one * 4, .pending = 9, .priority = 1 },
    };
    hcca_result_t r = hcca_run_cap(s, 3, 100000000, false);
    CHECK(r.frames_delivered == 18, "HCCA delivers all buffered frames");

    /* 2. HCCA is contention-free: delivery uses polls, never collisions. The
     *    result struct has no collision counter by construction. */
    CHECK(r.polls_issued >= 1, "HC issued at least one poll");

    /* 3. Higher-priority streams are polled first. After sorting, the first
     *    stream the HC serves should be the highest priority (id 1, prio 5). */
    hcca_stream_t order[3] = {
        { .id = 0, .txop_grant_us = one, .pending = 1, .priority = 1 },
        { .id = 1, .txop_grant_us = one, .pending = 1, .priority = 9 },
        { .id = 2, .txop_grant_us = one, .pending = 1, .priority = 4 },
    };
    hcca_run_cap(order, 3, 100000000, false);
    CHECK(order[0].id == 1, "highest-priority stream scheduled first");
    CHECK(order[0].priority == 9, "ordering is by descending priority");

    /* 4. A TXOP grant bounds how many frames a station sends per poll. With a
     *    one-frame grant but many pending, multiple polls are needed. */
    hcca_stream_t tight[1] = {
        { .id = 0, .txop_grant_us = one, .pending = 5, .priority = 1 },
    };
    hcca_result_t rt = hcca_run_cap(tight, 1, 100000000, false);
    CHECK(rt.frames_delivered == 5, "all frames delivered across multiple polls");
    CHECK(rt.polls_issued >= 5, "small TXOP forces one poll per frame");

    /* 5. CAP budget bounds total medium time. A tiny CAP delivers fewer frames. */
    hcca_stream_t bigq[1] = {
        { .id = 0, .txop_grant_us = one * 100, .pending = 100, .priority = 1 },
    };
    hcca_result_t rb = hcca_run_cap(bigq, 1, 1500 /* us */, false);
    CHECK(rb.frames_delivered < 100, "small CAP budget limits delivery");

    /* 6. HCCA, like PCF, preempts DCF via PIFS. */
    CHECK(PIFS_US < DIFS_US, "HC's PIFS beats DCF's DIFS");

    TEST_SUMMARY();
}
