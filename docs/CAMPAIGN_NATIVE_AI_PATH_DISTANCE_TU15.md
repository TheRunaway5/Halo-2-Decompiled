# Native campaign remaining path distance

Whole TU1.5 `200C00` now has a native data implementation using the shared
native point/frame geometry. It reads the real actor guard, signed path
indices, retained coordinates and frame records. It follows original
`218780` dependencies for transformed points and performs the second XYZ
copy after failed frame resolution. No transform or distance is supplied by
the reference harness or the caller.

Each segment uses the original extended arithmetic and operand order:
`sqrt((dz*dz + dx*dx) + dy*dy)`, followed by extended addition to the previous
REAL sum and a REAL spill. The end index is freshly read after loading
previous Z and computing Y/X differences. A rejection at this boundary keeps
those three exact 80-bit quantities in a named typed pending value. Native
private coordinates do not occupy a synthetic Xbox stack.

[Proof](../analysis/campaign-native-ai-actions-20261030/path-distance-proof.json)
records 309 unchanged physical-original cases: 142 completed calls, 162
rejected reads and five true unmapped faults. All 84 root instruction sites
and 224 closure sites are exercised. Comparisons cover 12,781 ordered reads,
four input timelines and eight pending-quantity boundaries, with no retained
stores. GCC, Clang, actual-source ASan/UBSan, nine semantic mutations and an
independent byte-identical original repeat pass.

The pinned original SHA-256 is
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The arithmetic profile requires masked exceptions and up to three older live
x87 values. Callee-private stack aliases, overflow/unmasked exception owners
and numeric CALL/RET binding remain outside this component. Full navigation
search, action planning/publication and natural Armory continuation remain
open. The previous generated `200C00` body has not been retired or claimed as
native integration by this data-only proof.

The required original regression input is
`tests/fixtures/native_campaign_ai_path_distance_tu15.json.gz` (407,411 bytes).
All completed owned compiler/original-execution/source scratch directories
were verified removed. No executable or raw bank is retained; normal launcher
selection has not changed.

## Qualified numeric callers

The [binding proof](../analysis/campaign-native-ai-actions-20261030/path-distance-binding-proof.json)
now qualifies actual `1B8938` and `1BBF3B` CALL/RET transport. Its1,092 physical
original cases include290 completed calls,792 interrupted read prefixes,10
true memory faults,14 late return cuts and28 pending extended results. All
44,044 retained reads, complete scalar outputs, stack cleanup and admitted FP
state match. GCC, Clang, actual-source ASan/UBSan,11 semantic mutations and a
byte-identical independent original repeat pass.

The initial binding test exposed a real interrupted-projection mismatch.
The shared geometry algorithm now reports its actual permuted Z, local X and
scaled local X as typed native mathematical values. The binding retains those
values at their source boundaries, including unit/zero/signaling-NaN scales;
it constructs no virtual SIMD program. Separate mutations dropping each of
the three coordinate publications are rejected. The previous failed diagnosis
and qualified626-case reference are preserved.

Both callers are routed in the actual World source, and all six real
World/caller/kernel objects compile from a frozen local include closure.
That compilation is not World execution or linked-client acceptance. Current
shared-geometry regressions pass778 point,309 distance,468 path-result,877
area-choice and421 area-binding cases. The point/distance compiler, sanitizer
and mutation suites also pass after this extension.

Five other direct callers remain unqualified: `1B8472`, `1C1776`, `1C33E4`,
`1C5715`, `1C595C`. The old generated distance body remains for that unresolved
scope. Full Armory/action/navigation/native-only/launcher acceptance stays open.

The required current transport fixture is
`tests/fixtures/native_campaign_ai_path_distance_transport_tu15.json.gz`
(1,606,050 bytes). The initial893,259-byte original reference is preserved as
`analysis/campaign-native-ai-actions-20261030/path-distance-transport-v1.json.gz`.
Its small oracle/test/validator source snapshot is retained beside the report.
All completed owned original/compiler/source trees are removed.
