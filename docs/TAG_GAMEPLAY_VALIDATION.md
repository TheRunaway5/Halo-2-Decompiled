# Tag migration gameplay validation

2026-10-12. **Multiplayer verification passes on the exercised Slayer paths.
Campaign validation remains incomplete.** No tag migration regression was
reproduced. The campaign tests retain actual failures rather than substituting
damage, skipping mission services, clearing collision, or reporting startup as
a completed playthrough.

The reproducible build is `build-tag-gameplay-frozen-20261012`. Its production
source was copied to `analysis/tags/gameplay-validation-20261012/source` before
building because other work was changing class layouts in this checkout.
`source-sha256.json` pins 11,016 files. Original assets and reference captures
are shared through symlinks. The native executable SHA-256 is
`63b0adae915d575ccd058d58bc87ece0fce280095c3db4a94b11b984d7d71d20`.
The current launchers belong to concurrent work; this validation invokes the
recorded binaries directly and does not replace those launchers.

## Complete multiplayer matches

| Final fixed-build run | Result | Simulation ticks | Rendered frames | Fired rounds |
| --- | --- | ---: | ---: | ---: |
| Two local players, Slayer, final collision-sensor driver | 3–1, score limit | 1,639 | 6,535 | 498 |
| Local host and actual UDP peer, Team Slayer | 2–2, time limit | 2,910 | 11,464 | 542 |
| Separate rendered host/client processes, Team Slayer | 1–1, time limit | 2,924 | 5,913 host / 5,606 client | 92 host / 107 client |

`tag_gameplay_match_tests` drives actual SDL virtual controllers through the
normal rendered runtime. It uses authored spawns, collision, weapon definitions,
damage, scoring, respawns and game-over owners. It sets ordinary editable
variant fields: three kills, a 90-second limit and SMGs. It never teleports a
player, injects damage/deaths/scores, edits collision or accelerates the game
clock. The controller driver reads state for aiming and movement; when a route
stalls it tries walking around the obstruction.

The finite tests require movement, ammunition consumption on the same full
weapon identity, fatal damage, a changed salted unit after respawn, live audio,
particle draws, submitted Vulkan frames, automatic results and the complete
original countdown. The final Coagulation driver uses live composite
collision and passes with 927 audio packets, 15,738 particle draws and 1,494
final-scoreboard draws. It is pinned in `scene-collision-driver-build.json`
and `coagulation-final-driver.json`. The earlier 3–2 complete match and its
original driver remain retained as `ffa-final.log`; no production engine code
changed between them. The UDP case additionally requires the peer's final
score/kill/death records and countdown to agree with the authority. It uses
the port's native UDP protocol, not retail Xbox System Link.

`tag_network_match_tests` additionally runs **two independent rendered native
processes**, each loading its own cache/tag world and using its own SDL
controller, audio and Vulkan loop. Both complete the normal match, death,
salted respawn, automatic scoreboard and results countdown. Their final full
player/unit/weapon identities and score/kill/death records match exactly.
The client identifies its assigned player from the real scoreboard highlight,
opened through the configured PC scoreboard key; it does not assume join order.
Both processes remain open for 15 ordinary updates after countdown completion
to receive and observe the final results before closing their windows.

The new driver links the unchanged pinned production libraries/runtime object.
Its source, compiler/link commands and hash are in `network-driver-build.json`;
the final executable SHA-256 is
`eb392d98c7095e7e2e766b62ad7a27893f58930133dca0a0e710aef51e079c88`.
The reproducible runner and both traces/results are in
`two-process-final/report.json`. An earlier two-process run also completed
with matching 1–2 final records; its exact driver source and results are retained
as `network-driver-initial/` and `initial-two-process-match.json`.

The measured match results and exact executable pins are in
`analysis/tags/gameplay-validation-20261012/validation.json`. Per-tick TSV files
retain positions, full identities, vitality, ammo, scores, input and weapon
state. Actual windows used 1280×720 on an NVIDIA GeForce RTX 5080. This is Linux
evidence; it does not establish physical controller, WAN, Windows or macOS
behavior. Objective families remain gated as recorded in
[game mode progress](GAME_MODE_PROGRESS.md).

Additional rendered probes pass on Lockout:

- Map pickup and switch, independent dual-weapon fire/reload, posed first-person
  animation, firing sounds/particles, grenade animation and offhand drop.
- Actual UDP death, automatic death-scoreboard GPU submission, salted respawn
  and HUD restoration.

These focused probes use their existing explicit test setup, including debug
placement or collision simplification. They are separate from the complete
matches above, which retain authored collision and spawns.

