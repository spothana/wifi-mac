/* block_ack.h - Block Ack agreement window sizing and PBAC.
 *
 * SEGMENT 5 (Efficient Data Recovery with Block Acknowledgments).
 * SPEC: 802.11-2020 Clause 10.25 (Block Ack) / 802.11be Clause for EHT.
 *
 * Block Ack confirms many MPDUs at once. The transmission window (WinSizeO)
 * is capped by the *least* capable endpoint's technology:
 *   Non-HE STA (either side)  : <= 64
 *   Both HE STAs              : <= 256
 *   Both EHT STAs             : <= 1024
 *   HT or DMG (non-EDMG)      : <= 64
 *   EDMG or EHT               : <= 1024
 * Agreements bind MLDs when the association is MLD<->MLD, else STA<->STA.
 * A "Protected Block Ack" (PBAC) agreement requires both STAs to be PBAC STAs
 * (MFPC in RSN Capabilities AND PBAC in Extended RSN Capabilities both = 1).
 */
#ifndef BLOCK_ACK_H
#define BLOCK_ACK_H

#include "mac_common.h"

/* Technology class, ordered roughly by capability for window sizing. */
typedef enum {
    STA_NONHT = 0, /* legacy / non-HT                          */
    STA_HT,        /* 802.11n                                  */
    STA_DMG,       /* 802.11ad (non-EDMG)                      */
    STA_VHT,       /* 802.11ac                                 */
    STA_HE,        /* 802.11ax                                 */
    STA_EDMG,      /* 802.11ay                                 */
    STA_EHT,       /* 802.11be                                 */
} sta_tech_t;

typedef struct {
    sta_tech_t tech;
    bool       is_mld;        /* affiliated with a Multi-Link Device      */
    bool       mfpc;          /* MFPC bit in RSN Capabilities             */
    bool       pbac;          /* PBAC bit in Extended RSN Capabilities    */
} ba_endpoint_t;

/* Maximum Block Ack transmission window for a pair of endpoints. Applies the
 * "least capable wins" rule from the evidence. */
uint32_t ba_max_window(const ba_endpoint_t *a, const ba_endpoint_t *b);

/* Is an endpoint a PBAC STA? (MFPC && PBAC). */
static inline bool ba_is_pbac_sta(const ba_endpoint_t *e) {
    return e->mfpc && e->pbac;
}

/* Can a Protected Block Ack agreement be established? Both must be PBAC STAs. */
bool ba_protected_allowed(const ba_endpoint_t *a, const ba_endpoint_t *b);

/* Does the agreement bind MLDs (true) or individual STAs (false)? Per the
 * evidence: MLD-level iff BOTH endpoints are MLD-affiliated. */
static inline bool ba_agreement_is_mld(const ba_endpoint_t *a,
                                       const ba_endpoint_t *b) {
    return a->is_mld && b->is_mld;
}

#endif /* BLOCK_ACK_H */
