# Campaign implementation status

## Current simulation continuation — 2026-10-14

The Armory now reaches its authored completion through ordinary player Controls
in 4,150 game ticks, without cinematic skip or script faults. A second run
restarts the authored checkpoint once and completes at the same game tick after
4,983 submitted steps. Both runs fit the aggregate memory ceiling and reproduce
their pre-compaction tick traces and raw logs byte for byte.

**The full campaign remains unfinished.** The other 14 authored entry points
still fail on unsupported animation, attachment, AI, save, interpolation,
kill-volume or visibility services. Native Armory completion does not establish
original animation/combat behavior, rendered or audio parity, later map handoff,
or complete campaign/BSP/checkpoint coverage.

The continuation fixes actor-start reference frames, nonpositive short sleeps,
posed unit query centers, ordinary authored navigation adjacency, first-chunk
sound prediction and shared collision storage. Original executable fixtures
cover the relevant scalar/selection/transform behavior. Unit query centers use
the existing native pose owner; full original animation scheduling and posed
combat hit regions remain open.

All simulations are serial and zero-swap. The guard configures 60 MiB for the
whole job and all descendants, leaving 4 MiB accounting headroom, and rejects
any kernel-measured peak above 64 MiB. The Armory and its checkpoint replay
peak at 60 MiB. Three full multiplayer simulations pass the same policy and
retain byte-identical tick/final traces against both captured baselines.
Compilation runs separately, serially, under 1 GiB with zero swap.

See the [complete results and remaining map matrix](../analysis/tags/gameplay-validation-20261013/campaign-completion/RESULTS.md),
[aggregate report](../analysis/tags/gameplay-validation-20261013/campaign-completion/report.json),
and [current reproduction procedure](../analysis/tags/gameplay-validation-20261013/campaign-completion/REPRODUCE.md).
The captured application and validation targets link successfully; the
concurrently edited root checkout and installed launcher are separate delivery
checks. Original base/TU1.1 CPU evidence is not Xbox TU1.5 visual proof.

The subsequent [AI owner continuation](../analysis/tags/gameplay-validation-20261014/campaign-ai-references/RESULTS.md)
adds separate source squad/group records and packed-reference iteration. Forty
whole-original banks and 9,002 original queries validate linked order, group
traversal, starting-location SID matching and inactive/direct-reference behavior.
The canonical importer now retains squad/group parents, starting-location SIDs
and inherited general flags, and resolves team bytes using the original direct
unit/creature/general-type precedence. All 15 actual campaign maps pass the
import/constructor checks. The actor bank's 96 cold-constructor cases remain
passing. Armory completion/checkpoint and all three full multiplayer traces are
unchanged; the measured continuation peak is below 61 MiB, with zero swap.

These AI banks are not yet connected to Simulation's actor birth, activation,
perception or script-list lifetime. The same 14 campaign entry points remain
failed. No additional mission or original AI/gameplay parity is claimed. The
captured application links; the concurrently edited root application's full
build and launcher delivery remain separate checks.

## Historical implementation record — 2026-10-07

2026-10-07 historical status. The goal was the complete original campaign. **No campaign mission
was then claimed complete or playable end to end.** Fourteen of 15
authored map entry points still encounter unsupported services in the first
60 simulation ticks. Delta Towers passes that window, then stops at weapon
cloning in the 300-tick check. This report distinguishes implemented services
from the remaining campaign work.

## Implemented this pass

* Added pending `switch_bsp` and `switch_bsp_by_name` requests using their
  original compiled ordinals. The tick boundary replaces collision/navigation
  data while preserving the interpreter, globals, game clock and player state.
  The renderer replaces static geometry, lightmaps and sky, and rebases shared
  model materials. Requesting the current BSP cancels an earlier request;
  invalid ordinals are ignored.
* Added cross-BSP checkpoint restoration. The saved immutable collision and
  navigation world is restored before the saved VM/world continuation, pending
  switches are cancelled, and the renderer follows the restored BSP revision.
