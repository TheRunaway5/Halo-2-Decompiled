# TU1.5 super-bounce investigation

**A Turf super bounce now succeeds in the native match and rendered game.**
It repeats with identical source-body motion and reaches the invisible ceiling.
The launch-producing physics frame matches unchanged TU1.5 instructions on the
host CPU. The broader object update also matches all CPU and memory outputs
when its two verified MCC `grip` name operands are linked explicitly. Whole-engine
1:1 parity, physical Xbox precision and complete native dependency closure
remain separate gates.

## Successful native route

Current receipts are in
`build/test-results/superbounce-tu15-native-launch-20261026/`.
The passing fixture is `tests/fixtures/superbounce_turf_native_match_tu15.json`.
It uses `MultiplayerPreparedWorld` and `MultiplayerMatch::step`, including the
normal source scenery population, player birth, equipment and local view.
The declared offline birth is on the ledge beside Turf's medical tent at
`(-8.5,8.25,2.01)`, facing positive Y. There is no live repositioning, supplied
contact, velocity or impulse.

After one neutral setup tick for the active player's first-person temporal
inputs, the 30 Hz command sequence is: neutral20, forward12, jump/forward1,
crouch/forward12, crouch36, then release crouch and movement for100 ticks.
The last segment looks down at pitch `-1.5`.

| Observation | Native result |
| --- | --- |
| Ordinary jump speed | `3.0799999237060547` |
| Tick85 vertical velocity | `-4.150182723999023` |
| Tick86 vertical velocity, with jump released | `13.733759880065918` |
| Tick86 source-body position | `(-8.5574083,11.9131737,1.4572964)` |
| Highest source-body position, tick140 | `z=19.050827026367188` |
| Height gained after launch | `17.593530654907227` |

The positive gate requires an uncommanded downward-to-upward reversal above
twice the authored jump speed and at least10 units of subsequent flight.
Initial birth height cannot satisfy the flight gate.

The refreshed native executable also reproduces every one of these181 tick
rows exactly (`resumed-early-native-launch.json` and
`resumed-early-motion-comparison.json`). Its independent launch-frame check,
`resumed-early-world-hardware/report.json`, passes206,438 unchanged original
instructions and92,464,743 persistent bytes with zero differences. The source,
executable and fixture identities are recorded in
`resumed-early-world-execution.json`; its raw comparison banks were removed.

`launch-repeat-0.json` through `launch-repeat-2.json` contain identical181-tick
rows. `rendered-launch-v2.json` delivers all181 ordinary input commands and
submits181 real native frames; every recorded position, velocity, crouch value
and jump/crouch input matches the three repeats. The rendered comparison is
recorded in `rendered-motion-comparison.json`. Removing crouch prevents the
launch; shortening airborne forward movement from12 to8 ticks prevents it too.
These failing positive-gate controls remain in `launch-control-*.json`.

The refreshed cohort passes all seven CTests without skips, including actual
rendered gameplay, in `resumed-render-kernels-build/report.json`.
`resumed-acceptance.json` records three more identical fresh native repeats,
both negative controls and a second181-frame rendered run. All native and
rendered tick rows remain identical to the initial cohort.

`native-turf-start.png` and `native-turf-ceiling.png` retain the actual rendered
start and ceiling views:427,274 and414,570 bytes respectively. Their hashes
and explicit small-artifact retention are recorded in
`retained-visual-evidence.json`; raw frames and build products are disposable.
The refreshed captures are byte-identical to both images. Their duplicate PNG
copies were removed; `retained-visual-evidence-refreshed.json` records the fresh
hashes and the two retained references, totaling841,844 bytes.
The six-dot texture mark has not been independently registered to a coordinate.

The declared spawn bank selects the offline match adapter; its report records
`canonical_source_engine:false`. This proves the native scene, command and
physics route with a declared birth. Canonical spawn scoring, online peers,
all maps, physical controllers and launcher delivery are separate claims.

## Launch-frame TU1.5 comparison

`launch-world-hardware-v3/report.json` compares the actual launch-producing
native `1CC8D0` frame against **206,438 unchanged original i386 instructions**,
with **92,464,739 persistent bytes** and complete FP/live-x87 outputs equal.
This uses the existing explicit void-frame API contract: its returned private
64 KiB stack and unreturned volatile registers are not fabricated outputs.
No solver result, contact, callee return or CPU estimate is substituted.
The reference banks/original binaries were removed after qualification.

The unlinked raw-MCC `B8F40` comparison is **not** a whole-call pass.
`launch-object-hardware-v2/report.json` executes48,084 original instructions;
its GP/SSE/x87/FP outputs match, but it finds107 returned-stack bytes and four
persistent bytes differing at `0x82D912E4..E5` and `0x82D912E8..E9`.
Those are at offsets `+0x15C4` and `+0x15C8` in the captured biped allocation.
The observed position, velocity and crouch outputs of this object phase match.
The refreshed unlinked comparison in `resumed-launch-object-hardware/report.json`
retains the same discrepancy.

