/* test_block_ack.c - segment 5 (Block Ack windows and PBAC) */
#include "block_ack.h"
#include "test_util.h"

static ba_endpoint_t mk(sta_tech_t t, bool mld, bool mfpc, bool pbac) {
    ba_endpoint_t e = { t, mld, mfpc, pbac };
    return e;
}

int main(void) {
    printf("== Block Ack ==\n");

    ba_endpoint_t eht  = mk(STA_EHT, true,  true, true);
    ba_endpoint_t he   = mk(STA_HE,  true,  true, true);
    ba_endpoint_t ht   = mk(STA_HT,  false, false, false);
    ba_endpoint_t nonht= mk(STA_NONHT, false, false, false);
    ba_endpoint_t edmg = mk(STA_EDMG, false, false, false);

    /* Window sizing (least-capable endpoint wins). */
    CHECK(ba_max_window(&eht, &eht) == 1024, "both EHT -> 1024");
    CHECK(ba_max_window(&he, &he)   == 256,  "both HE -> 256");
    CHECK(ba_max_window(&he, &nonht)== 64,   "HE with non-HE -> 64");
    CHECK(ba_max_window(&ht, &ht)   == 64,   "both HT -> 64");
    CHECK(ba_max_window(&edmg, &eht)== 1024, "EDMG/EHT pair -> 1024");
    CHECK(ba_max_window(&eht, &he)  == 256,  "EHT with HE -> 256 (HE caps it)");

    /* MLD-level agreement iff both endpoints are MLD-affiliated. */
    CHECK(ba_agreement_is_mld(&eht, &he), "both MLD -> MLD-level agreement");
    CHECK(!ba_agreement_is_mld(&eht, &ht), "one non-MLD -> STA-level agreement");

    /* PBAC requires MFPC && PBAC on both ends. */
    ba_endpoint_t pbac_ok  = mk(STA_EHT, true, true, true);
    ba_endpoint_t pbac_no  = mk(STA_EHT, true, true, false);
    CHECK(ba_is_pbac_sta(&pbac_ok), "MFPC && PBAC -> PBAC STA");
    CHECK(!ba_is_pbac_sta(&pbac_no), "PBAC bit clear -> not a PBAC STA");
    CHECK(ba_protected_allowed(&pbac_ok, &pbac_ok), "both PBAC -> protected BA allowed");
    CHECK(!ba_protected_allowed(&pbac_ok, &pbac_no), "one non-PBAC -> protected BA denied");

    TEST_SUMMARY();
}
