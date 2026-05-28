/* test_seq_num.c - segment 6 (sequence numbers, dup cache, privacy) */
#include "seq_num.h"
#include "test_util.h"

int main(void) {
    printf("== Sequence numbers & duplicate cache ==\n");

    /* Counter assigns then increments, wrapping modulo 4096. */
    seq_counter_t c; seq_counter_init(&c);
    CHECK(seq_counter_assign(&c) == 0, "first sequence number is 0");
    CHECK(seq_counter_assign(&c) == 1, "second is 1");
    c.next = SEQ_MOD - 1;
    CHECK(seq_counter_assign(&c) == SEQ_MOD - 1, "assigns 4095 at the top");
    CHECK(c.next == 0, "wraps to 0 modulo 4096");

    /* Privacy randomization on MAC address change. */
    rng_t rng; rng_seed(&rng, 2024);
    seq_counter_t p; seq_counter_init(&p); p.next = 100;
    seq_counter_on_addr_change(&p, false, &rng);
    CHECK(p.next == 100, "no randomization when privacy inactive");
    seq_counter_on_addr_change(&p, true, &rng);
    CHECK(p.next < SEQ_MOD, "randomized counter stays in [0,4095]");

    /* Duplicate detection: retry=1 + cached match -> duplicate. */
    dup_cache_t dc; dup_cache_init(&dc);
    uint64_t addr = 0xAABBCCDDEEFFull; uint16_t tid = 3, seq = 42; uint8_t frag = 0;

    CHECK(!dup_cache_is_duplicate(&dc, addr, tid, seq, frag, false),
          "first reception is not a duplicate");
    CHECK(dup_cache_is_duplicate(&dc, addr, tid, seq, frag, true),
          "retried frame matching cache is a duplicate");

    /* A retried frame that was never seen is NOT a duplicate (and gets cached). */
    CHECK(!dup_cache_is_duplicate(&dc, addr, tid, 99, frag, true),
          "retry with no cache match is not a duplicate");

    /* Different TID is a different sequence space -> not a duplicate. */
    CHECK(!dup_cache_is_duplicate(&dc, addr, 7, seq, frag, true),
          "same seq under different TID is not a duplicate");

    /* Retry bit clear is never treated as duplicate even if seq matches. */
    CHECK(!dup_cache_is_duplicate(&dc, addr, tid, seq, frag, false),
          "retry=0 is never a duplicate");

    TEST_SUMMARY();
}