* Added scene placement/lifetime policy using authored origin/manual BSP masks,
  sorted transition-volume pairs, strict interior containment, and persistent
  versus automatic placements. Named persistent objects retain handles while
  hidden, and script visibility survives reactivation. Missing eligible
  automatic scenery is created on adoption. General parent trees and attached
  trigger-node transforms remain unfinished.
* Connected `sound_impulse_predict`, `predict_model_section` and
  `predict_structure_section` to retained native assets. As updated on 2026-10-14, sound prediction retains the first compressed chunk;
  full PCM is decoded by the existing playback path on demand. Model prediction retains every section/PRT payload
  and every referenced material bitmap record; BSP prediction retains every
  cluster/instance-definition geometry payload. Cluster prediction also retains
  dependent models and indexed bitmap payloads, including volume/vector formats
  and all three authored cache LOD byte ranges. This backend loads whole native assets
  synchronously; it does not reproduce Xbox chunk queues, cache eviction,
  or object/instance lightmap bucket residency. Rendering still selects its
  currently supported display geometry/formats separately.
* Added original 54-slot sound-class names and substring-selected dB fades to
  the live SDL mixer. Class identity survives start/loop/end concatenation;
  existing and future voices use current gain. Fade clocks follow wall/audio
  time and checkpoint restoration retains in-flight fade state. Ducking,
  DirectSound quantization, saved voice cursors and full spatial playback
  remain unfinished.
* Added original five objective states, cooperative-mode query for the actual
  solo backend, flashlight availability/change tick, cinematic ambience-detail
  enable state and one-shot ambience-rebuild suppression state. Objectives
  still need their pause-menu text/checkbox UI; flashlight input/rendering and
  ambient-detail scheduling/portal rebuild remain unfinished.
* Added type-mask cinematic object cleanup, retaining native player ownership
  and stable actor indices across checkpoint restore. Stored cinematic LOD
  flags in scene objects; the current model backend already admits its highest
  LOD without a distance fade. General parenting/protection and original LOD
  selection remain unfinished.
* Corrected `sleep_until` to match original `2089B0/2099F0`: queue a nonliteral
  predicate, sleep, then evaluate it. Literal/global predicates are sampled
  immediately. Timeout zero bypasses predicate evaluation. Poll/timeout duration
  conversion uses the original float32 seconds/rate calculation and nearest-even
  rounding at the native 30 Hz game frequency.
* Added immutable VM checkpoints containing globals, all threads, nested
  expression frames, polling state and wake deadlines. Restore is permitted only
  outside dispatch and only for the same script program.
* Implemented `cinematic_skip_start_internal`, `cinematic_skip_stop_internal`,
  `game_save_cinematic_skip`, `game_saving`, `game_reverted` and `game_revert`.
  Save requests are captured after world updates and the game-clock increment.
  Restore resumes the saved continuation at the restored tick before increment;
  the reverted signal is a timestamp comparison, as in the original.
* Restored native player/actor vitality, movement and weapon state, active
  projectiles, random generator, allegiances, scene object generations and names,
  camera/transition state, input flags, mission segment and game counters.
  A restore requested inside a builtin stops further VM dispatch before applying
  the world restore. Observed script faults remain in diagnostic history.
* Added Escape to request a cinematic skip when a recoverable checkpoint exists.
  Repeated held input cannot repeatedly restore the same checkpoint. The native
  input bridge does not establish physical Xbox controller parity.
* Corrected player `object_hide` handling and implemented the player-list damage
  protection used by Cairo's unchanged stash/unstash helpers. Object vitality
  immunity and unit reaction/impulse immunity are separate flags. Movement
  disabling is independent of general player input disabling.
* Added external-global access with explicit rejection of unknown services.
  The verified null-backed `cinematic_letterbox_style` short ignores writes and
  refreshes reads to zero, including Cairo's compiled long expression with
  conversion at the declared short binding. Texture/geometry flushes are
  admitted because their original evaluators really are return-only operations.
