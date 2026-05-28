/* rng.h - tiny deterministic PRNG (xorshift64) so simulations are repeatable.
 * A real NIC uses a hardware RNG to pick the backoff; for teaching we want the
 * same seed to always produce the same trace. */
#ifndef RNG_H
#define RNG_H
#include <stdint.h>

typedef struct { uint64_t s; } rng_t;

static inline void rng_seed(rng_t *r, uint64_t seed) {
    r->s = seed ? seed : 0x9E3779B97F4A7C15ull;
}

static inline uint64_t rng_next(rng_t *r) {
    uint64_t x = r->s;
    x ^= x << 13; x ^= x >> 7; x ^= x << 17;
    return (r->s = x);
}

/* uniform integer in [0, n] inclusive -- backoff picks slots in [0, CW]. */
static inline uint32_t rng_uniform_incl(rng_t *r, uint32_t n) {
    return (uint32_t)(rng_next(r) % ((uint64_t)n + 1));
}

#endif /* RNG_H */
