/* test_rts_cts.c - segment 3 (bandwidth signaling and RTS/CTS) */
#include "rts_cts.h"
#include "test_util.h"
#include <string.h>

static subchannel_map_t all_idle(int n) {
    subchannel_map_t s; memset(&s, 0, sizeof s);
    s.n_subch = n;
    for (int i = 0; i < n; i++) s.cca_idle[i] = true;
    return s;
}

int main(void) {
    printf("== RTS/CTS bandwidth signaling ==\n");

    /* Validity: needs bandwidth-signaling TA and matching TXVECTOR widths. */
    rts_request_t good = { CHW_160, BW_DYNAMIC, true, true };
    CHECK(rts_is_valid_bw_signaling(&good), "valid RTS: TA set + widths match");
    rts_request_t bad_ta = { CHW_160, BW_DYNAMIC, false, true };
    CHECK(!rts_is_valid_bw_signaling(&bad_ta), "invalid: TA not bandwidth-signaling");
    rts_request_t bad_w = { CHW_160, BW_DYNAMIC, true, false };
    CHECK(!rts_is_valid_bw_signaling(&bad_w), "invalid: TXVECTOR widths differ");

    /* Static, full width clear -> CTS echoes full width. */
    rts_request_t st = { CHW_160, BW_STATIC, true, true };
    subchannel_map_t clear8 = all_idle(8); /* 160 MHz = 8 x 20 */
    cts_response_t r = cts_compute(&st, &clear8, true);
    CHECK(r.valid && r.granted_width == CHW_160, "static: full 160 MHz CTS");

    /* Static, a non-primary subchannel busy -> no CTS (static needs full). */
    subchannel_map_t st_busy = all_idle(8); st_busy.cca_idle[5] = false;
    cts_response_t r2 = cts_compute(&st, &st_busy, true);
    CHECK(!r2.valid, "static: partial clear -> no CTS");

    /* Dynamic, top half busy -> CTS falls back to a clear narrower width. */
    rts_request_t dyn = { CHW_160, BW_DYNAMIC, true, true };
    subchannel_map_t half = all_idle(8);
    for (int i = 4; i < 8; i++) half.cca_idle[i] = false; /* upper 80 busy */
    cts_response_t r3 = cts_compute(&dyn, &half, true);
    CHECK(r3.valid && r3.granted_width == CHW_80,
          "dynamic: falls back to clear 80 MHz");

    /* Dynamic, primary 20 busy -> no CTS at all. */
    subchannel_map_t prim_busy = all_idle(8); prim_busy.cca_idle[0] = false;
    cts_response_t r4 = cts_compute(&dyn, &prim_busy, true);
    CHECK(!r4.valid, "dynamic: primary busy -> no CTS");

    /* Punctured non-primary subchannel is skipped for CCA. */
    subchannel_map_t punct = all_idle(8);
    punct.cca_idle[6] = false; punct.punctured[6] = true; /* busy but punctured */
    cts_response_t r5 = cts_compute(&dyn, &punct, true);
    CHECK(r5.valid && r5.granted_width == CHW_160,
          "dynamic: punctured subchannel ignored for CCA");

    /* NAV busy -> responder is silent regardless. */
    cts_response_t r6 = cts_compute(&dyn, &clear8, false);
    CHECK(!r6.valid, "NAV busy -> no CTS");

    TEST_SUMMARY();
}
