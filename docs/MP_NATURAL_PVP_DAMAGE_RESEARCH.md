# Natural actual-map PvP damage audit

The current native damage path is not generally immune: the real-SDL four-player case naturally damages, kills, scores and respawns a participant. In addition, an unchanged four-player Lockout match, driven only by external controls, admits 25 contacts, kills a player, and later respawns that player. Its complete CPU output and observer contact log repeat byte-identically. The final isolated raw-device run admits first shield damage at published tick 331, lethal damage at published tick 502, and successful respawn at published tick 674. Contact observations occur before the tick increment (simulation ticks330/501); these are distinct recorded time fields. Tick674 is an observed successful spawn, not an inferred fixed respawn delay. This does not reproduce the user's exact encounter or establish original posed-model/Havok hit-finding parity.

## Scope and retained authority

The probes load the real Lockout cache (`14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`) and authenticated u5English gameplay executable (`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`). They link the final `build-kbm-20261008-184611` checkpoint's actual cache/systems/renderer/runtime components. Its final release manifest records native SHA `7ae2f10a2cfe1aa19a80fe80629ac5b5651eb4580fa5a1b8db92d6cbb1879af6` and source manifest SHA `71da05194ae860ee9c7eca4cf6a9b541e2ea50c1cfaedd282ef1d457da73704c`. Older pre-final binary pins are not used as current authority. The new report pins each actual linked input and executable separately.

The probes do not alter a target, body root, velocity, vitality, grounded flag, collision collection, weapon readiness, source owner, history, pose, or current unit. All participants enter through `MultiplayerMatch::join`; the real authored start selection and stock SMG loadout are retained. Declared host control inputs are the only behavioral stimulus. The existing native triangle/capsule movement and fixed 30Hz scheduling remain explicit native boundaries, not Xbox/Havok producers. No source field is assigned to bypass those boundaries.

The optional observed simulation is a separate ignored copy of the checkpoint's `simulation.cpp`, with only stream observations added around actual contact, vitality result, and aftermath. Production and release files are untouched. Both stdout and the observation stream independently repeat; observed stdout equals the uninstrumented CPU case byte-for-byte. Its diff is preserved with the report.

## Completed measurements

| Case | Actual delivery and result | Qualification |
|---|---|---|
| Four players, CPU, shooter 3, 2400 ticks | First contact tick928; lethal contact tick1022; 25 contacts at death; ordinary respawn subsequently observed | External `Controls` provider, no SDL delivery claim; repeated stdout and contact stream |
| Four views, real isolated SDL, owned host waypoint input, 900 ticks | First fire327; first shield damage331; death502; shooter kills1/score1 and victim deaths1; new full-unit identity at respawn674 | Actual trigger/reload input, 307 frames, one measured SDL execution; native collision/clock boundaries explicit |
| Four views, isolated SDL, 1350 ticks | Real forward/look/trigger delivery; first fire1317; first target shield damage1347; shield1 becomes0.933928549 | Capture ends before a kill window; harness exit1 is not immunity |
| Four views, isolated SDL, changed walking gain, 1800 ticks | No clear firing line found; no fire; loaded60 and unchanged vitality | Failed input scenario, not an admitted damage failure |
| One participant | No PvP target exists | Cannot establish player-versus-player damage |
| Two participants, walking paths | Tested paths stay world-occluded and do not fire | No damage verdict from these attempts |
| Two participants, periodic native jump/heading variants | Several actual contacts decrease player 1 shield without changing its state; no sustained lethal contact case | Qualified native jump/movement; original Havok jump producer is not established |

The CPU lethal contact retains attacker player `EC730003`, captured attacker unit `E2750006`, designator3, and target unit `E2710002`. In the repeated CPU case, the actual first capsule contact distance3.73727894 precedes real world triangle44 at 6.74451923. Damage uses imported postprocessed SMG amount4.625. The initial normalized shield request is0.0660714284; the lethal request is body0.102777779, with body=-0.0138890743/shield0. The synchronous tail leaves the retained dead unit's current owner NONE and previous owner `EC710001`. These are observations of the real native path, not supplied expected values.

The 4.625 amount is supported independently by the complete original [combat postprocessor](MP_COMBAT_TAG_POSTPROCESS_UPDATES_RESEARCH.md): original 1.1 `164750` and 1.5 `165B30` both change raw-cache SMG endpoints4.75 to bits40940000/4.625. This is not a 1.5-only adjustment. The existing [damage/history](MP_SMG_DAMAGE_HISTORY_RESEARCH.md), [direct-impact/default multiplier](MP_SMG_EVENT_STRENGTH_RESEARCH.md) and [lethal attribution](MP_LETHAL_ATTRIBUTION_RESEARCH.md) contracts remain bounded original-kernel authority; the new native probes do not replace or widen them.

## Actual raw-device delivery and isolation

The SDL probe uses real SDL virtual game-controller APIs, standard raw button/axis records, and the actual checkpoint runtime event/polling path. The host script observes published positions only to choose its external walk/look/fire intent. This strategy is not claimed as original player behavior. It neither supplies a world hit nor bypasses weapon or damage admission.

