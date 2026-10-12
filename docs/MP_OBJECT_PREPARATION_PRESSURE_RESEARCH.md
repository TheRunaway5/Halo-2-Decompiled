# Object request preparation and memory pressure

`object_preparation_pressure.hpp/.cpp` ports complete original `DDAC0`,
`148360`, `148740`, `22ECB0` and `148510`. This closes the biped request
preparation and pressure CPU owners required by the generic constructor and
physical-component wrapper. It does not create a native Unit or complete
world reclamation. Every reached lower owner remains mandatory.

The external evidence is
`analysis/multiplayer/integration/spawn-source/object-preparation-pressure`.
The registered test is `original_object_preparation_pressure`.

## Original execution and paired bodies

The unchanged original PlayerTrace preparation and join/spawn prefix retain
their previously declared compatibility/platform/world/resource boundaries.
Two independent runs produce identical `measured/report.json` and
`repeated/report.json`, SHA-256
`81976d9769e4e05df5b031eb50af97963e03aacb6564b63393726522c45291ca`.
They contain 40 natural entries: one `DDAC0` and thirteen each of `148360`,
`148740` and `22ECB0`. Another fifteen entries are explicitly declared
preparation controls: only the actual definition's flag word and masked
MXCSR inputs vary; the complete actual request and authored scalar are
retained. These controls never replace a live root or Unit body.

The actual Lockout biped has `+264 = 00000004` and `+270 = 3E333333`.
Its natural `DDAC0` is therefore a no-op. Setting bit 0 in a separate kernel
control admits the arithmetic; additionally setting bit 4 rejects it again.
No authored-root adjustment is inferred from the scalar alone.

Complete original-byte correspondence and direct controls cover both owned
TU1.1 and authenticated u5English:

| TU1.1 | u5 | Complete instructions |
|---|---|---:|
| DDAC0 | DE9E0 | 26 |
| 148360 | 1496F0 | 123 |
| 148740 | 149AD0 | 60 |
| 22ECB0 | 2335B0 | 16 |
| 148510 | 1498A0 | 16 |

Every complete body uniquely matches its normalized instructions, internal
branches, relocated data and call targets. The paired direct original
controls retain equal result bits, output bytes and normalized store order.
`controls-measured-v2/report.json` and `controls-repeated-v2/report.json`
are independently identical, SHA-256
`e7fa44c6dec80ef563d8fd6e1e60aefac6079198abaad24cbe2c70b157a9eea0`.
Earlier control observations remain separate; v2 is the final contract.

## Complete retained contract

`DDAC0` reads the definition from the actual C4 request, resolves its literal
low16 tag-table slot, tests byte `+264` bit 0, then rereads the DWORD and
tests bit 4. Only the admitted branch reads scalar `+270`. Each component
uses separate source `MULSS` then `ADDSS` in order:
`request[1C+i*4] += request[34+i*4] * scalar`, i=0,1,2. The three stores are
`DDB06`, `DDB19`, `DDB2C`; no fused operation or additional request fill is
used.

`148740` first checks second and first overflow pools' signed `+20` counts.
It still processes the main pool even when an overflow already implies 3.
The main pool's signed `+F0340` is converted with `CVTSI2SS` and multiplied
by the selected-image scale. If the first ordered `COMISS` admits it,
`22ECB0` **replaces XMM0** before the remaining three comparisons. This
dependency is obscured by the decompiled text and is retained in the port.

`22ECB0` reads all sixteen triples in descending address order. The first
triple is main-pool `+F0384`, `+F03C8`, `+A8`; each address decreases by 4.
It sums `(left-right)*scale` with 32-bit wrap, then performs signed
`CVTSI2SS` and the source scalar multiplication. There is no silent zero
statistic or fabricated quiet-heap fallback. The final pressure is the
maximum of the original main/overflow classifications, 0..3.

`148360` requires actual busy/collecting bytes, last-collection tick,
overflow and clock pointer slots. A nonzero busy byte returns immediately.
Otherwise the actual pressure call precedes setting busy. The owner runs
up to three outer phases, three inner phases and **51** successful reclaim
attempts per inner phase. Every eighth successful attempt in an outer phase
runs the original world-dispose/new-map sequence, toggling the collecting
byte around the latter. It checks whether a previously present overflow
disappeared before invoking the diagnostic/reset dependency `1482F0`.
Failed reclaim in outer phase 0 with no recent collection invokes complete
`148510`, refreshes pressure, then stores the actual current tick.

