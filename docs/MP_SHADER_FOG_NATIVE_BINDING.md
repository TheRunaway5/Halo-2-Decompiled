# Native fog externs and complete camouflage pass banks

The shared native fog provider now computes scalar39 and color41 from the
supplied original fog datum, near/far interval, camera forward and retained
default RGB. The selected camouflage-pass binder applies the actual vertex
extern rows to both live and retained banks before authored animation. This
does not connect a live view/fog producer or submit camouflage layers.
[Provider contract](../include/halo2/source_shader_fog.hpp),
[Provider and selected-pass binding](../src/source_shader_fog.cpp),
[Original retail/TU1.1/TU1.5 consumer evidence](MP_FOG_EXTERNS_SOURCE_RESEARCH.md)

## Preserved arithmetic and data ownership

Valid fog requires a non-NONE datum and far greater than near. Binary32
subtraction forms the interval, binary32 division forms its reciprocal, then
separately rounded products are subtracted from positive zero. The consumer
does not replace that order with a direct numerator division or sign-bit
negation. An invalid interval supplies positive-zero scalar39 and copies
the caller's three default RGB words. That default remains explicit rather
than being inferred from the saved capture's zero record.
[Native arithmetic](../src/source_shader_fog.cpp),
[Original operations, intermediate words and pointer copy](MP_FOG_EXTERNS_SOURCE_RESEARCH.md)

The native primitive admits finite inputs/results, nearest-even arithmetic,
masked SSE exceptions and gradual binary32 values. It checks the available
host controls and rejects unsupported rounding or flush modes. Explicit
binary32 stores and strict compiler options preserve the operation boundaries.
The returned scalar/RGB pair shares its equal reciprocal value; instruction
counts and architectural FP exception/status delivery are not reproduced.
Nonfinite contexts and live default-pointer initialization remain outside this
admission.
[Strict build options](../CMakeLists.txt),
[Native comparison and rejection controls](../tests/source_shader_fog_tests.cpp)

`bind_camouflage_fog_pass` consumes only scalar39 scalar rows and color41 RGB
rows in the selected material. The shared19BE0 consumer preserves all
unmentioned components, including RGB's destination W. Both the live bank
and retained animation base receive the same writes. Other externs and child
passes retain their own owners; the caller binds each selected pass separately.
[Selected-pass API](../include/halo2/world_material.hpp),
[Vertex extern semantics](MP_VERTEX_EXTERN_NATIVE_BINDING.md)

## Comparison scope and verification

The frozen native comparison matches2,304 scalar/RGB pairs against4,608 whole
unchanged dispatch calls across retail, TU1.1 and TU1.5. The768 input contexts
include264 valid intervals and504 default branches, asymmetric/default RGB,
signed zero, adjacent float intervals and gradual subnormal products.
Host rounding/flush/mask rejection and nonfinite input/result rejection also
pass. A fresh266-object CMake build passes all nine focused CTests and all14
verification commands. Both complete new CPU comparisons pass scoped
ASan/UBSan with leak detection. The instrumented owners are the fog provider/
selected-pass binder, first-biped resolver, vertex extern consumer and world
material; the remaining compatible libraries are built normally. Fresh
unchanged-original execution reproduces the fixture and all ten report/
listing outputs byte for byte.
[Frozen verification manifest](../analysis/multiplayer/game-variants/shader-fog-native-20261012/verification.json),
[Original fixture](../tests/fixtures/fog_externs_original.json),
[Complete native comparisons](../tests/source_shader_fog_tests.cpp),
[Verification driver](../tools/verify_shader_fog.py)

A separate complete comparison matches all60 pixel words,128 vertex
words and state75 in64 complete original16B90 preparations, including12
reset-enabled dispatcher calls and four valid fog contexts. It imports the
actual global override shader and its selected implementation, starts from
the original caller's declared vertex bank, applies authored parameter
remaps, resolves first-biped camouflage using the captured owner type/parent
and declared primary, then binds native scalar37/color40 and fog39/41.
Unmentioned vertex words and the supplied state75 alpha are preserved.
[Whole original binder/service inputs and limits](MP_CAMOUFLAGE_PASS_SERVICES_SOURCE_RESEARCH.md),
[Complete native pass-bank comparison](../tests/camouflage_pass_bank_tests.cpp)

The successful run is `verify-20261012T002720.611133Z`. It freezes the already
verified shader-state source plus the explicitly listed owned fog/test files
and their CMake declarations. Unrelated concurrent live-checkout changes are
excluded and recorded. There is no new renderer change or GPU parity claim
in this CPU milestone; the earlier shader-state rendered proof remains
separate and immutable.
[Frozen source and successful CTest log](../analysis/multiplayer/game-variants/shader-fog-native-20261012/verify-20261012T002720.611133Z/ctest.log),
[Earlier rendered state milestone](MP_SHADER_STATE_NATIVE_BINDING.md)

Those64 comparisons consume the immutable saved resource-backed report;
they do not manufacture resource readiness in the original execution. The15
original Master Chief bitmap-request stops remain stops. Native bitmap cache
identity, device packets, vertex-resource acceptance, normal clone submission,
current-frame scene-copy sampling and concurrent alias writes are outside the
bank comparison. No additional game variant is admitted and the operational
goal remains incomplete.
[Remaining original pass/copy schedule](MP_CAMOUFLAGE_PASS_SOURCE_RESEARCH.md),
[Alias boundary](MP_CAMOUFLAGE_DEPTH_ALIAS_SOURCE_RESEARCH.md),
[Operational goal](GAME_MODE_PROGRESS.md)