An additional Zanzibar match reaches its ordinary time limit and results but
fails the required combat/respawn coverage. The first driver sees through
placed scenery because its visibility query uses only BSP collision. At tick 1,
the live composite collision hits at 12.139 units while the target is 17.6037
units away and the BSP ray is clear. The main test driver now queries the same
current collision as gameplay. That is a read-only test sensor correction;
production engine behavior is unchanged.

Subsequent Zanzibar attempts correctly stop firing into the obstruction, but
receive no authored navigation route and fail to reach combat. Experimental
wall steering also fails and was removed from the main test driver. All four
failed run reports/traces and exact experimental driver sources remain in
`zanzibar-ffa*.json`, `scene-collision-driver/`, `scene-steering-driver/` and
`wall-follow-driver/`. Zanzibar combat coverage is not counted as a pass.

## Campaign findings

The actual application was rendered and driven through SDL input in Old
Mombasa and High Charity, using the existing workbench switches to equip an
authored SMG and place an authored squad. This exercises campaign engine
consumers; it is not an authored mission playthrough.

Old Mombasa ran 1,800 ticks and 1,685 rendered frames, fired 240 rounds and
registered 226 hits, with audible output (`peak=1`), but killed no actor.
The test deliberately fails its fatal-damage requirement. The selected SMG
damage tag contains lower=0 and both upper bounds=4.75. Independent raw-asset
inspection retained the exact bytes `000000000000984000009840` in
`campaign-damage-assets.json`.

`Simulation::damage` still selects `w.damage_min` in the generic campaign path.
The pre-migration snapshot uses that same expression and reads the same lower
tag field. The multiplayer SMG path separately uses the recovered amount
calculation. This is an existing campaign damage gap, not a changed tag byte or
a reproduced migration regression. General campaign amount/difficulty and
live request owners remain unfinished; see
[the original amount-owner handoff](DAMAGE_AMOUNT_RUNTIME_HANDOFF.md).
Using a guessed multiplier would not establish original behavior.

The same rendered SDL encounter driver was also built against the saved
pre-engine-migration campaign/cache sources. It fired 240 rounds, registered
229 hits and killed no actor, failing the same fatal-damage requirement.
The post-migration run registered 226 hits; these closed-loop input runs are
not identical input replays or a whole-game differential. They reproduce the
same failure on both sides. The baseline damage/cache source bytes were
verified unchanged from the backup. Its in-progress snapshot needed recorded
declaration/include, controller-owner qualification and CMake linkage repairs
to compile; those repairs and exact source hashes are explicit in
`baseline-source-sha256.json`. The baseline executable, command, log and trace
are retained in `campaign-baseline-run.json`.

High Charity ran 1,800 ticks and 5,520 frames with audio, but the input driver
stopped at obstructed geometry after four shots and no hits. That failure does
not establish a damage regression. Earlier Cairo and Delta attempts likewise
did not reach combat; their failed traces are retained.

All 15 normal authored campaign startup entries also stop on unsupported
engine services within 300 ticks. Twelve have identical diagnostic lists in
the historical runtime and the fixed current build. The other three advance
past previously missing training/weather services and stop at later services.
The audit includes map/executable hashes and complete diagnostics in
`campaign-startup-frozen.json` and `campaign-startup-old-runtime.json`.
No full campaign mission or campaign parity is claimed.

A later recheck copied and hashed the currently installed runtime before
running the same 15 startup windows. All 15 still failed. The tutorial now
advances past AI dialogue and stops at `device_set_power`; the other diagnostic
lists match the fixed validation build. This later runtime is distinct from
the pinned gameplay build. Its exact selection/hash and diagnostics are in
`installed-runtime-tutorial-recheck.json` and
`installed-runtime-campaign-startup.json`. Subsequent source changes require
their own validation.

The rendered Cairo BSP transition/checkpoint test separately loads BSP 1 and
restores BSP 0. This checks streamed tag/resource ownership and restoration;
it is not mission completion. Its capture is `bsp-restore-final.ppm`.

A separate normal Cairo run uses `--campaign`, the authored starting inventory
and no placed squad. Its SDL Escape input takes the ordinary cinematic-skip
path and restores the authored checkpoint. Both the saved baseline and migrated
build then stop on `object_type_predict`, with no spawned actors, movement or
combat. The migrated run observes 1,801 step calls and 1,705 rendered frames;
the baseline observes 1,802 step calls and 1,621 frames. Recorded state columns
agree over their shared observation range. One transient capture-click fire
input differs and the baseline consumes one extra tick before the queued quit;
these are explicitly retained in `campaign-normal-cairo-comparison.json`.
The two failing run reports and the unchanged production linkage are in
`campaign-normal-cairo.json`, `campaign-normal-cairo-baseline.json` and
`campaign-normal-*-driver-build.json`. This is an actual normal-startup failure,
not successful campaign combat.