The remaining fields are now identified. Read-only observation records the
actual writes at `115D7C` and `115D91`, after the non-dual `grip` queries at
`115D68` and `115D83`. The native owner already binds the canonical `grip`
StringID `0x0400060B` to the authored MCC key `0x04000617`. The earlier reference
name profile omitted these two exact queries. Their cached animation indices
are216 in the native run and remainNONE in the unlinked reference.

`qualified-grip-name-report.json` executes47,992 original instructions with
explicit authenticated StringID operand linking and matches **all92,530,279
accessible bytes**, including returned stack, and complete GP/SSE/x87/FP
outputs. The baseline name profile still fails; adding only those two missing
`grip` sites removes every difference. This correspondence is now included in
`tools/original_mcc_symbol_reference_tu15.py` and recorded in
`qualified-grip-name-reference.json`. It is an explicit name-aware reference
mode, **not an unchanged-instruction proof**. It changes no production kernel,
captured bank, contact, impulse, branch or supplied callee result. No comparison
mask was widened. The independent launch-world proof remains unchanged-code.

Additional full-Match original-cache and writeback reference attempts stopped
at missing reference boundaries; they are not passing parity evidence.
The later body-only original-cache attempt still reaches unowned first-person
resource callee `16E8A0` at `12657A`; omitting local view binding does not remove
that boundary. It therefore does not qualify an original-asset whole trajectory.
Its temporary helper was restored and its executable is excluded from the
accepted cohort; `object-difference-probe.json` records both observations.
The writeback replay reaches `1A3B70`, whose first instruction reads the
original I/O queue selector at `0x569AF0`. The native animation-resource owner
handles the real `126139 -> 1A3B70` request with bound resource bytes and a
retained native read receipt. The offline replay lacks the original queue
inputs; mounting guessed queue state would not qualify that call.
The exploratory composition prototype is explicitly invalidated in
`invalid-composition-prototype/validity.json` and is excluded from acceptance.
No comparison mask was widened to hide a gameplay difference.

Earlier whole-boundary controls also pass unchanged original CPU comparison:
Lockout's released-crouch `B8F40` call34,292 instructions; Turf's earlier
landing object update9,324; and its native world frame26,963. These controls
are documented separately from the successful launch.

The successful route does not call the `2F5600` collector below. Its collision
fix remains required for the separate late-crouch regression, and must not be
presented as the cause of this successful bounce. Native frame activity still
contains explicit compiled residual owners; `full_native_physics_closure`
remains false. The reference executes on a modern host CPU, so Xbox/Pentium III
RSQRT estimate bits and physical Xbox end-to-end parity remain unqualified.

## Rendered input and HUD fix

The finite native input-script path now accepts `jump` and `crouch` tokens and
delivers them through ordinary `Controls`. The rendered regression checks every
delivered movement/look/jump/crouch command against the fixture before accepting
its body trajectory.

The first rendered run stopped while loading HUD materials: the emblem owner
had already bound `emblem_background_index` to original operand `0x170005FB`,
then the HUD tried interpreting that original operand in the MCC namespace,
where its index resolved to `current_difficulty`. HUD material loading and
objective waypoints now preserve the emblem owner's completed binding. The
general typed-name and colliding-operand rejection guards remain unchanged.
The actual rendered bounce and HUD contract/geometry checks cover this fix.

## Native collision fix

A repeatable late crouch release above Turf's authored medical tent stopped
inside object movement at `2F7038 -> 2F5600`. The original indirect call target
is valid. `2F5600` was missing from the live native collision owner registry;
the address correspondence in the older world translation did not contain an
executable owner for it.

`native_tu15_scoped_contact_query` now owns the complete scoped collector,
exception-chain link, query argument frame, restoration, and return adapter.
The original shape-pair query `2F4DE0` remains an explicit compiled residual
dependency. This is a structured native collector kernel; its dispatcher has
no lifted `2F5600` fallback. It supplies no intersection, contact, velocity, or
impulse. The generator authenticates all 25 original wrapper instructions and
the original query call before emitting the bindings.

The original authority is English TU1.5 `u5English/default.xbe`, SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Executable-backed reference work is offline. Production uses the compiled
kernel and prepared MCC runtime content.

## Earlier component evidence

The earlier isolated-world/component receipts live under
`build/test-results/superbounce-tu15-20261026/`.

