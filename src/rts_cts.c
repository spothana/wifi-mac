/* rts_cts.c - see rts_cts.h */
#include "rts_cts.h"
#include <stddef.h>

bool rts_is_valid_bw_signaling(const rts_request_t *r) {
    /* Both conditions from the evidence must hold for a VHT/HE/EHT RTS in
     * non-HT / non-HT-duplicate format. */
    return r->uses_bw_signaling_ta && r->txvector_widths_match;
}

/* Largest contiguous-from-primary width (in number of 20 MHz subchannels) for
 * which every non-punctured subchannel had idle CCA. Dynamic mode lets the
 * responder "fall back" to a narrower width that is fully clear. The candidate
 * widths are 1,2,4,8,16 subchannels (20/40/80/160/320 MHz). */
static int clear_subchannel_count(const subchannel_map_t *sc) {
    /* Walk candidate widths from widest to narrowest, return the first whose
     * non-punctured subchannels are all CCA-idle. */
    static const int cand[] = {16, 8, 4, 2, 1};
    for (size_t c = 0; c < sizeof(cand)/sizeof(cand[0]); c++) {
        int w = cand[c];
        if (w > sc->n_subch) continue;
        bool ok = true;
        for (int i = 0; i < w; i++) {
            if (sc->punctured[i]) continue;         /* punctured: skip CCA  */
            if (!sc->cca_idle[i]) { ok = false; break; }
        }
        if (ok) return w;
    }
    return 0;
}

static chan_width_t subch_to_width(int n) {
    switch (n) {
        case 1:  return CHW_20;
        case 2:  return CHW_40;
        case 4:  return CHW_80;
        case 8:  return CHW_160;
        case 16: return CHW_320;
        default: return CHW_20;
    }
}

cts_response_t cts_compute(const rts_request_t *r,
                           const subchannel_map_t *sc, bool nav_idle) {
    cts_response_t out = { .valid = false, .granted_width = CHW_20,
                           .reason = "" };

    if (!rts_is_valid_bw_signaling(r)) {
        out.reason = "RTS not valid for bandwidth signaling";
        return out;
    }
    if (!nav_idle) {
        out.reason = "responder NAV busy, no CTS";
        return out;
    }

    int clear = clear_subchannel_count(sc);
    if (clear == 0) {
        out.reason = "primary 20 MHz not idle, no CTS";
        return out;
    }

    if (r->mode == BW_STATIC) {
        /* Static: responder answers only if the FULL requested width is clear,
         * and the CTS echoes exactly that width. */
        if (subch_to_width(clear) >= r->requested_width &&
            clear >= (int)(r->requested_width / 20)) {
            out.valid = true;
            out.granted_width = r->requested_width;
            out.reason = "static: full width clear";
        } else {
            out.reason = "static: full width not clear, no CTS";
        }
    } else {
        /* Dynamic: responder may grant any width up to the requested one that
         * is fully clear on non-punctured non-primary subchannels. */
        chan_width_t w = subch_to_width(clear);
        if (w > r->requested_width) w = r->requested_width;
        out.valid = true;
        out.granted_width = w;
        out.reason = (w == r->requested_width)
                     ? "dynamic: full width clear"
                     : "dynamic: fell back to clear width";
    }
    return out;
}