`SDL_VIDEODRIVER=x11` plus a private Xvfb display isolates the window. The test-only `SDL_HINT_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT="0x0000/0x0000"` is set before SDL initialization, and the probe requires zero recognized existing controllers. Its own virtual devices actually report VID/PID0. The installed primary SDL2 header documents this whitelist contract. Background joystick delivery is explicitly enabled for the test. It does not change the user's hardware configuration. The log retains actual video driver, controller inventory, delivered commands, source identities, shield/body/death/counters, and screenshot dimensions.

Earlier exploratory runs are retained separately as rejected provider setups. One addressed a fifth controller ordinal outside the native four-view assignment. Another used two existing controllers; when one ceased to be present, the observed owned virtual pad moved from ordinal2 to1 and its movement delivery moved from view3 to view2. `Platform::refresh` actually erases detached pads and the runtime binds views by current vector index. This is a concrete native ownership instability, not evidence for an original Xbox controller policy or permission to invent one. Those mixed-provider captures also changed physical window dimensions; they are not isolated PvP evidence. The corrected capture is640x480.

## Concrete live-path and coverage limits

- `Simulation::trace` raycasts real authored world triangles, then compares native analytic capsules in `ray_body`. No live caller invokes `posed_collision`; the existing posed collision tests use declared captured matrix/permutation/ray inputs. Thus current damage admission does not consume the actor's authoritative current posed collision regions.
- `shot` uses imported standing/crouch camera-height eye, not an owned current weapon muzzle marker. `apply_unit_damage` deliberately takes the null model-region path; source headshot/region selection is not connected.
- SDL KBM/runtime and animation tests prove movement/action delivery, magazine decrement/reload and presentation, but do not assert a naturally spawned target's shield/body/death. `smg_damage_tests` clears collision, relocates participants and supplies vitality/stun fields; `lethal_match_tests` additionally supplies radius/history. They establish downstream arithmetic/ownership, not natural actual-map hit admission.
- The existing body renderer samples the available idle clip; the live persistent source command/physical/classification/channel/current-bank producer is incomplete. The [command/locomotion owner report](MP_COMMAND_LOCOMOTION_POSE_RESEARCH.md) identifies the smallest next source seam: admitted `151AA0/151C90`, including `151980/30BD0/E7B10`, default7C constructor/copy, deadzone/request/classifier and owned primary/secondary/local/current banks. Its genuine source-created physical-state1 is a literal one-update case; native grounded must not be promoted to that source state.

A source-faithful live hit integration therefore requires the genuine authoritative object-phase current world matrices and active collision permutations/region selection, then the existing posed collision kernel and original damage-region inputs. Render-time fractional pose sampling is presentation only and cannot become collision authority. Full current-pose/contact/device/readiness scheduling remains open. The present positive result rules out blanket immunity in this pinned native case; it does not identify the cause of the user's unrecorded encounter.

## Reproduction and immutable evidence

[Complete report and individual pins](../analysis/multiplayer/integration/spawn-source/natural-pvp-diagnosis/report.json), [final linked inputs](../analysis/multiplayer/integration/spawn-source/natural-pvp-diagnosis/final-input-pins.json), [observer-only diff](../analysis/multiplayer/integration/spawn-source/natural-pvp-diagnosis/simulation-observer-only.diff), [actual delivered commands](../analysis/multiplayer/integration/spawn-source/natural-pvp-diagnosis/sdl-four-waypoints.tsv), [complete actual roster](../analysis/multiplayer/integration/spawn-source/natural-pvp-diagnosis/sdl-four-waypoints-roster.tsv), [contact/normalized/aftermath log](../analysis/multiplayer/integration/spawn-source/natural-pvp-diagnosis/sdl-four-waypoints-contacts.log), [actual capture](../analysis/multiplayer/integration/spawn-source/natural-pvp-diagnosis/sdl-four-waypoints.png).

The ignored diagnostic directory retains every failed scenario/provider log as well as the completed positive. Earlier exploratory logs are not assumed to match the final source version. The positive's exact executable is `sdl_probe_waypoints`, built from its separately pinned source plus the observation-only simulation object and final checkpoint components. No earlier freeze or production file was edited.

The rapid CPU loop verifies the complete repeated contact stream and stdout. It ran successfully after creation:

```bash
analysis/multiplayer/integration/spawn-source/natural-pvp-diagnosis/reproduce-cpu.sh
```

The following uses actual SDL raw devices and the unchanged native runtime; it requires a free GPU slot. Pass a fresh output prefix; the script refuses to replace existing artifacts. The measured positive is one execution, not a claimed byte-identical GPU/time replay:

```bash
analysis/multiplayer/integration/spawn-source/natural-pvp-diagnosis/reproduce-sdl.sh \
  analysis/multiplayer/integration/spawn-source/natural-pvp-diagnosis/fresh-sdl-reproduction
```

The positive log contains no Vulkan validation errors. It records X11, zero existing recognized controllers, three owned virtual pads and 640x480 output. Full source-owned Xbox hit-finding, input/session readiness, world startup and rendering parity remain unproved; this evidence does not identify the cause of an unrecorded user encounter.
