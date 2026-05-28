/* rts_cts.h - RTS/CTS bandwidth signaling (static & dynamic modes).
 *
 * SEGMENT 3 (Managing Bandwidth Signaling and RTS/CTS).
 * SPEC: 802.11-2020 Clause 10.3.2.9 / VHT/HE dynamic bandwidth operation.
 *
 * Before a wide-bandwidth transmission, a STA may send RTS to reserve the
 * channel. For VHT/HE/EHT STAs sending RTS in non-HT or non-HT-duplicate
 * format, a "bandwidth signaling TA" is used, and the TXVECTOR channel-width
 * parameters must be identical. The responder's CTS width depends on mode:
 *   - STATIC : CTS echoes the full requested width (responder must have the
 *              whole width idle, else it does not respond).
 *   - DYNAMIC: CTS may cover any width for which CCA was idle on ALL
 *              non-punctured non-primary 20 MHz subchannels for a PIFS.
 */
#ifndef RTS_CTS_H
#define RTS_CTS_H

#include "mac_common.h"

typedef enum { BW_STATIC = 0, BW_DYNAMIC = 1 } bw_mode_t;

typedef enum {
    CHW_20  = 20,
    CHW_40  = 40,
    CHW_80  = 80,
    CHW_160 = 160,
    CHW_320 = 320,   /* EHT / 802.11be */
} chan_width_t;

/* Per-20 MHz subchannel CCA/puncture state for the requested width. Index 0 is
 * the primary 20 MHz. Up to 16 subchannels for 320 MHz. */
#define MAX_20MHZ_SUBCH 16
typedef struct {
    int  n_subch;                       /* width / 20                       */
    bool cca_idle[MAX_20MHZ_SUBCH];     /* CCA idle for a PIFS on subchannel*/
    bool punctured[MAX_20MHZ_SUBCH];    /* subchannel punctured (preamble)  */
} subchannel_map_t;

typedef struct {
    chan_width_t requested_width;
    bw_mode_t    mode;            /* originator dynamic-capable -> DYNAMIC   */
    bool         uses_bw_signaling_ta; /* TA set to bandwidth signaling TA  */
    bool         txvector_widths_match;/* CH_BANDWIDTH_IN_NON_HT==CH_BANDWIDTH */
} rts_request_t;

typedef struct {
    bool         valid;          /* responder sent a CTS at all             */
    chan_width_t granted_width;  /* width covered by the CTS                */
    const char  *reason;
} cts_response_t;

/* Validate that an RTS is well-formed for bandwidth signaling (segment 3
 * evidence): bandwidth-signaling TA set and the two TXVECTOR width parameters
 * identical. */
bool rts_is_valid_bw_signaling(const rts_request_t *r);

/* Compute the CTS a VHT/HE/EHT responder returns, given the requested RTS and
 * the per-subchannel CCA map. nav_idle must be true (responder only answers if
 * its NAV is idle). */
cts_response_t cts_compute(const rts_request_t *r,
                           const subchannel_map_t *sc, bool nav_idle);

#endif /* RTS_CTS_H */
