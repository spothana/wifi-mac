/* edca.c - see edca.h */
#include "edca.h"

void edca_init(edca_station_t *s, int id,
               const uint32_t pending_per_ac[AC_COUNT], uint64_t seed)
{
    s->id = id;
    rng_seed(&s->rng, seed);
    for (int a = 0; a < AC_COUNT; a++) {
        s->ac[a].pending       = pending_per_ac[a];
        s->ac[a].active        = pending_per_ac[a] > 0;
        s->ac[a].cw            = EDCA_DEFAULT[a].cw_min;
        s->ac[a].retries       = 0;
        s->ac[a].delivered     = 0;
        s->ac[a].internal_coll = 0;
        s->ac[a].external_coll = 0;
        edca_choose_backoff(s, (access_category_t)a);
    }
}

void edca_choose_backoff(edca_station_t *s, access_category_t ac) {
    s->ac[ac].backoff = rng_uniform_incl(&s->rng, s->ac[ac].cw);
}

void edca_on_result(edca_station_t *s, access_category_t ac, bool success) {
    edca_ac_t *q = &s->ac[ac];
    if (success) {
        q->delivered++;
        q->pending--;
        q->cw      = EDCA_DEFAULT[ac].cw_min;
        q->retries = 0;
        if (q->pending == 0) { q->active = false; return; }
    } else {
        q->retries++;
        uint32_t next = (q->cw + 1u) * 2u - 1u;
        q->cw = next > EDCA_DEFAULT[ac].cw_max
                ? EDCA_DEFAULT[ac].cw_max : next;
        if (q->retries > MAX_RETRY) {
            q->pending--;
            q->cw = EDCA_DEFAULT[ac].cw_min;
            q->retries = 0;
            if (q->pending == 0) { q->active = false; return; }
        }
    }
    edca_choose_backoff(s, ac);
}