The later world-services snapshot still fails all 15 startup windows. Its new
tutorial `map::at` exception is traced to `SourceDeviceWorld::script_set`:
`device_set_power` receives handle `E2BE004F`, but the campaign loader explicitly
excludes machine definitions, leaving that device owner absent. The original
animation/physics connection remains separate scope. Exact binary/source pins,
backtrace and call frame are retained in `campaign-world-services-recheck.json`
and `tutorial-device-owner-gap.json`; no missing owner was synthesized.

### Source-backed campaign prediction prerequisite

The normal Cairo failure above led to a separate prerequisite fix. Retail
opcode `68`, evaluator `002A2400`, reads the supplied object definition's
predicted-resource block at `obje+B4` and invokes `0016E5E0` in mode 2. It
does not simply predict the object's render model. Twenty-eight executions of
the unchanged evaluator, dispatcher and model-section traversal agree with
the authored lists, including `NONE` and both supplied geometry-readiness
results. HS argument evaluation/return storage and the final cache services
are explicit supplied boundaries in this oracle.

The new `Cache::object_resources` uses the shared tag API, verifies the full
salted handle and object-group inheritance, and decodes those records through
the shared metadata-block reader, including Vista resident-region checks.
The simulation sends the request to native residency through
the normal predictor callback, including after a level restart. All 11 literal
campaign calls, covering 475 authored bitmap/model records across Cairo,
Outskirts and The Great Journey, pass asset tests with retained native data.
Duplicate requests, `NONE` and stale salts are checked. Other resource types
remain explicit unsupported native owners; this is coverage of the shipped
campaign operands, not a claim that all resource dispatch is implemented.
The final reader additionally imports every resident object list across all
74 Xbox/Vista gameplay caches: 14,109 object definitions and 136,982 resource
records. Every imported record has type 0 or 2. This broader check validates
metadata/counts/region bounds; it does not preload every model and bitmap.
Its report is `object-prediction/all-object-lists.json`.

The original type-2 path requests geometry without the explicit model
predictor's material traversal. A first candidate incorrectly added that
traversal and failed on a Great Journey shader reference. That failed test is
retained. The corrected implementation separates geometry retention from
material admission and tracks whether a later explicit model prediction has
admitted its materials. Both the updated campaign checkpoint suite and the
475-record asset test pass on the isolated candidate.
An explicitly derived geometry-only cache fixture additionally starts with
zero resident bitmaps and then admits all 105 bitmaps on a later explicit
model request, matching direct model admission. Its single metadata-count
edit and both file hashes are recorded in `object-prediction/deferred-materials.json`.

Normal Cairo startup now advances beyond object prediction and the existing
native cache barrier, prints its authored mission messages, and stops on the
next missing `device_set_position_immediate` service. This failing attempt
retains 1,801 step observations, 1,676 rendered frames, the ordinary cinematic
skip and checkpoint restore, with no actors or combat. The current root has a
later device binding, but the missing campaign machine owner described above
still prevents treating that binding as complete gameplay.

The additive production patch is preserved in
`object-prediction/root-production.patch`. The candidate was built from the
immutable gameplay source plus that prediction fix and the later root cache
barrier binding. Its full source hashes, compiler/link commands and unchanged
binary pins are in `object-prediction/candidate-build.json`; it is separate
from the original frozen build. The current root's touched production sources
also pass syntax checks and the shared-tag access guard. A complete build of
the concurrently edited root is not claimed by the isolated candidate tests.
The gameplay reports below use the source recorded in
`object-prediction/candidate-build-before-region-guard.json`. A final follow-up
routes the same list read through the cache-aware helper's additional Vista
region checks; the earlier content source is preserved beside that manifest.
The final asset, checkpoint and deferred-material checks are recorded in
`object-prediction/region-guard-cpu-checks.json`.
The final normal-startup rerun is
`object-prediction/normal-cairo-region-guard.json`; it again stops at the same
device service after passing object prediction. The reusable retail oracle is
`tools/oracle_object_prediction.py`, with pinned owned cache inputs in
`tests/fixtures/object_prediction_assets.json` and the shared resource fixture.

