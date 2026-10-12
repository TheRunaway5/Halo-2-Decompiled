# Retained unit local-transition owner

The new `unit_local_transition` module closes the complete direct
`BB240 → 1CEFE0 → 1DCE20/1DCC30 → B8250` branch on retained source-owned
storage. It matches **31 complete-buffer original cases, 2,195 ordered stores
and 29 actual clock reads**. This removes the local-save callback boundary
from the constructor tail; it does **not** complete native unit creation,
channel installation, grenade release or live Q gameplay.

The authority is unchanged TU1.1 XBE (`aaea4c9c…5377`) and authored Lockout
(`14c54046…987a9`). The source observer retains full object headers/bodies,
manager/current/saved banks, counter bytes, register/stack inputs, defined
AL/XMM0 returns, literal clock/context, reads and writes. Independent source
execution and fixture export repeat byte-for-byte. Imported tag/fixture bytes
remain ignored, external evidence rather than embedded production assets.

| Complete body | Verified later-update body | Native entry |
|---|---|---|
| BB240 | BC380 | `capture_source_object_transition` |
| 1CEFE0 | 1D38A0 | `save_source_unit_local_transition` |
| 1DCE20 | 1E16E0 | `source_transition_remaining_seconds` |
| 1DCC30 | 1E14F0 | `start_source_transition_counter` |
| B8250 | B9390 | Existing `mark_source_object_for_update` |

The four transition bodies have complete normalized instruction/internal
branch correspondence in the immutable
[render interpolation report](../analysis/multiplayer/integration/inventory-source/animation-interpolation/frozen/report.json).
B8250 remains the previously frozen complete
[action-dispatch owner](MP_UNIT_ACTION_DISPATCH_RESEARCH.md). TU1.1 instruction
addresses in store observations are source identities; they are not executable
host pointers or a claim that the u5 world setup was run here.

`BB240` follows the original signed body fields +10E (saved locals), +110
(local byte count, MOVSX then logical SHR5), +112 (current locals) and +12A
(manager). It returns before metadata when +112 is FFFF and before bank access
when the actual object definition +38 model is NONE. Otherwise it invokes
1CEFE0 and **always** marks the object through B8250, including rejected or
zero-duration requests. It allocates no body/bank and creates no pose. The
captured biped and weapon body extents/offsets are original outputs, never
allocation constants in the new module.

`1CEFE0` rejects nonpositive node counts or a duration not greater than the
imported zero word. An active unfinished counter rejects a request below its
actual remaining seconds (unordered comparison also rejects). Admission
clears bytes duration/current/flags in original store order, preserving curve,
then copies actual current locals forward by REP MOVSD and starts direction1.
Only defined AL is exported. The observer compares every copied word and all
unchanged manager/body bytes; it does not accept a reconstructed pose.

`1DCC30` reads signed16 clock frequency, multiplies the requested seconds in
binary32, spills that result, applies x87 FISTP, clamps integer ticks1..127 and
uses duration=ticks−1. The inactive branch clears duration/current/flags;
the active branch preserves an unchanged duration/direction or rescales the
signed8 current byte with the listed SSE divide/multiply and CVTTSS2SI.
`1DCE20` uses signed8 duration/current and directionbit1, multiplying the
remaining count by literal clock+4 dt; an inactive counter returns zero
without reading the clock. Curve byte2 is preserved by all these owners.

The clock/context provider is mandatory where the original reads it. The
observed clock is the inherited fixture's full12 bytes at 518444, not a newly
proved ordinary clock producer. Its literal bytes are
`00001e008988083d00000000`; constructor calls retain actual FPCW023F, while
explicit root calls retain the inherited oracle's declared caller context
FPCW027F. Both use nearest rounding; MXCSR is recorded per call. Neither30Hz
nor this dt is baked into the native module. SSE modes other than masked,
nearest, no DAZ/FTZ stop; x86 host modes are checked too. FISTP conversion uses
the supplied x87 rounding control and requires masked conversion exceptions.
The module reproduces retained data/result arithmetic, not x87/MXCSR sticky
status flags. Non-x86 instruction context remains explicitly unsupported.

The measured source composition runs the unchanged genuine prepare,
join/spawn/loadout and cluster prefix, then declared transition roots using
only source-produced unit/banks/counter and actual executable duration words.
It performs a complete original 1DCD10 tick before a shorter request, obtains
remaining seconds from actual 1DCE20, and feeds those exact bits into an equal
request. Pure zero/one-second/rescale caller requests are explicit stimuli,
not recovered grenade or gameplay duration producers. The counter tick is
already owned by the earlier clock module and is not reimplemented here.

The first extension attempt incorrectly continued after the old constructor
suite's deliberate invalid bind stop. Its pending 1D0A60 observation, tool and
logs are retained as drafts. The final composition starts its extra requests
at the successful prefix instead; it never restores or supplies manager/body
bytes to bypass that stop. Older reports and the efd670 constructor-tail
freeze remain unchanged.

Required production inputs are the existing `UnitConstructorObjectPool`,
retained actual bodies and banks, real `SourceTagMetadataStore` callbacks for
**bipd/weap**, imported one/zero words, and the owned raw clock/context
provider. Actual generic Cache metadata resolves the source definitions,
including stock SMG E57B0406 at sourceVA80215FE4 with its full1388-byte tag.
The new module can bind the existing required local-save callback directly;
no resource, graph, allocation or update success callback is substituted.

Normal and GCC ASan/UBSan isolated CMake comparisons pass literal, actual
Cache, TU1.1 operand and authenticated u5 operand modes (4/4 each). Clang
ASan/UBSan also passes the literal fixture. Missing clock/metadata/header or
truncated counter backing and an unsupported FTZ provider context stop. The
actual Cache archives and matching observed headers are copied from the
previous immutable dependency snapshot; no claim is made that every archive
member was rebuilt from the observed headers. No full runtime/GPU test is
claimed.

The remaining complete constructor frontier is **114F00→1CEC20**, its actual
request/channel install owners, **1CFA90→1DF670** with resource/current
transform owners, and **114BD0→1CF4C0/1CFB90/115AF0**. Full B8A30 allocation,
body/type constructor order, whole retained unit/action composition and the
lower grenade lifecycle must compose before a live Unit bridge is admitted.
The new files make no Runtime/Simulation/Unit, frontend, renderer, shader or
release edits.

Evidence and commands:
[manifest](../analysis/multiplayer/integration/spawn-source/unit-local-transition/manifest.json),
[reproduction](../analysis/multiplayer/integration/spawn-source/unit-local-transition/REPRODUCE.md),
[source observation](../analysis/multiplayer/integration/spawn-source/unit-local-transition/measured-positive/report.json),
[native API](../include/halo2/unit_local_transition.hpp).
