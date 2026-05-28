/* mac_common.h
 *
 * Shared types and timing constants for the 802.11 MAC coordination-function
 * teaching simulator.
 *
 * SPEC CROSS-REFERENCE (IEEE Std 802.11-2020):
 *   - Clause 10.3   : DCF (Distributed Coordination Function)
 *   - Clause 10.4   : PCF (Point Coordination Function)
 *   - Clause 10.22  : HCF (Hybrid Coordination Function) -> EDCA + HCCA
 *   - Clause 10.3.2.3 : Interframe space (IFS) relationships
 *
 * All times in this simulator are expressed in MICROSECONDS (us) as a 64-bit
 * integer "virtual clock". We use OFDM PHY (Clause 17) default characteristics
 * roughly matching 802.11a/g so the numbers line up with the textbook tables.
 */
#ifndef MAC_COMMON_H
#define MAC_COMMON_H

#include <stdint.h>
#include <stdbool.h>

typedef uint64_t usec_t; /* virtual-clock microseconds */

/* ------------------------------------------------------------------ *
 * PHY / slot timing (OFDM, 802.11a/g defaults, 802.11-2020 Table 17-21)
 * ------------------------------------------------------------------ */
#define SLOT_TIME_US      9u    /* aSlotTime                              */
#define SIFS_US           16u   /* aSIFSTime                              */
/* PIFS = SIFS + aSlotTime ; DIFS = SIFS + 2*aSlotTime  (10.3.2.3)        */
#define PIFS_US           (SIFS_US + SLOT_TIME_US)
#define DIFS_US           (SIFS_US + 2u * SLOT_TIME_US)
/* EIFS = SIFS + DIFS + ACK_TX_TIME (used after a frame received in error) */

/* ------------------------------------------------------------------ *
 * DCF contention window bounds (Clause 9.4.2.27 default EDCA-less DCF)
 * For pure DCF the PHY characteristic table gives aCWmin/aCWmax.
 * ------------------------------------------------------------------ */
#define DCF_CW_MIN        15u   /* aCWmin (OFDM)                          */
#define DCF_CW_MAX        1023u /* aCWmax (OFDM)                          */
#define MAX_RETRY         7u    /* dot11ShortRetryLimit (illustrative)    */

/* ------------------------------------------------------------------ *
 * Frame durations (fixed, for teaching clarity rather than per-rate calc)
 * ------------------------------------------------------------------ */
#define ACK_TX_US         28u   /* time to transmit an ACK/CF-ACK frame   */
#define DATA_TX_US        200u  /* time to transmit one data MPDU         */
#define CTS_TX_US         28u
#define RTS_TX_US         28u
#define POLL_TX_US        32u   /* CF-Poll / QoS CF-Poll                  */
#define BEACON_TX_US      50u

/* ------------------------------------------------------------------ *
 * EDCA Access Categories (Clause 10.22.2.2, Table 10-1 default params)
 * ------------------------------------------------------------------ */
typedef enum {
    AC_BK = 0, /* Background  */
    AC_BE,     /* Best Effort */
    AC_VI,     /* Video       */
    AC_VO,     /* Voice       */
    AC_COUNT
} access_category_t;

/* Default EDCA parameter set (802.11-2020 Table 10-1, OFDM defaults). */
typedef struct {
    uint32_t aifsn;   /* arbitration IFS number  */
    uint32_t cw_min;
    uint32_t cw_max;
    usec_t   txop_limit_us; /* 0 == single MPDU only */
} edca_params_t;

static const edca_params_t EDCA_DEFAULT[AC_COUNT] = {
    /* AIFSN, CWmin,        CWmax,       TXOP(us) */
    [AC_BK] = { 7, DCF_CW_MIN,        DCF_CW_MAX,        0    },
    [AC_BE] = { 3, DCF_CW_MIN,        DCF_CW_MAX,        0    },
    [AC_VI] = { 2, (DCF_CW_MIN+1)/2-1,DCF_CW_MIN,        3008 },
    [AC_VO] = { 2, (DCF_CW_MIN+1)/4-1,(DCF_CW_MIN+1)/2-1,1504 },
};

static inline const char *ac_name(access_category_t ac) {
    switch (ac) {
        case AC_BK: return "AC_BK";
        case AC_BE: return "AC_BE";
        case AC_VI: return "AC_VI";
        case AC_VO: return "AC_VO";
        default:    return "?";
    }
}

/* AIFS[AC] = SIFS + AIFSN[AC] * aSlotTime  (Clause 10.22.2.4) */
static inline usec_t aifs_us(uint32_t aifsn) {
    return SIFS_US + (usec_t)aifsn * SLOT_TIME_US;
}

#endif /* MAC_COMMON_H */
