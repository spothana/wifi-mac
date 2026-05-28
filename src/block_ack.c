/* block_ack.c - see block_ack.h */
#include "block_ack.h"

/* Per-endpoint maximum BA window implied by that endpoint's own technology.
 * The pairwise rule "least capable wins" is then just the minimum of the two.
 *   EHT / EDMG : 1024
 *   HE         : 256
 *   VHT/HT/DMG/non-HT : 64
 * This reproduces every row of the evidence table:
 *   both EHT -> min(1024,1024)=1024 ; both HE -> 256 ; HE+non-HE -> 64 ;
 *   EDMG/EHT -> 1024 ; EHT+HE -> min(1024,256)=256 (HE caps it). */
static uint32_t endpoint_cap(sta_tech_t t) {
    switch (t) {
        case STA_EHT:
        case STA_EDMG: return 1024;
        case STA_HE:   return 256;
        case STA_VHT:
        case STA_HT:
        case STA_DMG:
        case STA_NONHT:
        default:       return 64;
    }
}

uint32_t ba_max_window(const ba_endpoint_t *a, const ba_endpoint_t *b) {
    uint32_t ca = endpoint_cap(a->tech), cb = endpoint_cap(b->tech);
    return ca < cb ? ca : cb; /* least-capable endpoint governs */
}

bool ba_protected_allowed(const ba_endpoint_t *a, const ba_endpoint_t *b) {
    return ba_is_pbac_sta(a) && ba_is_pbac_sta(b);
}
