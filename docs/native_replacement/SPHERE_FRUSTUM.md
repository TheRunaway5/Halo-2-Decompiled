# TU1.5 sphere/frustum classifier

Actual `184690` now defaults to the typed native classifier in the particle-draw
unit. Its complete 110-instruction retained continuation remains available to
tracing, unsupported inputs, and `HALO2_REFERENCE_SPHERE_FRUSTUM`.

`native_tu15_sphere_frustum_kernel.hpp` owns the structured bounds/plane
classification. The retained view supplies lazy scalar reads, ordered native
SSE arithmetic, SIMD/flags publication, and the real RET read. The existing
public `source_sphere_frustum_visibility` uses the same algorithm through a
float value view. Plane 4 can reject a sphere but does not participate in the
containment test. Plane 5 retains its distinct center-X/normal-X acquisition and
multiply order, including NaN payload ownership.

The binding authenticates the complete original body, selected-image resolver,
and full dispatcher before inserting its exact default prefix. The original
body SHA-256 remains
`2d60a15a07af88107316c4ee6484ae1784a617e760fad5e7a2cef8a71ed5d52e`.
The public value seam separately authenticates its old complete body. The common
generator applies this binding only to the authenticated particle context.
The module generator preserves its existing attachment declaration include;
this restores byte-for-byte full-unit regeneration after common unused-header
pruning. No other recovered function body changes.

The unchanged local TU1.5 image is the raw ABI authority. The current corpus has
312 full returns and 197 before-read prefixes, including center/frustum/RET
overlaps, unaligned operands, IEEE edge inputs, incoming SIMD upper lanes,
rounding/precision modes, sticky bits, DF, and 0/1/4/8 live extended values.
Seven routes produce 3,563 comparisons and 116,571 complete pre-I/O packets per
compiler. The two bound default routes execute 1,018 calls with no instruction
callbacks or literal evaluations. Positive forced raw and specialized reference
controls execute 107 callbacks and 107 literal evaluations respectively, and
match 110 physical packets and the complete final ABI.

The numerical core and raw adapter passed GCC sanitizer and independent Clang
sanitizer runs. Five compiled mutations are rejected: including plane 4 in
containment, reversing the final-plane multiply, removing pre-read publication,
exposing FP state to a hostile memory provider, and losing rejection XOR flags.
Eight complete-body/resolver/dispatcher/public-seam mutations and 24 admission
or reference refusals are also measured. The old public float implementation is
a separately labelled native-native regression, not the raw CPU oracle.

The later opaque-MMX qualification adds a second copy of all 509 physical
paths, with eight complete declared 80-bit payloads, all 256 known-mask values,
and zero, signed-zero, normal, denormal, NaN, unnormal, and arbitrary upper-word
patterns. The private observer initializes a declared FSAVE register image and
uses an owned FNSAVE/FRSTOR observation stub outside the unchanged original
instruction image. It measures the real full tag word and all eight payloads
at every genuine I/O/final/prefix boundary. MMX registers are all nonempty;
the full FSAVE numeric-class tag word is not necessarily zero. An independent
uninterrupted MOVQ/FNSAVE probe also produces `AAAA` for all-FFFF upper words.
The initial incorrect zero-tag assumption and its correction are retained in
`native-sphere-frustum-mmx-oracle-diagnostics-20261109`.

The native `native_tu15_sse_mmx_scope.hpp` never writes, clears, or interprets
State's MMX aliases, mask, or x87 fields. It parks the caller's host register
file, runs the SSE-only body and protected providers under source controls and
status, then restores every host payload/tag/TOP while retaining current source
control/status/MXCSR for State.save. It exposes no x87 arithmetic, spill, or
child-adoption interface. The classifier's binary metadata separately rejects
any x87/MMX instruction, store, push, or call. Tag classification is diagnostic
test observation only; the numerical kernel and its admission do not inspect
the opaque payloads.

