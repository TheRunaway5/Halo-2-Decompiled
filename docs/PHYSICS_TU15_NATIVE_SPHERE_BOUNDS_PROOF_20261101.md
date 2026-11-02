# Native TU1.5 sphere bounds

The actual remaining Warthog bounds owner2E8D90 is now a native semantic
kernel and is wired into broadphase. All20 source sites are authenticated
against English TU1.5 SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
It preserves the native x87 margin-plus-radius calculation, one binary32 spill,
SIMD operand order, all four vector lanes and the second center read after
publishing the lower bound. It contains no virtual registers, instruction
execution, supplied arithmetic result or original after-state.

`build/test-results/native-sphere-bounds2-20261101/report.json` passes295
physical original cases with GCC and Clang:284 complete calls, ten admitted
read/store/observer prefixes and an actual aligned-store fault.192 independent
precision/rounding/DAZ/FTZ combinations, signed zeros, denormals, infinities,
paired NaNs, source radius/margin cancellation and aliased output are covered.
Wrong lower sign, stale upper center and wrong radius-field mutations fail.

The initial oracle helper incorrectly required x87 status and dead stack slots
to remain unchanged for a routine that performs x87 arithmetic. Its failed
report remains. The corrected observer verifies actual incoming live payloads,
control/TOP/tags and separately records the original numerical status; the
native side must match that independently measured status and MXCSR exactly.
Four read prefixes and one matrix-alignment case holding a private x87 value
remain explicitly excluded. Private transient payload/unmasked trap ownership
is not qualified by this report.

`build/test-results/native-world-sphere-bounds-binding-20261101/report.json`
passes110 original world cases,96 native force compositions, the world model
and its negative control. The model executes the true sphere owner, verifies
both source stores and lower/upper expansion, counts native work and rejects
invocation of the old residual. A separate per-world sphere activity counter
and required positive-activity gate prevent zero fallback from being mistaken
for an exercised native replacement.

The first live census identified2E8D90 in actual bounds fallback. Later driving
cases sometimes do not execute this shape, so generic zero-bounds-fallback in
those cases is not sphere qualification. Visible positive sphere activity and
strict original-frame comparison remain required and pending.

All component/compiler/world-model scratch owners are absent. Their separate
cleanup receipts retain measurements where available. Only small diagnostics,
source pins and reports remain; no large new artifact was retained. Live pins
are in `analysis/physics/native-sphere-bounds-live-binding-20261101/source-pins.json`.