Recent means last tick is not `FFFFFFFF` and the signed 32-bit wrapped
`current_tick-last_tick` is less than 10; negative differences also qualify.
The actual clock pointer and global tick are reread at the original points.
Normal exit clears busy. A missing/throwing lower owner preserves the
already executed prefix; the port does not invent an exception cleanup that
clears busy or completes collection.

Required lower identities are `1C7EC0`, `1C7970`, `1C6100`, `1C5FF0` and
`1482F0`. Native composition binds `148740` to the complete heap query and
`148510` to the complete collector. For `1C7970`, only ECX, low AL and the
three stack arguments are owning inputs: the first two arguments have only
a defined low byte; their upper bytes remain unspecified. The final argument
is full `FFFFFFFF`. `1C7EC0` receives full argument 1. Original callee return
instructions independently establish stack pops 12 and 4 respectively.
Opaque addresses require bounded real backing. Malformed spans/address
overflow stop explicitly, rather than becoming success or `NONE`.

## Native differential and floating-point boundary

The test compares 55 owning-scope entries (40 natural, 15 declared controls)
and 262 paired original controls. It checks **4,608** defined lower-call
boundaries, **831** ordered retained stores and **71,690** full retained output
bytes. Controls cover busy admission, exact recent-tick boundaries and wrap,
failed/successful reclaim results, pressure exit, full retry cap, every-eight
collection and overflow disappearance, plus signed counters/statistic wrap
and five masked rounding/DAZ contexts. Lower reclaim/world return values in
these direct controls are explicitly test-only boundaries, not native live
providers. Natural low-pressure entries execute the coupled native heap and
statistic bodies, with no pressure-result replay.

Unicorn does not reliably update SSE sticky exception status for these
arithmetic instructions. Its literal observed MXCSR records are preserved,
but are not treated as Xbox hardware authority. The differential compares
control bits and exact result bits. Native x86 executes separate actual SSE
operations, retains its real sticky status in the explicit caller context,
and restores the host's complete MXCSR; restoration is checked. Unmasked
exception delivery and non-x86 arithmetic backends are explicit unsupported
boundaries. No emulator status is copied into native output to conceal the
limitation.

The native public scalar outputs are the statistic's low32 XMM0 bits and the
pressure classification, plus retained stores and MXCSR. Source private ABI
stack residue and caller-saved vector upper lanes are not reconstructed;
the measured owning pressure caller consumes the low scalar only.

Strict optimized copied-source and actual ASan/UBSan checks pass. Registered
production checks and their copied objects/archives are recorded separately.
Only the owned module object is correlated to its source and archive member;
other linked archive members are observed dependencies, not claimed rebuilt
or audited by this slice.

## Selected-image inputs and next live gate

`u5/operand-packet.json`, SHA-256
`114e5aa6a177a98c850415428a5f0308db1237236f5c611d4b455a06512dd567`,
contains six literal DWORDs in `SourceObjectPressureOperands` field order:

| Field | TU1.1 | u5 |
|---|---|---|
| main_usage_scale_bits | 4650A4 | 46A144 |
| top_limit_bits | 451FF8 | 456910 |
| high_limit_bits | 4650A0 | 46A140 |
| medium_limit_bits | 46509C | 46A13C |
| low_limit_bits | 465098 | 46A138 |
| statistic_usage_scale_bits | 465094 | 46A134 |

Production receives the actual selected-image words through the API; no
payload is hardcoded. Heap records, busy/collecting state, clock and world
callbacks are separate real owners. The generic constructor can now compose
`10CBD0 -> DDAC0` and its `148360` pressure prefix. Genuine header/body
allocation, growth, physical and animation/world/resource owners must still
compose before a live Unit or incoming Q action can be claimed. No final
Unit buffer, component offset, active byte, grenade count or successful
allocator/audio result is supplied here. Exact commands and immutable audit
scope are in the evidence directory's `REPRODUCE.md` and final manifest.