* Discarded-timeline audio voices are stopped, and saved scripted loops/alternate
  selection are restarted. This is native audio restoration, not a claim of
  original mixer cursor, ambience or module callback parity.

## Evidence and reproducible checks

The authority is the local original XBE, SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`,
and the authored nodes in `01b_spacestation.map`. Primary instruction analysis:
[skip controller](CAMPAIGN_SKIP_RESEARCH.md),
[BSP lifetime and commit](CAMPAIGN_BSP_RESEARCH.md),
[audio classes and saved state](CAMPAIGN_AUDIO_RESEARCH.md),
[asset residency](CAMPAIGN_RESIDENCY_RESEARCH.md),
[actor/scenery animation playback](CAMPAIGN_ANIMATION_PLAYBACK.md),
[animation and supporting services](CAMPAIGN_ANIMATION_RESEARCH.md), and
[earlier Cairo service research](CAMPAIGN_RESEARCH.md).

The original-instruction fixture in
`tools/oracle_campaign_skip.py` executes Cairo's unchanged static
skip-start/stop helpers with the original HS interpreter and checkpoint
controller. It supplies storage, BSP/header compatibility and unrelated module
callbacks. Both natural completion and admitted skip input restore a pending
predicate at the saved tick, return false from skip-start, and reach skip-stop
without a second revert. This is bounded script/controller proof, not a full
emulated console playthrough.
`tools/oracle_campaign_skip_edges.py` separately checks 66 original controller
edge cases; the complete input/cache/module bridge is still incomplete.

`tests/campaign_tests.cpp` independently exercises the native implementation:
nested VM continuation restore, polling/global/timeout boundaries, unsafe and
cross-program rejection, post-call restore suspension, object generations,
presentation transitions, both cinematic completion routes, real Cairo compiled
skip/stash/unstash helpers, protection distinctions, audio cleanup and retained
fault history. A combat replay also checks identical movement, AI/weapon state,
projectile positions and random firing spread after restoring a checkpoint.
It verifies replay determinism of the current native systems; it does not prove
retail Havok or actor behavior parity.

`tools/oracle_campaign_bsp.py` passes 143 unchanged-XBE request, commit,
lifetime, placement and cross-BSP restore cases. Its external resource and
object effects are fixture adapters, not a console playthrough. Native tests
independently verify changing collision floors, continued VM execution,
revert/cancellation, stable generations and visibility, original Cairo audio
classes and retained payloads, and actual offline SDL mixing across class
phases. The audio/channel instruction fixtures run through
`tools/oracle_campaign_audio.py` and `tools/oracle_campaign_animation.py`.
Animation research is evidence for the next controller work; native actor
relative-animation playback has not been implemented by this pass.

```sh
cmake -S . -B build-campaign -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build-campaign -j 8
ctest --test-dir build-campaign --output-on-failure
python3 tools/audit_campaign.py --native build-campaign/halo2_native
```

The audit deliberately returns failure while any authored startup faults. Its
JSON records runtime/map hashes, tick count, exit status and actual diagnostics.
The baseline is `analysis/campaign/startup-before.json`; the current report is
`analysis/campaign/startup-latest.json`. A successful short startup window would
still not prove mission completion.

Delivery verification: all 26 normal CTest groups and four focused
address/undefined sanitizer groups passed. The rebuilt title-first launcher
rendered 30 title frames on the Linux RTX 5080 with synchronization validation
enabled and no Vulkan validation messages. A six-frame Vulkan replay also
loaded Cairo BSP 0, switched to BSP 1 and restored the checkpoint to BSP 0,
with one restore, revision 2 and no script faults or validation messages.
These GPU checks cover title startup and resource replacement; they do not
cover authored mission progression or full cutscenes.

Use the root `Halo2-Title-Launcher` or the refreshed **Halo 2 Native (Title)**
desktop shortcut. Its runtime is `build-campaign/halo2_native`; keep that build
and the project assets with the launcher. Checksums/platform/validation limits:
[build manifest](../analysis/campaign/build-manifest.json),
[SHA-256 list](../analysis/campaign/build.sha256). Test evidence:
`analysis/campaign/{ctest-delivery.log,asan-delivery-ctest.log,
launcher-title.log,launcher-title.png,bsp-replay.log,bsp-replay.png,
residency-tests.log,deltatowers-300.log}`.

## Remaining campaign dependencies

| Area | Current blocker or required behavior |
| --- | --- |
| Mission entry and level streaming | Complete parent/source protection, attached transition volumes, actor admission/lifecycle, granular resource residency, later map handoffs and authored streaming paths. |
| Cinematics | Actor/scenery relative animation, live anchor node transforms, non-looping playback, interpolation, custom-duration queries, variants/permutations, attachments, weapon cloning, cinematic lighting services and camera/object synchronization. |
| Sound | Ambient details/portal rebuild, ducking, dialogue timing/localization, full cache residency, restore lifecycle and saved mixer positions. |
| Encounters | Complete AI expressions/groups/orders/command scripts, dynamic navigation, original combat status, squad/actor object handles and authored encounter conditions. |
| Sandbox | Vehicles, equipment, grenades, melee, dual wield, weapon interactions, damage attachments and original physics/AI behavior. |
| Mission progression | Objectives, devices and moving collision, trigger attachments, HUD/messages, checkpoints conditioned on safety, death/restart, persistent disk saves, difficulty/loadouts, mission completion and next-level transitions. |
| Validation | Every mission on authored paths, all difficulties and checkpoint/skip/death routes, dialogue/audio, gameplay and visual checks; original-console comparisons and full playthrough evidence. |

Current Cairo startup reaches `object_set_permutation` after saving/stashing
players, predicting assets and cleaning up objects. The tutorial next requires
`hud_enable_training`; the prologue next requires `cinematic_outro_start`.
Delta Towers reaches `cinematic_clone_players_weapon` in the longer check.
All other actual entry failures are retained in the audit. Shader and
lighting implementation remains in the user's separate conversation.

The current checkpoint is in memory, scoped to the same map and implemented
native subsystems. It is not compatible with Xbox save files and is not a disk
save system. General safe-save/restart, map handoffs and complete object-list
handling remain explicit dependencies.

## 2026-10-14 actor lifecycle validation

The active campaign goal remains unfinished. The new source actor lifecycle now
handles linked squad/global membership, joint/observation/tracking cleanup,
clumps, dynamic firing points, swarms, command participants, HS thread retirement,
unit detachment and original counters. Non-NULL slot callbacks and a nonempty
followed-reference registry still require their actual owners. No fallback success
or separate tag decoder is supplied.

384 whole-original lifecycle cases match in both owned native pools and actual
source-backed bank views.288 unit binding/unbinding cases also match through
cold actor retirement. Constructor, control and recursive-seat regressions pass.
The eight final native jobs ran serially with zero swap; their maximum measured
peak was36,753,408bytes. Original-reference jobs peaked at62,914,560bytes, below
the64MiB ceiling. The component builds directly from captured current sources;
this stage did not link the full current application or refresh its launchers.

These actor/control owners are not connected to Simulation yet. Complete source
object/actor birth, admission, activation and shared ownership are prerequisites.
The sealed campaign runtime still has Armory completion/checkpoint evidence and
14 explicit failed mission entries; this component work does not establish new
mission completion. All previous runtime/source evidence pins remain unchanged.
Reference execution uses the base retail CPU image and is not TU1.5 visual proof.
Details and reproduction: [lifecycle results](../analysis/tags/gameplay-validation-20261014/campaign-ai-lifecycle/RESULTS.md).

The subsequent actor-control stage completes retail eligibility, activation,
clump wake and team propagation.256 whole-original cases match in both owned
storage and a borrowed source actor bank. Binding and teardown now also pass
through the bank's public API against the same backing. All12 serial component
checks pass; final native peak25,055,232bytes, zero swap. The source-bank view
does not allocate or copy records and must be reconstructed after whole-bank
backing replacement. Whole birth/admission, squad membership, world checkpoint
integration and campaign wiring remain open. No additional mission completion
is claimed. [Actor-control results](../analysis/tags/gameplay-validation-20261014/campaign-ai-birth/RESULTS.md).

Squad join/activation, global membership and location-reference reset now share
the actor bank API and original unlink owners with teardown. All 256 new
whole-original membership cases match in owned and source-backed storage;
all 14 final serial component jobs pass. Their peak was 35,536,896 bytes,
zero swap. The original-reference peak was 62,914,560 bytes. The captured
six-executable cohort and earlier evidence are sealed without changing old
results. Full birth/admission, source-world startup RAM and campaign wiring
remain open; Armory remains the only retained authored mission completion.
[Membership results](../analysis/tags/gameplay-validation-20261014/campaign-ai-membership/RESULTS.md).

The source-memory transfer stage removes full temporary copies for validated
single-region reads/writes, retaining cross-region staging and byte/permission/
checkpoint behavior. The actual 40 MiB overlap reproducers now pass at
42,655,744 bytes; their prior versions hit the contained guard. All 16 actor and
resource regressions plus three new guarded CTest cases pass. Armory completion
and checkpoint traces remain identical before/after. Three full serial native
multiplayer simulations also match the prior sealed traces; maximum final peak
64,372,736 bytes, zero swap.

The campaign remains unfinished: 13 other entries still fail on required script
owners, and the current Oracle entry hits a separate BSP-import memory failure.
Equal partial Oracle logs are not parity proof. Current-source Lockout/Cairo/
Armory source-world startup also remains above the contained memory budget.
A fresh root configuration error was fixed by registering multiplayer
continuation after its physics target exists. No launcher update or new mission
completion is claimed. [Memory results](../analysis/tags/gameplay-validation-20261014/campaign-runtime-memory/RESULTS.md).

The subsequent BSP-memory stage restores Oracle's resource admission. The loader
reserves the exact decoded instance triangle total, and forced collision
composition returns freed host allocator buffers at BSP/checkpoint boundaries.
All 65 BSP fingerprints across the 15 campaign maps retain every ordered geometry,
normal/material/identity and navigation field. All 20 final runtime jobs satisfy
the serial 64 MiB measured ceiling and zero swap; maximum peak 64,765,952 bytes.
Armory completion/checkpoint and the three full multiplayer traces remain
identical. Oracle now reaches its genuine unsupported `ai_place` overload.

All 14 other campaign entries still require script/AI owners. Actual source-world
physical-bank startup remains above the budget; allocator reclamation alone did
not fix it. The active goal next needs shared actual backing/checkpoint ownership,
full birth/admission and campaign integration. No new mission completion or
launcher update is claimed. [BSP-memory results](../analysis/tags/gameplay-validation-20261014/campaign-bsp-memory/RESULTS.md).


The physical-backing stage removes the duplicate source copy of the actual host
physical bank. Memory/native addresses and platform protection now share one
retained page lease; checkpoint staging binds that lease into the replacement
before publication. Existing small memory/resource/actor regressions and all six
guarded CTest cases pass. Fresh consistently compiled engine/renderer cohorts
build under the separate 1 GiB, zero-swap compiler cap.

Actual English Xbox TU1.5 Lockout, Cairo and Armory map/world startup now pass
under the simulation ceiling. Lockout additionally births and binds its actual
source player/biped/weapon and completes 180 original controller/world ticks,
including forward movement. Both Armory authored routes and all three full native
multiplayer traces remain byte-identical; the 14 unfinished mission entries retain
their real required-owner failures. Across the 37 final serial jobs, the largest
measured peak is 63,827,968 bytes with zero swap.

Large real arena/world continuation remains unfinished: both tests hit contained
OOM in dense SourceRuntimeMemory::checkpoint copies. The arena requests a
51,265,536-byte copy; the world reaches a 3,611,948-byte copy after ten real ticks.
The next memory work must preserve full snapshot bytes/isolation, native owner
protections and quiescent publication within the existing limit. Actual campaign
actor birth/admission and all remaining authored mission/BSP/checkpoint paths
still require completion. Other chats' later root world/CMake changes were
preserved; current touched API consumers pass syntax checks, but no full current
root application/launcher or campaign-wide behavior/visual parity is claimed.
[Physical-backing results](../analysis/tags/gameplay-validation-20261014/campaign-physical-backing/RESULTS.md).


The snapshot-memory stage replaces large temporary source/arena-file buffers
with independent disk-backed byte snapshots, shared through a small storage
module. Exact byte/hole/protection/ownership behavior remains checked. The old
48 MiB restore and 40 MiB copy OOMs now pass; actual arena continuation passes
original startup, private files/cursors, repeated handoff, rejection/cancellation
and independent owner lifetime. Private physical snapshot mappings are adopted
without a second resident copy, and fresh native files preserve zero ranges as
sparse holes. A 52 MiB live-data adoption stress case fails before and passes
after the change under the same cap.

Current root storage configuration/build, all 12 guarded CTest cases and the
resource/file checks pass. Both Armory routes and all three full multiplayer
traces retain identical outputs. The full fresh engine/renderer cohorts build
under the separate 1 GiB compiler guard; native jobs remain serial under 64 MiB
measured with zero swap.

Real world continuation still hits contained OOM. Export and canceled preparation
now complete; a subsequent malformed-import preparation reaches the redundant
full-bank checkpoint used only for immutable-image comparison, increasing anon
RSS 53,772 -> 55,660 KiB before failure. The next memory work must compare immutable
regions exactly without that temporary snapshot, then finish the complete world
continuation test. The 14 remaining mission entries, actual campaign birth and
all authored mission/BSP/checkpoint paths remain unfinished. No current full root
app/launcher or campaign-wide behavior/visual parity is claimed.
[Snapshot-memory results](../analysis/tags/gameplay-validation-20261014/campaign-snapshot-memory/RESULTS.md).


The immutable-image admission stage removes the redundant whole live-bank
checkpoint from world import. Exact byte/metadata/topology comparisons now run
without heap allocation; all 22 legacy decisions match. The unchanged complete
arena and world continuation tests pass, including rollback, repeated restore,
salted identities, clock rebasing and every-byte equality over 60 later original
controller/physics ticks. All 13 current-root guarded storage checks and both
resource/file checks pass.

Across 36 final serial jobs, 22 pass and 14 retain genuine campaign-owner failures;
maximum measured RAM is 65,019,904 bytes with zero swap. Both Armory routes and all
three full multiplayer traces remain byte-identical. Actual campaign birth,
remaining script owners and every mission/BSP/checkpoint path are still unfinished.
Current concurrent world/AI changes were preserved. No full current application,
launcher delivery or campaign-wide console parity is claimed.
[Immutable-memory results](../analysis/tags/gameplay-validation-20261014/campaign-immutable-memory/RESULTS.md).


The kill-volume stage fixes a campaign tag-import omission: the existing decoder
now retains the authored kill-trigger list for every scenario. The prior importer
fails Cairo's list comparison; all 15 actual campaign maps and two multiplayer
maps pass afterward, preserving all 95 campaign kill rows. Current-root content
syntax and the independent module CMake build/guarded CTest pass.

The new borrowed SourceKillVolumeView matches 256 paired base/TU1.5 original
state/scheduling cases in both owned and source-bank views, including full bytes,
required containment calls and checkpoint rebinding. It uses one existing record;
HS argument/result and containment callees are declared in the original fixture.
Real geometry/fatal damage and Simulation HS integration remain unfinished.

Across 39 final serial reference/native jobs, 25 pass and 14 retain actual missing
mission-owner failures. Maximum measured RAM is 64,479,232 bytes with zero swap.
Armory completion/checkpoint and all three complete multiplayer traces remain
byte-identical. No new mission completion, current full application/launcher
delivery or original-console parity is claimed. The full campaign goal remains
active. [Kill-volume results](../analysis/tags/gameplay-validation-20261014/campaign-kill-volumes/RESULTS.md).

The actual campaign-world stage adds typed difficulty/options, original position
delivery and kill-volume controls to the retained source world. A proper Cairo
player now completes original geometry containment, disabled-volume checkpoint
restoration and fatal damage through its ordinary world tick. The reached team
table was missing; the world now invokes the existing complete original row-18
constructor and map reset. No body, health, death or containment result is
assigned as a replacement. Position inside the authored volume is a declared
diagnostic input rather than a natural traversal test.

The unchanged complete world continuation passes rollback, repeated restore,
salted identities, clock rebasing and every-byte equality across 60 later actual
controller/physics ticks. A fresh consistent current-source engine cohort and
rebuilt header consumers pass; native checks remain serial under the 64 MiB
measured ceiling with zero swap. Actual AI lifecycle, campaign Simulation/HS
wiring and all remaining mission/BSP/checkpoint paths are still unfinished.
No full application/launcher or console-parity claim is made. The full campaign
goal remains active. [Actual world results](../analysis/tags/gameplay-validation-20261014/campaign-live-owners/RESULTS.md).

The AI lifecycle stage replaces the manual allocation prefix with complete
original 1CFA40 and runs complete 1CFC90 after the source game options, clock and
engine reset. The readiness regression fails before and passes after the reset
call. The published Cairo player/geometry/checkpoint/fatal-damage test and the
full 60-tick world continuation pass with the original campaign AI frame enabled.
This fixture has no NPC actors and does not prove their birth or decisions.

Both Armory routes and all three full multiplayer traces remain byte-identical.
Across 21 final serial jobs, seven pass and 14 retain the same actual mission
owner failures. Maximum measured RAM is 64,569,344 bytes, with zero swap.
Published files pass current-root syntax checks while preserving six concurrent
header changes. Source campaign Simulation/HS integration, actual NPC birth and
all remaining mission/BSP/checkpoint paths stay open; the goal remains active.
[AI lifecycle results](../analysis/tags/gameplay-validation-20261014/campaign-ai-world-startup/RESULTS.md).

The private AI/HS integration proof now completes original counted placement of
Cairo's authored Johnson squad, validates actual actor/unit/command/HS-thread
bindings, restores whole authority with correctly staged pool views, and passes
an ordinary campaign AI/object tick. It recovers 55 complete original routines
and reuses three existing frame helpers without duplicate generated bodies.
The accepted serial job peaks at 62,914,560 bytes with zero swap.

This backend remains isolated from root pending a clean public interface and
broader validation. The legacy campaign runner still has 14 required-owner entry
failures; no new full mission, broad actor matrix or console-parity claim is made.
The full campaign goal remains active. [Private AI/HS proof](../analysis/tags/gameplay-validation-20261014/campaign-ai-script-entry/RESULTS.md).

The public campaign world stage now exposes original AI placement through a
readable typed interface. Its canonical backend contains 55 complete original
squad/command/HS routines and reuses three existing frame helpers. Actual Cairo
counted, authored and packed starting-location placement passes actor/unit/
command/HS-thread binding, whole authority restoration and bounded ordinary
AI/object ticks (3/1/1). The original overload comparison covers 256 cases.

Campaign kill-volume/fatal-damage, full 60-tick world continuation and the
180-tick multiplayer world smoke test pass. Both Armory routes and all three
match traces remain byte-identical. Across 29 final serial jobs, 15 pass and
14 retain the same actual mission-entry failures. Maximum RAM is 64,131,072
bytes, below 64 MiB, with zero swap. A fresh internally consistent source cohort
builds; focused current-root syntax preserves 11 concurrent header changes.

The original HS frame scheduler and campaign frontend/player/object lifetime
integration remain unfinished. Thread creation and linkage do not prove full
command-script execution. All remaining mission/BSP/checkpoint/restart/death,
navigation, combat/hit-region and presentation paths stay open. The full campaign
goal remains active. [Public world results](../analysis/tags/gameplay-validation-20261014/campaign-world-api/RESULTS.md).
