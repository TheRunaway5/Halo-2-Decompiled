# Live source bounds and render-cache startup

The source-enabled multiplayer renderer now reads each body's retained owner
sphere from the same source bank that supplies its WORLD nodes. The source
world also constructs and activates its actual model render-cache pool. These
are connected production paths; complete live shield drawing remains open.

`MultiplayerObjectRuntime::object_sphere` resolves the full salted object handle
through the existing raw datum pool, then reads the four float words at body
`+30/+34/+38/+3C`. `MultiplayerMatch::source_object_sphere` forwards that read.
The existing live body, coverage and lighting assembly consumes this sphere
when the retained source world is bound. Its existing current-unit/previous-unit
selection remains the owner selection for living bodies and corpses. Missing or
stale handles stop the read. The diagnostic retained-player fixture keeps its
own captured sphere; ordinary host-only rendering keeps its existing bounds.

The original retail `133790` reads this retained sphere. Rebuilding a sphere
from a preview model is not evidence for those particular object bytes.
[Fresh original instructions](../analysis/damage-shields-goal-20261011/live-body-wiring/instructions/00133790-retail.asm),
[production reader](../src/multiplayer_object_runtime.cpp),
[live consumer](../src/multiplayer_runtime.cpp).

The live source-world startup invokes complete TU1.1 `3D260()` after the
existing `125280` arena constructor, then `2B520()` after object-map activation.
The first owner uses the actual `51841C` allocator, virtual `125E50` method,
`1656A0` checksum and `16D070` pool constructor, then publishes `510C24`.
Activation joins the already compiled `16D220/326970` reset owners. No separate
host render-cache array, captured pool, readiness flag or chosen allocation
result is introduced. This is the isolated render-cache lifecycle join; the
remaining owners in full renderer initializer `2B480` are still required.
[Fresh complete owner pairing and exact prerequisites](SHIELD_LIVE_MODEL_BUILD_BINDING_HANDOFF.md),
[compiled startup generator](../tools/generate_death_startup_runtime_1_1.py).

The independent original startup capture executes six complete roots from
unchanged TU1.1 instructions. Two captures are byte identical: 1,235 distinct
instruction addresses, 1,053,629 ordered retained stores and all eight final
memory banks. The native fixture compares every store and the complete final
banks. The actual native world, authored biped birth and two-player prepared
match also check the published pool's validity, capacity, stride, name-derived
salt, empty occupancy and record salts. Source sphere tests compare all four
float bit patterns against the independently implemented original bounds owner
after real biped creation, and reject NONE/wrong-generation handles.
[Original capture](../tools/oracle_source_render_cache_startup_1_1.py),
[fixture](../tests/fixtures/source_render_cache_startup_original.json),
[native binding checks](../tests/multiplayer_object_runtime_tests.cpp).

All 11 selected startup, native binding, shield selector, model admission/build,
shadow and model shader checks pass without skips. The source-enabled
two-player render was also exercised; the validation record retains each build
and capture outcome, including a failed intermediate atlas-identity check.
Concurrent renderer edits advanced after the tested build; the record preserves
the tested executable and stable delivery sources separately from that drift.
[Validation artifacts](../analysis/damage-shields-goal-20261011/live-owner-sphere).

The existing manual death-camera scenario still stops at original cleanup call
`D80E5 → BB6E0`; corpse-path validation is incomplete. The new sphere accessor
does not write vitality or feedback. The host `Simulation` damage snapshot and
the retained source-object damage bank remain separate authorities. Geometry
resource construction, actual resource I/O/completion/fixups, model admission,
node context, view/lighting owners and the complete transparent scene queue
remain necessary before claiming a visible source shield effect. The rendered
frame and isolated checks do not prove damage gameplay or Xbox framebuffer
parity. [Next exact bindings](SHIELD_LIVE_MODEL_BUILD_BINDING_HANDOFF.md).