| Check | Result and limit |
| --- | --- |
| `missing-owner-source.json`, `turf-late-release-repeat.json` | Authenticated original target; repeated native failure at the real collision boundary before the fix. |
| `tent-native-original-2f5600/report.json` | Native collector plus its reached query closure matches 8,030 unmodified original i386 instructions, complete GP/SSE/x87/FP state and 92,520,035 accessible memory bytes. Linux CPU execution is not physical Xbox verification. |
| `contact-original-1f9c80-v2/report.json` | Another actual contact-controller branch matches 397 original instructions and 89,312,867 accessible memory bytes. This was an ordinary landing, not a successful bounce. |
| `cmake-tent-native-final/report.json` | `original_biped_movement` and `native_tu15_turf_late_crouch_contact` pass. The latter requires the actual native kernel call and the qualified peak velocity bits. |
| `crouch-prime-final-build/report.json`, `wiki-route-acceptance.json` | All three focused CTests pass, including the new native Lockout priming control. All 190 tick rows match between MCC and original assets, with 90 forced-crouch ticks. Removing crouch inputs makes that control fail with zero forced-crouch ticks. This does not establish a launch. |
| `acceptance.json` | Ordinary jump, late crouch landing, and the positive Turf candidate have identical per-tick motion across MCC and original assets: 151, 118, and 141 ticks respectively. Both asset routes use the same native owners, so this is an asset comparison, not an independent whole-engine oracle. |

The ordinary jump peaks at `3.0799999237060547`. The qualified late crouch
landing peaks at `2.4800314903259277`. The retained positive Turf candidate
peaks at `3.301017999649048`; it fails the required uncommanded downward-to-upward
launch threshold of `6.159999847412109`. Its fixture retains `expected_launch:
true` and is not registered as a passing bounce regression.

The isolated harness creates the original player/biped and authored scenery through the
live source factories, delivers ordinary player commands, advances the 30 Hz
world, and reads actual source body state. Survey geometry supplies no live
collision response. Any repositioned exploration is explicitly marked
`diagnostic_position_inputs`; it cannot qualify a natural classic route.

The complete native Match exposes a material setup difference: the direct Turf
drop lands on the tent roof, while the isolated control falls through it.
Therefore the earlier isolated grids and their matching component calls are
not a full-scene super-bounce oracle. The passing regression uses the Match
path and its complete scene; no physics arithmetic was changed to force it.

Focused CMake checks used a frozen source cohort. Its unrelated static-library
configuration/dependency adaptations are enumerated in
`wiki-route-acceptance.json`; they were not applied wholesale to the working
checkout or launcher-selected runtime.

## Route references and correction