The candidate additionally completes a fresh real-time Coagulation Slayer
match, 3–2: 1,967 ticks, 7,630 rendered frames, 604 fired rounds, 1,114 audio
packets, 18,406 particle draws and 1,440 results draws. Its report and trace
are `object-prediction/coagulation-final.json` and
`object-prediction/coagulation-final/ticks.tsv`. The original historical runs
above remain unchanged. The campaign fix addresses an existing prerequisite;
it is not a reproduced tag-migration regression or completed campaign.
The updated candidate also passes a new independent host/client Team Slayer
match, 3–1, with 2,709 ticks and exact final identity/score records. Both
processes complete automatic results and the full countdown. The host submits
9,619 frames and the client 9,510; the retained report is
`object-prediction/two-process-final/report.json`.

## Original-reference and lifecycle checks

Twenty selected CTests pass without skips on the fixed build: tag contracts,
original assets and the retail tag oracle; instruction-derived SMG update,
ready/equip/damage, UDP publication, scoring and respawn fixtures; original
game-over imports/owners; native results and campaign checkpoint/skip owners.
The one- and two-player menu/profile/lobby/load/results/unload/return lifecycle
checks also pass. The XML results are `frozen-reference-tests.xml` and
`menu-lifecycle-tests.xml` in the validation directory.

A separate decoded-content differential successfully imports all 74 gameplay
caches across Xbox and Vista, covering 174 BSPs and 1,392 component comparisons.
Every serialized component digest matches the saved baseline. The actual
scenario type selects campaign or multiplayer import mode, and the owned
retail XBE is supplied explicitly for both sides. Coverage includes complete
collision, navigation and script programs, plus explicitly serialized core
player/movement, weapon, actor/squad, scenario-object and cutscene/trigger/
transition fields. It does not serialize every `LevelContent` field or prove
complete original runtime behavior. `content-differential-probe/gameplay-report.json`
and `build-gameplay-inputs.json` retain source/binary/cache/image hashes,
commands and all component digests. The first Vista attempt lacked an XBE;
its failures remain in the initial report, separate from the corrected pass.


The rendered Vista menu roundtrip also passes with TU1.1: lobby cancellation,
loading menu/world fades, the game-over scoreboard above the fade, report,
Done and a fresh lobby are verified. This probe accelerates a portion of the
match clock explicitly; it is separate from the complete ordinary-time matches
above. Its unchanged executable hash, command and result are retained in
`menu-roundtrip-tu11.json`.

The initial probe lacked `VK_LAYER_KHRONOS_validation`. A validation layer was
extracted only inside the workspace and enabled for this process. A subsequent
retail-executable run failed because the loader has no retail scoreboard tables.
The same direct `OriginalData` probe confirms all four tables are absent with
retail and present with TU1.1/TU1.5 both before and after the migration. The
retail failure remains retained; no engine behavior or assertion was changed to
obtain the supported-update pass. Layer provenance and the side-by-side inputs
are in `validation-layer/download-arch.json` and
`menu-diagnostics/scoreboard-image-inputs.json`.

Existing animation, material, particle-death and effect-light gaps are also
present in runtime logs. Passing these checks does not establish full retail
framebuffer, audio or campaign equivalence.

## Reproduction

Run from the repository root with original imported files and an active display:

```sh
build-tag-gameplay-frozen-20261012/tag_gameplay_scene_collision_tests \
  assets/disc/maps/coagulation.map assets/disc/maps \
  assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03 \
  ffa analysis/tags/recheck-ffa
build-tag-gameplay-frozen-20261012/tag_gameplay_match_tests \
  assets/disc/maps/coagulation.map assets/disc/maps \
  assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03 \
  team-lan analysis/tags/recheck-team-lan
python3 tools/run_tag_network_match.py \
  --native build-tag-gameplay-frozen-20261012/tag_network_match_tests \
  --output analysis/tags/recheck-two-process
build-tag-gameplay-frozen-20261012/tag_campaign_gameplay_tests \
  assets/disc/maps/03a_oldmombasa.map assets/disc/maps e1_cov_inf2_5 \
  analysis/tags/recheck-campaign
build-tag-gameplay-frozen-20261012/tag_campaign_normal_gameplay_tests \
  assets/disc/maps/01b_spacestation.map assets/disc/maps --normal \
  analysis/tags/recheck-normal-campaign
python3 tools/audit_campaign.py \
  --native build-tag-gameplay-frozen-20261012/halo2_native --ticks 300 \
  --output analysis/tags/recheck-campaign-startup.json
```

The campaign commands currently return failure. The CMake target
`tag_campaign_gameplay_tests` accepts either a squad name or `--normal`.
The fixed normal observer was linked separately as
`tag_campaign_normal_gameplay_tests` to preserve the original tested binary.
Live targets are explicit manual targets in `cmake/Tags.cmake`, so display/asset
requirements and existing campaign gaps do not silently turn ordinary CTest
runs green.
