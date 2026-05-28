/* mac_arch.h - MAC sublayer coordination-function taxonomy.
 *
 * SEGMENT 1 (Foundations of MAC Layer Coordination).
 * SPEC: IEEE 802.11be-2024 / 802.11-2020 Clause 10.
 *
 * The MAC sublayer organizes how stations (STAs) share the medium. It is built
 * from a family of coordination functions:
 *
 *   DCF  - Distributed Coordination Function : basic contention-based access
 *          (CSMA/CA). Foundation for everything else.        [src/dcf.c]
 *   PCF  - Point Coordination Function        : legacy polled, contention-free
 *          access via a Point Coordinator.                   [src/pcf.c]
 *   HCF  - Hybrid Coordination Function        : QoS-aware, two sub-functions:
 *            EDCA  - Enhanced Distributed Channel Access  : *prioritized* QoS,
 *                    contention-based.                       [src/edca*.c]
 *            HCCA  - HCF Controlled Channel Access         : *parameterized* QoS,
 *                    polled by a Hybrid Coordinator.         [src/hcca.c]
 *   MCF  - Mesh Coordination Function          : coordination for mesh BSS.
 *   TUA  - Triggered Uplink Access             : organized uplink transmissions
 *          (the AP triggers STAs to send on the uplink).
 *
 * This header just enumerates the family so the architecture is explicit; MCF
 * and TUA are named here for completeness (their full behavior is out of scope
 * for the runnable models, which focus on DCF/PCF/EDCA/HCCA and the data-path
 * mechanisms in segments 2-8).
 */
#ifndef MAC_ARCH_H
#define MAC_ARCH_H

#include <stdbool.h>

typedef enum {
    CF_DCF = 0,  /* contention-based, no QoS                */
    CF_PCF,      /* polled, contention-free, no QoS         */
    CF_EDCA,     /* HCF: contention-based, prioritized QoS  */
    CF_HCCA,     /* HCF: polled, parameterized QoS          */
    CF_MCF,      /* mesh coordination                       */
    CF_TUA,      /* triggered uplink access                 */
    CF_COUNT
} coordination_function_t;

typedef struct {
    const char *name;
    bool        contention_based; /* true=contend, false=polled/triggered */
    bool        qos_aware;
    const char *note;
} cf_descriptor_t;

static const cf_descriptor_t CF_TABLE[CF_COUNT] = {
    [CF_DCF]  = { "DCF",  true,  false, "CSMA/CA foundation" },
    [CF_PCF]  = { "PCF",  false, false, "legacy polled CFP" },
    [CF_EDCA] = { "EDCA", true,  true,  "HCF prioritized QoS" },
    [CF_HCCA] = { "HCCA", false, true,  "HCF parameterized QoS" },
    [CF_MCF]  = { "MCF",  true,  true,  "mesh coordination" },
    [CF_TUA]  = { "TUA",  false, true,  "triggered uplink access" },
};

#endif /* MAC_ARCH_H */
