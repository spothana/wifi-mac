/* seq_num.c - see seq_num.h */
#include "seq_num.h"

void dup_cache_init(dup_cache_t *dc) {
    dc->head = 0;
    for (int i = 0; i < DUP_CACHE_SIZE; i++) dc->e[i].valid = false;
}

static bool match(const dup_entry_t *e, uint64_t address, uint16_t tid,
                  uint16_t seq, uint8_t frag) {
    return e->valid && e->address == address && e->tid == tid &&
           e->seq == seq && e->frag == frag;
}

bool dup_cache_is_duplicate(dup_cache_t *dc, uint64_t address, uint16_t tid,
                            uint16_t seq, uint8_t frag, bool retry) {
    /* A frame is a duplicate iff its Retry subfield is 1 AND it matches a
     * cached <address, tid, seq, frag> entry (segment 6 evidence). */
    if (retry) {
        for (int i = 0; i < DUP_CACHE_SIZE; i++)
            if (match(&dc->e[i], address, tid, seq, frag))
                return true; /* discard duplicate */
    }
    /* Not a duplicate: record it (round-robin replacement). */
    dup_entry_t *slot = &dc->e[dc->head];
    slot->address = address; slot->tid = tid; slot->seq = seq;
    slot->frag = frag; slot->valid = true;
    dc->head = (dc->head + 1) % DUP_CACHE_SIZE;
    return false;
}
