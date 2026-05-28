# 802.11 MAC Mechanisms — A Teaching Simulator

A small, dependency-free C project that **implements and tests the MAC sublayer
mechanisms** of IEEE 802.11, built to be read alongside the standard (the
modules cite clauses of 802.11-2020 / 802.11be-2024).

Each topic is a self-contained, documented C module with its own test suite.
Every test's `CHECK` is named after the spec property it verifies, so the tests
double as a checklist of MAC behaviors.

## The eight segments

| # | Segment | Module(s) | Test |
|---|---------|-----------|------|
| 1 | MAC coordination-function taxonomy (DCF / PCF / HCF=EDCA+HCCA / MCF / TUA) | `mac_arch.h`, `dcf*`, `pcf`, `edca*`, `hcca` | `test_edca_triggers`, `test_dcf`, `test_pcf`, `test_edca`, `test_hcca` |
| 2 | EDCA prioritization + backoff invocation triggers | `edca*.c`, `edca_triggers.h` | `test_edca`, `test_edca_triggers` |
| 3 | RTS/CTS bandwidth signaling (static & dynamic) | `rts_cts.c` | `test_rts_cts` |
| 4 | Frame aggregation (A-MSDU / A-MPDU, exponents, spacing) | `aggregation.c` | `test_aggregation` |
| 5 | Block Ack window sizing + Protected Block Ack | `block_ack.c` | `test_block_ack` |
| 6 | Sequence numbers, duplicate cache, privacy randomization | `seq_num.c` | `test_seq_num` |
| 7 | Multi-recipient rate selection + non-HT rate conversion | `rate_control.c` | `test_rate_control` |
| 8 | TXOP/PPDU duration, TWT modes, NSTR limitation | `tx_constraints.c` | `test_tx_constraints` |

## Build and run

```bash
cmake -S . -B build
cmake --build build

./build/demo                                # runs every segment with traces
ctest --test-dir build --output-on-failure  # 11 test suites
```

Requires only a C11 compiler and CMake >= 3.10. No third-party libraries. Builds
clean under `-Wall -Wextra -Wpedantic` and runs clean under
AddressSanitizer + UBSan.

## What each segment models

### 1. Coordination-function taxonomy (`mac_arch.h`)
Enumerates the family: **DCF** (contention-based foundation), **PCF** (legacy
polled CFP), **HCF** split into **EDCA** (prioritized QoS, contention-based) and
**HCCA** (parameterized QoS, polled), plus **MCF** (mesh) and **TUA** (triggered
uplink) named for completeness. DCF/PCF/EDCA/HCCA are fully simulated by the
coordination-function engines in `dcf*`, `pcf`, `edca*`, `hcca`.

### 2. EDCA + backoff triggers (`edca*.c`, `edca_triggers.h`)
The scheduler runs four per-AC backoff entities with the Table 10-1 defaults;
voice/video win the medium more often via lower AIFSN/CW. `edca_triggers.h`
isolates *when* a backoff is invoked: a new MPDU queued under specific
queue/counter/medium conditions, a TXOP completing, or losing an **internal
(virtual) collision** to a higher-priority AC. `medium_busy()` models Physical
CS, Virtual CS, TXNAV, and (for mesh STAs with MCCA) RAV.

### 3. RTS/CTS bandwidth signaling (`rts_cts.c`)
Validates that a VHT/HE/EHT RTS uses a bandwidth-signaling TA with matching
TXVECTOR widths, then computes the responder's CTS. **Static** mode answers only
if the full requested width is clear; **Dynamic** mode falls back to the widest
sub-width whose non-punctured non-primary 20 MHz subchannels were CCA-idle for a
PIFS. Punctured subchannels are skipped; a busy primary or busy NAV yields no
CTS.

### 4. Frame aggregation (`aggregation.c`)
A-MPDU max length from the capability **exponent** (`2^(13+e) - 1`), the
**group-addressed rule** (use the *minimum* exponent across recipients so the
least-capable STA can decode), the A-MSDU length cap, and **minimum MPDU
spacing** that scales with PHY rate and the MPDU MU Spacing Factor.

### 5. Block Ack (`block_ack.c`)
Window sizing by the **least-capable endpoint**: EHT/EDMG -> 1024, HE -> 256,
HT/DMG/non-HT -> 64 (so EHT<->HE is capped at 256 by the HE side). Agreements
bind **MLDs** when both endpoints are MLD-affiliated, else individual STAs.
**Protected Block Ack** requires both endpoints to be PBAC STAs (MFPC && PBAC).

### 6. Sequence numbers & duplicate cache (`seq_num.c`)
12-bit sequence counters that wrap **modulo 4096**, organized into spaces (SNS2
QoS data by `<addr,TID>`, SNS9 MLD QoS by `<MLD MAC,TID>`, SNS11 group). A
duplicate-detection cache discards a frame iff its **Retry bit is 1** and it
matches a cached `<addr,TID,seq,frag>`. With MAC privacy active, counters are
**randomized mod 4096** on a MAC address change.

### 7. Rate control (`rate_control.c`)
For a control frame to multiple recipients, picks the highest `(MCS,NSS)` tuple
**every** recipient supports. **Non-HT conversion** maps an MCS to a reference
rate, then selects the highest `BSSBasicRateSet` rate <= that reference for the
response.

### 8. Transmission constraints (`tx_constraints.c`)
`aPPDUMaxTime` bound on EHT PPDUs; **TWT** negotiation (Request always resolves,
Suggest is flexible, Demand only succeeds on an exact parameter match); and the
**NSTR** limitation, which holds only when *all* conditions are met (MLD
affiliation, an NSTR link pair, an RTS received on that pair, and a sibling STA
holding a TXOP on the paired link).

## What this is and isn't

A **teaching** model: it favors readable, spec-traceable logic over PHY
fidelity. Frame durations are fixed constants, collisions are decided by
"same-slot" rather than signal overlap, and rate/spacing formulas are simplified
shapes that preserve the *relationships* (more padding at higher rates, least
capable endpoint governs) without reproducing per-PPDU PHY tables. The backoff
PRNG is deterministic (`rng.h`) so a seed reproduces a trace exactly. Natural
extensions: add EIFS after errored frames, real per-rate durations, A-MPDU
reordering buffers, and full MCF/TUA behavior.

## Suggested study path

1. `mac_arch.h` — anchor the coordination-function family.
2. `dcf.c` (clause 10.3) — run the demo, find a collision, watch `cw`/`retries`.
3. Compare PCF (10.4) and HCCA (10.22.3): both polled; HCCA grants TXOP
   *durations* and orders by priority.
4. EDCA (10.22.2) + `edca_triggers.h` — trace an internal collision
   (`AC_BK yields to AC_VO`) in the demo.
5. Walk segments 3-8 module by module; each test names the property it checks.

## Layout

```
include/  mac_common.h mac_arch.h rng.h
          dcf.h dcf_sched.h pcf.h edca.h edca_sched.h edca_triggers.h hcca.h
          rts_cts.h aggregation.h block_ack.h seq_num.h rate_control.h tx_constraints.h
src/      dcf.c dcf_sched.c pcf.c edca.c edca_sched.c hcca.c
          rts_cts.c aggregation.c block_ack.c seq_num.c rate_control.c tx_constraints.c
          demo.c
tests/    test_util.h
          test_dcf.c test_pcf.c test_edca.c test_hcca.c test_edca_triggers.c
          test_rts_cts.c test_aggregation.c test_block_ack.c test_seq_num.c
          test_rate_control.c test_tx_constraints.c
CMakeLists.txt
```
