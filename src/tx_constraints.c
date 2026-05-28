/* tx_constraints.c - see tx_constraints.h */
#include "tx_constraints.h"

bool twt_negotiation_succeeds(twt_setup_command_t cmd, bool params_match) {
    switch (cmd) {
        case TWT_REQUEST: /* responder chooses; always resolves            */
            return true;
        case TWT_SUGGEST: /* preferred but flexible; always resolves       */
            return true;
        case TWT_DEMAND:  /* only the indicated parameters are acceptable  */
            return params_match;
        default:
            return false;
    }
}

bool nstr_limited(const nstr_context_t *c) {
    return c->affiliated_with_mld
        && c->mld_has_nstr_pair
        && c->received_rts_on_pair
        && c->sibling_holds_txop;
}
