/* seq_num.h - sequence numbers, duplicate cache, and SNS spaces.
 *
 * SEGMENT 6 (Reliability through Sequence Numbers and Caches).
 * SPEC: 802.11-2020 Clause 10.3.2.14 (duplicate detection) / sequence spaces.
 *
 * Receivers keep a duplicate-detection cache keyed by <address, sequence
 * number, fragment number> (and TID for QoS). A received frame is discarded as
 * a duplicate if its Retry subfield is 1 AND a matching entry is cached.
 *
 * Transmitters draw sequence numbers from organized Sequence Number Spaces
 * (SNS). Modeled here:
 *   SNS2  : individually addressed QoS Data, indexed by <Address 1, TID>
 *   SNS9  : MLD QoS Data,                    indexed by <MLD MAC, TID>
 *   SNS11 : group-addressed data,            single instance per AP MLD
 * Sequence numbers are 12-bit and wrap modulo 4096. If MAC privacy is active,
 * the counter is RANDOMIZED (modulo 4096) whenever the MAC address changes.
 */
#ifndef SEQ_NUM_H
#define SEQ_NUM_H

#include "mac_common.h"
#include "rng.h"

#define SEQ_MOD 4096u  /* 12-bit sequence number space */

/* A single sequence-number counter (one per <space,index> tuple). */
typedef struct {
    uint16_t next;     /* next sequence number to assign (0..4095)         */
} seq_counter_t;

static inline void seq_counter_init(seq_counter_t *c) { c->next = 0; }

/* Assign and post-increment, wrapping modulo 4096. */
static inline uint16_t seq_counter_assign(seq_counter_t *c) {
    uint16_t s = c->next;
    c->next = (uint16_t)((c->next + 1u) % SEQ_MOD);
    return s;
}

/* Privacy: on MAC address change with dot11MACPrivacyActivated, randomize the
 * counter modulo 4096. */
static inline void seq_counter_on_addr_change(seq_counter_t *c,
                                              bool privacy_active, rng_t *rng) {
    if (privacy_active)
        c->next = (uint16_t)(rng_uniform_incl(rng, SEQ_MOD - 1));
}

/* ---- Duplicate-detection cache ---------------------------------------- */
typedef struct {
    uint64_t address;   /* transmitter address (or MLD MAC)                 */
    uint16_t tid;       /* traffic identifier (0xFFFF if not QoS)           */
    uint16_t seq;       /* sequence number                                  */
    uint8_t  frag;      /* fragment number                                  */
    bool     valid;
} dup_entry_t;

#define DUP_CACHE_SIZE 64
typedef struct {
    dup_entry_t e[DUP_CACHE_SIZE];
    int         head;   /* round-robin replacement                          */
} dup_cache_t;

void dup_cache_init(dup_cache_t *dc);

/* Process a received frame. Returns true if it is a DUPLICATE that must be
 * discarded (retry=1 and a matching cache entry exists). Otherwise records the
 * frame in the cache and returns false. */
bool dup_cache_is_duplicate(dup_cache_t *dc, uint64_t address, uint16_t tid,
                            uint16_t seq, uint8_t frag, bool retry);

#endif /* SEQ_NUM_H */