Combined dense/MMX qualification has 1,018 physical paths, 7,126 native
comparisons and 233,142 complete pre-I/O packets per compiler. The 2,036 bound
raw/specialized native calls execute zero callbacks/literals. Four positive
reference routes execute 214 callbacks and 214 literal evaluations, matching
220 physical packets and final ABI. All 3,563 MMX transactions verify the full
opaque caller register file and source FP handoff, including 0/1/4/8 occupied
host slots, opaque MMX files, sparse host tags, provider exceptions, and
providers leaving MMX clobbers plus unmasked pending x87 exceptions. The seven
additional ownership refusals and four compiled MMX ownership/mask mutations
are measured separately from the earlier controls.

Proofs and scope:

- `build/test-results/native-sphere-frustum-private-routing-20261109/report.json`
  retains the five compiled mutation failures and positive reference evidence.
- `build/test-results/native-sphere-frustum-bound-clang-20261109/report.json`
  qualifies the final bound raw/specialized routes and overlapping inputs.
- `build/test-results/native-sphere-frustum-bound-generation-exact-20261109/report.json`
  records full fresh unit byte equality, raw/specialized syntax, and stable
  preparation-tool and compiler-header hashes.
- `build/test-results/native-sphere-frustum-official-20261109/cmake/report.json`
  passes all three registered SAN, Release, and generation tests, with zero
  skips. Its retained reports cover the final bound source and exact full-unit
  regeneration. The owned build and offline committed dependency copies were
  removed after completion.
- `build/test-results/native-sphere-frustum-public-regression-20261109/report.json`
  passes the actual public API's existing 512 sphere and 256 cluster fixtures
  under both GCC and Clang sanitizers.
- `build/test-results/native-sphere-frustum-mmx-official-20261109/cmake/report.json`
  passes all three registered sanitizer, Release, and full-generation tests
  after adding opaque MMX admission. The independent final Clang sanitizer
  proof is `native-sphere-frustum-mmx-bound-clang-20261109/report.json`.
  The nine compiled mutations, 31 guards, all source and caller FP packets,
  exact byte regeneration, and current input hashes are retained in these
  reports. The owned build and dependency archives/copies were removed.
- The registered paired/generator gate is
  `original_tu15_native_sphere_frustum(_release|_generation)?`, target
  `native_sphere_frustum_tests`.

The dense qualified raw domain uses the existing masked RawX87Scope contract;
opaque MMX uses the separate neutral owner described above.
Memory and Before callbacks may alter host FP state or throw; source FP is
restored before returning or propagating the error. They do not arbitrarily
mutate retained GP/SIMD state, input policy, or backing bytes in this corpus.
Unknown or tracing instruction services retain the original body. Binary
metadata independently establishes that this classifier has no stores, pushes,
or callees, so its store observer is never invoked; observer purity is not an
admission dependency. Panic-observer cases verify this property.

Active MMX is supported for its actual TOP0/depth0, masked, opaque-register
contract. Sparse numeric source tags, unmasked numerical exceptions,
asynchronous signals, and hardware page
fault delivery are not newly qualified. Complete source SW reaches State.save;
the pre-existing outer FpScope host C-bit restoration limitation is unchanged.
This component does not establish a new full-engine performance or launcher
qualification. All generated banks, executables, and dependency copies belong
to disposable test owners; only bounded reports are retained.

A later admission repair rejects a cached cursor/literal witness when its live
instruction callable has been cleared. The exact original body then skips
cursor publication; the native bridge falls back without updating the stale
cursor. Two source-backed regressions compare the exact retained body with
both actual raw and prepared bound fallbacks, for dense and opaque MMX input.
The old failure remains in
`native-sphere-frustum-stale-clear-old2-20261109/report.json`.

`native-sphere-frustum-stale-clear-official-20261109/cmake/report.json` passes
all three registered sanitizer, Release, and exact-generation tests, including
ten compiled mutations. Independent final Clang sanitizers preserve the same
1,018 physical paths, 7,126 comparisons and 233,142 complete packets. The live
common State cursor repair is a separately qualified dependency, SHA
`facb4ba8a51ac91199778d92b4f4dc1891ff8836d2604fcf8585457d20fb7a16`.
The closure receipt is
`native-sphere-frustum-stale-clear-closure-20261109/report.json`; all owned
builds and dependency copies were removed, with no large retained artifacts.
Earlier frozen receipts keep their historical input hashes.