The [original Lockout guide](https://haloology.wordpress.com/2014/02/14/lockout-super-jump/)
and [Turf guide](https://haloology.wordpress.com/2014/03/28/turf-superjump/)
provided initial route descriptions. The user supplied
[6 Halo 2 super bounces](https://www.youtube.com/watch?v=n7G0jM8inaI)
and [the super-bouncing wiki](https://halo.fandom.com/wiki/Super_bouncing).
The video includes a Coagulation base-cubby route and a Lockout takeoff/launch
around 0:54–1:02. Video footage establishes the demonstrated route and result;
it is not a retained controller-input or original memory trace.

The full wiki description, including its annotated Coagulation image, is
incorporated as route guidance: keep walking
after releasing crouch, avoid crouching again after priming, and, for the
Coagulation route, release movement just before landing. It identifies Turf's
landing mark as six dots on the medical tent. That texture mark has not yet
been registered to a collision coordinate. The article's proposed physics
explanation is community commentary, not an executable or hardware oracle.

**The early Lockout tower labels were reversed.** Authored pickups place the
sniper rifle at approximately `(-5.6411,-3.1223,-6.4375)` and the tower Battle
Rifle at `(6.7680,-2.5917,-5.8375)`. Reports named `br-primes` and earlier
negative-X corner drops explored the sniper side. They demonstrate bounded
crouch/contact behavior but do not verify the video's BR-tower route. Corrected
positive-X geometry is retained in `lockout-br-geometry.json`.

Comparison renders now place the video's takeoff on the **lower north BR
walkway**, rather than the upper platform explored earlier. Camera positions
and the installed survey runtime's hash are recorded in
`route-camera-survey.json`; those renders identify geometry, not acceptance
of the current collision build. A natural lower BR setup at
`(9.0,-0.7,-8.39)` reaches forced crouch with ordinary commands and remains
fully crouched throughout its 90-tick release interval. The retained control
`superbounce_lockout_crouch_prime_tu15.json` explicitly requires at least 80
such ticks. A passing priming control does not establish a super-bounce.
Natural exit/ledge trials from that setup have not recovered the demonstrated
landing. Explicit lower-bar pose diagnostics also produced no qualifying
launch; their pose changes exclude them from natural-route acceptance.

The apparent ramp-support issue investigated in `support-edge-controls.json`
was ruled out: authored sloping floor triangles span the BR path in question,
and the repeated Turf walk-off controls fall normally with and without a local
view. No support-physics change was made. Several clearance survey faces lie
inside solid geometry; their failed entry attempts cannot be treated as failed
classic routes. Diagnostic lower-bar placements also remain unqualified.

## Reproduction

Build and run the native and rendered regression with the disposable runner
(the rendered case requires a working desktop/Vulkan device):

```bash
python3 tools/run_tests.py \
  --target native_superbounce_tu15_tests --target biped_movement_tests \
  --target native_superbounce_runtime_tu15_tests --target mcc_hud_reference_tests \
  --filter '^(original_biped_movement|native_tu15_turf_late_crouch_contact|native_tu15_lockout_crouch_prime|native_tu15_turf_superbounce(_rendered)?|mcc_hud_reference_contract|mcc_hud_reference_geometry)$' \
  --cmake-arg=-DHALO2_SUPERBOUNCE_MCC_TURF=/absolute/path/to/prepared/MCC/turf.map \
  --cmake-arg=-DHALO2_SUPERBOUNCE_MCC_LOCKOUT=/absolute/path/to/prepared/MCC/lockout.map \
  --cmake-arg=-DHALO2_MCC_HUD_DONOR=/absolute/path/to/prepared/MCC/turf.map \
  --retain-report turf-superbounce-rendered.json --jobs 2 --test-jobs 1
```

The explicit `native_superbounce_tu15_tests` executable accepts
`--match-replay MAP INPUT_JSON OUTPUT_JSON` for the successful complete-scene
route, and `--replay MAP INPUT_JSON OUTPUT_JSON` for isolated component work.
Run one-off replays through
`tools/run_test.py`, with output under `HALO2_TEST_SCRATCH`. Fixtures include
the qualified Turf collision control, Lockout ordinary jump control, and
the earlier `superbounce_turf_candidate_tu15.json`, which remains unqualified.
The passing route is the separate `superbounce_turf_native_match_tu15.json`;
the old failing fixture was not silently changed into a passing control.
The separate `--reference-replay XBE MAP INPUT OUTPUT`
mode is confined to offline original-cache comparison.

## Remaining acceptance

The deterministic native Turf launch, continued flight, rendered gameplay,
negative controls and strict launch-world comparison are established for the
recorded cohort. The broader object call has full name-aware CPU/memory
correspondence. Original-asset whole-tick scheduling, full native physics
dependency closure, physical Xbox precision, online peers and launcher delivery
remain open. No launcher-selected runtime was promoted by this task.

The initial successful cohort starts with `source-preparation/source-manifest.json`;
explicit refresh receipts record its subsequent task changes. Its passing
CMake/CTest receipt is `final-rendered-build/report.json`.

The refreshed base source inventory is `resumed-source-cohort.json`, with
the changed physics/header hashes in `resumed-physics-cohort-delta.json` and
actual prepared cache hashes in `resumed-cache-inputs.json`. This source copy
includes concurrent native-kernel work. Two declared build-input
refreshes are recorded in `resumed-vehicle-link-refresh.json` and
`resumed-shader-prerequisite-refresh.json`: the current vehicle-rate archive
dependency and a missing, unused shader prerequisite. Neither changes gameplay
code. The resulting inventory is `resumed-render-source-cohort.json`.
Its CMake/CTest receipt is `resumed-render-kernels-build/report.json`;
`resumed-acceptance.json` records the subsequent fresh native repeats, controls
and original CPU comparison when run. Only passed receipts qualify acceptance.
The earlier `current-kernels-build` refresh was interrupted and never qualifies
acceptance. `cleanup-interrupted-owners.json` verifies its owned scratch was
removed. The first resumed two-job build was interrupted to resume the same
owned build with four jobs; `build-resource-adjustment.json` records that change.

`source-cohort-final-audit.json` verifies that the accepted private C++/header
and build inputs remained unchanged. The only later private edit is the
separately qualified offline name-reference profile. It also identifies later
concurrent checkout edits, including nine physics/header files, which are
excluded from this recorded cohort's acceptance. `final-acceptance.json`
consolidates the result and its limits.

Raw entry banks and original CPU scratch were removed after qualification;
their small metadata, hashes and cleanup receipts remain. Both finite build
owners have been removed, as verified by `cleanup-session-one.json` and
`cleanup-video-reference-session.json`. The latter removed 2,412,245,307 bytes,
including the temporary source/build tree, route replays and survey captures.
No large artifacts were retained by those earlier runs. Temporary survey
instrumentation was removed before their final three-check CMake run.
The current run's finite owners and build products are separately audited in
`superbounce-tu15-native-launch-20261026/cleanup-final-session.json`.
Only bounded reports, source hashes and the two explicitly listed small PNG
reference images are retained from the current run.
