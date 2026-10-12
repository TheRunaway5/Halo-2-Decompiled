# Original object-light sample composition

`source_object_light_sample` implements the complete original `D1E10` object
branch over explicit 56-byte render attribution, resident geometry, model PRT
availability, the current PRT cache and a bitmap service result. It composes the
independently verified `241C0`, `D33A0`, `D31C0`, `D2DC0` and lighting mix rather
than deriving light from expanded GPU vertices.

The return status and written sample are separate. A missing section returns2
and leaves all five caller words untouched. With a valid section, a positive
model PRT count returns0 only when the original PRT resource and current-cache
lookup succeed; otherwise it returns1. A nonpositive model count retains2.
Baked lighting does not change that status. The original can therefore write
a complete baked or fallback sample while reporting1 or2. Callers must preserve
the original status acceptance rule.

The PRT strip offset selects three literal indices. Declaration57 reads five
signed SHORTs per point with the original midpoint conversion; every other
declaration reads five floats. `24490` writes descriptor `+4/+8` as the point
payload size; `D1E10` adds that `+4` offset to the resident base at `+C`.
The sampler receives this resolved point payload explicitly. Geometry section
comes from contact `+14`; the independent PRT cache index comes from `+1C`.
Palette and barycentric words also come directly from the attribution packet.

Radiance and incident must both succeed before the baked sample participates
in mixing. Texture query coordinates use the second geometry descriptor
(zero-based index1, byte offset `20` hex), including its `+8` address offset.
Successful diffuse RGB is clamped and packed with `131ED0`; failed `D30D0`
emits the original object-branch literal `FFC0C0D0`.

`tools/audit_source_object_light_sample.py` executes unchanged complete original
`D1E10` and all called source services against an immutable retained snapshot.
Eight contacts are independently recorded complete `14A8E0` diagnostic outputs.
The remaining3,448 cases are explicit data controls: current-cache/model PRT
counts, missing section/material bitmap, radiance/incident streams, and two
float-point declaration layouts and unequal geometry/PRT cache indices. Two
explicit cache slots have distinct coefficient banks so an accidental section
index cannot pass by selecting identical data. They are not observed game frames. Every
executed source instruction is checked against the supplied original XBE.
All2,121 instruction sites match; each supplied current PRT record bank and
complete arena remain unchanged by the original call. Literal snapshot bytes
and explicit controlled cache-bank variants are recorded separately.

Native replay matches3,456 statuses and five-word output samples, plus1,728
two-word bitmap UV inputs:24,192 exact words. It uses the recorded original
bitmap service result without replacing that service in the original execution.
This checks the complete object-branch composition, while the existing texture
sampler has its separate CPU decoding/filtering checks. It does not establish
a native bitmap residency service, current PRT cache producer, object lifetime
or full-frame visual equivalence.

Whole-call composition exposed and corrected the earlier isolated UV fixture's
native stream selection: the source used the second descriptor, while that
test had placed its selected input in a synthetic third slot. Both the literal
bank corpus and complete object-branch replay now enforce the same index.
Independent review also corrected the geometry-section/PRT-slot distinction.
A deliberately wrong native slot-index build fails the expanded corpus at
case144, while the production implementation passes all cases.

Normal and fully instrumented address/undefined-behavior checks with leak
detection pass. Registered CTest is `retail_source_object_light_sample`.
Source, fixture, executable and log hashes are retained in
`analysis/multiplayer-shading/object-light-sample-native-validation.json`.
