# Campaign implementation status

2026-10-07. The goal is the complete original campaign. **No campaign mission
can currently be claimed complete or playable end to end.** Fourteen of 15
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
  `predict_structure_section` to retained native assets. Playback reuses
  predicted decoded PCM. Model prediction retains every section/PRT payload
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
