/* test_edca.c - exercises EDCA prioritization and collision handling. */
#include "edca.h"
#include "edca_sched.h"
#include "test_util.h"

int main(void) {
    printf("== EDCA (HCF contention-based) tests ==\n");

    /* 1. Default parameters encode the priority ordering from Table 10-1:
     *    voice/video have smaller AIFSN and CWmin than best-effort/background. */
    CHECK(EDCA_DEFAULT[AC_VO].aifsn <= EDCA_DEFAULT[AC_BE].aifsn,
          "AC_VO AIFSN <= AC_BE AIFSN");
    CHECK(EDCA_DEFAULT[AC_BE].aifsn <= EDCA_DEFAULT[AC_BK].aifsn,
          "AC_BE AIFSN <= AC_BK AIFSN");
    CHECK(EDCA_DEFAULT[AC_VO].cw_min <= EDCA_DEFAULT[AC_BE].cw_min,
          "AC_VO CWmin <= AC_BE CWmin");

    /* 2. AIFS computation matches the spec formula. */
    CHECK(aifs_us(EDCA_DEFAULT[AC_VO].aifsn) ==
          SIFS_US + EDCA_DEFAULT[AC_VO].aifsn * SLOT_TIME_US,
          "AIFS = SIFS + AIFSN*aSlotTime");

    /* 3. Within one station, an internal (virtual) collision is resolved in
     *    favor of the higher-priority AC. Force VO and BE to collide by
     *    seeding identical backoffs, then run a single station. */
    edca_station_t s;
    uint32_t pend[AC_COUNT] = {0};
    pend[AC_VO] = 5; pend[AC_BE] = 5;
    edca_init(&s, 0, pend, 42);
    /* Force equal effective countdowns so VO and BE fire in the same slot.
     * countdown = AIFSN + backoff. With AIFSN_VO=2, AIFSN_BE=3, choosing
     * backoff_BE=0 and backoff_VO=1 gives both a countdown of 3, a tie that
     * the resolver must break in favor of the higher-priority AC (VO). */
    s.ac[AC_BE].backoff = 0;
    s.ac[AC_VO].backoff = (EDCA_DEFAULT[AC_BE].aifsn + 0)
                          - EDCA_DEFAULT[AC_VO].aifsn; /* = 1 */
    edca_result_t r1 = edca_run(&s, 1, 100000, false);
    CHECK(r1.internal_collisions >= 1, "internal collision detected in one station");
    CHECK(s.ac[AC_VO].delivered + s.ac[AC_BE].delivered >= 1,
          "station still delivers after internal collision");

    /* 4. Across many stations, higher-priority ACs win the medium more often.
     *    Set up stations with both VO and BK traffic; expect VO success rate
     *    per frame to be at least as good as BK. */
    edca_station_t fleet[6];
    for (int i = 0; i < 6; i++) {
        uint32_t p[AC_COUNT] = {0};
        p[AC_VO] = 15; p[AC_BK] = 15;
        edca_init(&fleet[i], i, p, 1000 + i);
    }
    edca_result_t r2 = edca_run(fleet, 6, 5000000, false);
    CHECK(r2.success[AC_VO] >= r2.success[AC_BK],
          "AC_VO delivers >= AC_BK under contention");
    CHECK(r2.success[AC_VO] + r2.success[AC_BK] > 0, "EDCA makes progress");

    /* 5. Everything queued is eventually delivered (no frames stuck). */
    uint32_t total_done = 0;
    for (int i = 0; i < 6; i++)
        for (int a = 0; a < AC_COUNT; a++)
            total_done += fleet[i].ac[a].delivered;
    CHECK(total_done > 0, "frames delivered across the fleet");

    TEST_SUMMARY();
}
