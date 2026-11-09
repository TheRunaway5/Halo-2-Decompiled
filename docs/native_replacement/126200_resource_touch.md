# Resource-touch controller

All three compatible TU1.5 `126200` owners now default to the typed resource-touch
controller in `include/halo2/native_tu15_resource_touch_kernel.hpp`, with retained
caller transport in `src/physics/runtime/native_tu15_resource_touch.hpp`.
Primary, AI-actor and AI-frame bindings require complete body, resolver and child
dispatcher authentication. `HALO2_REFERENCE_RESOURCE_TOUCH` keeps the original
route available. Canonical bodies remain intact.

The controller searches for an existing request, allocates through the actual
datum owner when necessary, and publishes the request's mode and resource.
Allocation failure uses the genuine millisecond-clock owner and the original
unsigned deadline test. Existing requests update a backed queue timestamp only
when the signed wrapped age exceeds its mode threshold. The typed controller
retains saved-register publication and pop timing, including stack/data aliases.
It neither supplies child results nor creates a resource-completion outcome.

Registered GCC sanitizer and Release gates both passed:
`build/test-results/native-resource-touch-official-20261109/report.json`.
Each configuration compares 987 transfers on the native route and another 987
with the parent reference switch enabled, across all three production copies:

- 136 original full-return fixtures and 36 genuine child-entry fixtures;
- 157 before-parent-read fixtures, including saved-register and return reads;
- all 60 original parent instruction sites;
- 1,296 complete child-entry GP/flags/SIMD/FP/live-80-bit packets per route;
- 101,068,800 exact memory-bank bytes per route, plus ordered reads and stores;
- 30 body/resolver/dispatcher authentication mutation rejections and seven
  admission refusals.

The retained sanitizer evidence is
`sanitizers/evidence/native-tu15-resource-touch.json` (5,434 bytes, SHA-256
`0f55524e19bda7b5f45c3ea7e2268e6fa97de2fbe48525402abe79ce08c6949b`).
Release evidence is 5,367 bytes, SHA-256
`ddf0b151c859f503932867958e28583fb2f2edf12e93542b6200ceba60dda6c9`.
The original observer metadata authenticates opcode bytes `852c8e` and `852c8b`
before restoring two bitmap `TEST` reads omitted by Capstone. No original code
or result was patched. A small diagnostic records a reverse-copy overlap
fixture that destroyed its own return address; its normal-transfer
replacement uses forward copying, while ordinary allocations cover both DF
directions.

Complete pinned canonical `126190`, `16E6A0` and `33B590` algorithms compute the
fixture child outcomes from genuine backed memory. Clock fixtures use backed
cells and an observation-only `595` boundary. They provide no clock producer.
The production shared clock has its separate real-backend/input-journal proof.
The test adapters route AI's external child handoffs to those complete backed
algorithms; production cross-unit admission remains a distinct integration
condition. Independent normal/read-phase review found no actionable mismatch.

Full fresh generators, raw/specialized CPP syntax and explicit native-binding
idempotency checks passed for all three units. AI-actor evidence is
`build/test-results/native-resource-touch-actor-generation-20261109/report.json`.
The shared primary/AI-frame checks are recorded in
`build/test-results/native-bsp-leaf-surface-primary-death-generation-final-20261109/report.json`
and `build/test-results/native-bsp-leaf-surface-ai-frame-generation-fixed-20261109/report.json`.
AI-frame uses all 49 authenticated references and 691 owners; omitting the
required 49th reference is rejected by complete dispatcher authentication.

`1264E0` maintenance remains a genuine tail-entry boundary and an explicit
residual. This port closes the controller's retained body on admitted routes;
it does not close the complete resource subtree, physical page-fault behavior,
store/observer exceptions, or an engine workload. The current child proofs and
old frozen engine cohorts retain their own source scopes.

The runner removed all private builds, binaries, original probe images and
memory-bank/fixture copies. Only bounded reports, hashes and commands remain.
