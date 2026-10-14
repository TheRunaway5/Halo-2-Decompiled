# Tag migration gameplay validation

**Current priority, 2026-10-13:** Work on Halo 2 multiplayer first, as explicitly
requested by the user. Validate the migrated tag system in complete local and
networked matches, fix regressions against original evidence, preserve authored
map and variant behavior, and deliver the tested runtime with reproducible
validation. This includes spawning, combat/objectives, scoring, respawns,
HUD/results and normal match completion. Campaign work is deferred until the
multiplayer work is finished. The work remains incomplete. `priority.json`
retains the working order and `completion-audit.json` records the remaining
requirements. Work continues under the user's explicit resume instruction;
the goal widget still reports `blocked`, and the available goal tools cannot
set an existing goal back to active.

The 2026-10-13 continuation fixes a concrete multiplayer import failure:
Headlong's seven unused vehicle placements have palette index NONE, but the
vehicle attachment-template importer attempted to read their tag payloads.
The caller now skips those empty templates while preserving the authored rows
and nine valid vehicle templates. The shared tag API still rejects NONE.
Unchanged retail `BF0F0` independently rejects all seven original placement
records before construction. The new `tag_vehicle_template_tests` fails on the
old caller and passes with the guard; both previously failing device suites
also pass. The reusable oracle is `tools/oracle_empty_vehicle_placements.py`.

`analysis/tags/gameplay-validation-20261013/` retains the new source snapshot,
original-executable evidence, red/green regression, complete build and ten
passing focused CTests without skips. A separate isolated guard candidate
imports all 74 Xbox/Vista gameplay caches using their actual scenario type and
an explicitly supplied owned retail XBE. That survey covers BSP0 import, not
all-map gameplay. The production native executable also renders Headlong for
180 frames and 81 ticks at 1280x720; its screenshot and exact binary hash are
retained. The earlier zero-monitor failure is preserved separately; the user
restored the desktop before the successful visible run.

The first fresh full match exposed a validation build configuration problem:
the preserved asset registry still pointed to paths moved by the cache reset,
so the build omitted the existing raw light shader catalog. The match failed
during combat with `Raw sphere diffuse catalog was not built`. Only the private
validation asset bindings were corrected; the renderer's rejection was kept.
The rebuilt catalog passes its reference test, and the corrected build completes
a full ordinary-time local Slayer match, 3–1: 1,639 ticks, 4,082 rendered frames,
497 fired rounds, 1,035 audio packets, 180,416 particle draws and 981 results
draws. `asset-binding-repair.json`, `catalog-build.json` and
`coagulation-ffa-catalog.json` record this sequence and the unchanged binaries.
The fresh independent host/client Team Slayer match also passes, 3–1, with
1,651 ticks and identical final records. The host submits 4,576 frames and the
client 4,472; both complete the results countdown. The report is
`two-process-catalog/report.json`. Campaign findings below remain historical
evidence and are deferred under the current priority. Multiplayer coverage
outside these exercised Slayer paths remains incomplete.

The same pinned build completes Headlong Slayer, 2–1 at the time limit, with
2,910 ticks, 7,097 submitted frames and 114 fired rounds. This verifies the
match workflow after the empty-template fix, but its trace exposes a player
falling through scene collision and remaining alive below the map. The actual
kill volumes do not cover that location. A separate 60-step capsule diagnostic
reproduces the fall with composite scene collision in both the pre-migration
and migrated builds; every output word agrees. The same diagnostic stays
supported with BSP collision alone. This establishes a pre-existing native
composite collision defect, not an original contact-physics implementation.
No speculative kill height or collision workaround was added. The exact
commands, hashes and limits are in
`additional-maps/headlong-collision-comparison.json`.

The fresh variant audit performs all 78 Split Screen/System Link menu start
actions: two of the 39 authored variants are admitted and 37 remain explicitly
blocked. Both admitted variants complete bounded four-player and native UDP
startup checks. `multiplayer-variants.json` retains the individual blockers;
the strict all-admitted run correctly exits with failure. These checks do not
establish full gameplay for the unavailable variants.

The additional Lockout attempt reaches 2,910 ticks, 10,670 submitted frames and
the completed results countdown, but the controller driver never obtains a
line of fire across the map's levels. It fires no rounds and fails the required
combat coverage assertion. The failure is retained as
`additional-maps/lockout.json`; it is not counted as a passed match validation
or classified as an engine regression without further evidence.

After the explicit resume request, the Headlong capsule failure was reduced to
the actual floor and adjacent prop/wall contacts. The old solver spent its
entire eight-correction limit alternating between wall surfaces, starving the
floor response. Each solver pass now processes every candidate using the
corrected capsule. The new `scene_capsule_contact_tests` verifies floor surface
3666 through the recovered original terrain query and retains support for all
60 reproduced steps. Both the failing test and the scene-contact diagnostic
remain under `collision-diagnosis/`.

The next complete Headlong match retained the floor but exposed a landing
footstep event with marker ordinal 2 beyond the authored two-marker bank.
Unchanged TU1.1 `E63C0` chooses the unit's world position through `BACC0` when
the ordinal is outside the bank or the posed lookup fails. The adapter now
shares this fallback through `authored_footstep_position`; valid posed markers
keep their original path. `tools/oracle_footstep_position_fallback.py` executes
three unchanged position-selection prefixes with actual cache metadata and
declared raw caller/unit records. It stops before PVS/material/sound dispatch
and does not claim original live-unit allocation. The native regression fails
with the old adapter and passes with the fallback.

The fresh `collision-fix/build` production build passes seventeen focused tests
without skips, covering the new regressions, original terrain/scenery,
multiplayer/LAN startup, footsteps/animations, shared tags, prediction and
device/machine imports. The complete Lockout local match now passes, 0–1, with
581 fired rounds; the independent network match passes, 2–1, with identical
final records and 215/226 fired rounds. These runs use an improved test-only
SDL controller: Lockout has no authored AI navigation sectors, so the driver
reads collision to find reachable firing positions. It never supplies game
state. Headlong exposed an overly expensive large-map search and a controller
stall masked by repeated jumping; failed attempts remain retained. The driver
now bounds that search and measures horizontal progress. Both runners reject
gait-owner errors even when the native match otherwise reaches results.

All six final driver runs pass on the unchanged fixed production engine:

| Map | Local Slayer | Independent native UDP Team Slayer | Fired rounds local / host / client |
| --- | --- | --- | ---: |
| Headlong | 1–3, score limit | 2–2, time limit | 348 / 162 / 157 |
| Lockout | 2–0, time limit | 0–1, time limit | 402 / 238 / 232 |
| Coagulation | 3–2, score limit | 3–2, score limit | 520 / 322 / 345 |

Each network pair agrees on final player/unit/weapon identities and combat
records. All cases reach original results/countdown completion with live
rendering, audio, particles, combat and salted respawns. The final unwrapped
production executable also renders Headlong for 180 frames with two local
views. Its SHA-256 is
`806b73bbd81e028aa9455b2c550f22625e8e838698f6cf9ee06413dc0cd3e634`.
`collision-fix/report.json`, `REPRODUCE.md`, `final-source-sha256.json`,
`final-source-delta.json`, `final-asset-pins.json` and `final-pin-recheck.json`
retain the exact evidence. All 11,193 candidate source files recheck unchanged.
The installed launchers still select the independently changing root build,
so this validation invokes the pinned executable with explicit owned sources.

The subsequent four-player driver also completes Coagulation FFA and Team
Slayer with four rendered local views, ordinary SDL controller input, actual
combat and salted respawns. FFA reaches 2,910 ticks with individual scores
1,3,5,3 and 1,284 fired rounds. Team Slayer reaches 2,911 ticks with individual
scores 2,1,0,2 and 920 fired rounds. These are individual scores, not team totals.
The pinned engine is unchanged. Both reports and the separately built driver
are retained under `four-player-validation/`.

The following sixteen-process experiment exhausted host memory while other
Codex threads were active. The user reported the desktop/kernel memory-shortage
termination; no per-process memory peaks had been recorded. None of the sixteen
processes produced complete final records, and this experiment is a failure.
All owned test processes have stopped. Its logs are retained under
`full-roster-validation/coagulation-team/`; its uncontained launcher is disabled.

Following the user's original-Xbox-memory suggestion, both current match
runners now enforce **64 MiB of kernel-accounted host RAM for the entire job**,
including the runner and every child together, with **zero swap**. A shared lock
and fixed systemd unit permit only one match job at a time. Linux/systemd cgroup
verification is mandatory; unavailable controls stop the test. The budget is
never raised automatically. Runtime deadlines and group teardown apply even
when a child fails. GPU/driver memory is separate from this host cgroup budget;
this is a test containment policy, not an emulation of Xbox memory ownership.

`tools/tag_match_memory_budget.py` retains the kernel's peak and exit reason in
`memory-budget.json`. Four live containment checks verify actual kernel limits,
aggregate child pressure, group cleanup, refusal of a concurrent job, and
rejection of a forged environment marker. The single native startup check
reaches the 64 MiB cap and is killed within that group before gameplay begins;
its recorded peak is 67,108,864 bytes and swap allowance is zero. The host stays
available. This startup is a failed gameplay test, not a completed match.
`memory-guard-validation/` retains the evidence. No larger-budget match was
launched after the user's warning, and no engine behavior or assets were
changed to fit this cap.

Multiplayer work remains incomplete. Coverage now includes two-player local
matches and two-process native UDP on three maps plus four-player local FFA
and Team Slayer on Coagulation. Broader map/weapon/representation coverage,
rendered sixteen-player matches, the 37 unsupported variants, retail System Link/WAN
and complete original physics/render/audio behavior remain open. The current
rendered PC runtime also does not fit the new 64 MiB test budget. Campaign remains
deferred until the multiplayer work is finished.

## Memory-bounded multiplayer simulation

At the user's request, `tag_multiplayer_simulation_tests` now drives the pinned
production match and shared tag APIs without opening graphics or audio devices.
It imports owned map content, preserves authored collision, spawns, equipment,
weapon/damage rules and world-weapon handling, and supplies only player Controls.
The engine still advances one ordinary 1/30-second tick per step, including its
normal results countdown; the harness does not wait for rendered frames. One
authoritative world is shared by sixteen simulated players. Network cases use
actual loopback UDP with four authority seats and three four-seat peers, and
compare player identities, movement, ammo/combat records, team results and clock
against the authority at every tick. Clients retain snapshots rather than
independently simulating sixteen worlds.

All four final simulations pass with every participant moving, consuming
ammunition and completing a salted respawn:

| Owned map | Mode | Players | Ticks including results | Shots | Kills | Wall seconds |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| Beaver Creek | Offline Slayer | 16 | 5,610 | 10,642 | 267 | 4.790 |
| Beaver Creek | Native UDP Team Slayer | 16 | 3,164 | 2,488 | 54 | 95.840 |
| Coagulation | Offline Slayer | 16 | 5,610 | 7,779 | 98 | 4.488 |
| Coagulation | Native UDP Team Slayer | 16 | 5,610 | 5,698 | 47 | 8.595 |

The UDP cases verify 9,492 and 16,830 complete peer deliveries respectively.
`simulation-validation/report.json` records the exact commands, measurements,
limits and failures. The final driver SHA-256 is
`72532e04e1c231755a0ff188fc74d6f1380ea6ba85cfff011bb56a8d37ea9eeb`.
Its source/compiler/link recipe, fifty unchanged production library pins and
six owned input hashes are under `simulation-validation/final-driver/`.

The new harness initially generated movement-axis values such as 1.00000012,
which the existing transport correctly rejected. It also projected strafe
onto the opposite axis from `Simulation::step_players`, preventing one bot
from firing. The driver now uses the existing PC input bounds and engine's
strafe basis; explicit range, per-player combat and full-result assertions are
retained. Failed attempts and targeted analysis-only diagnostics remain saved.
Production engine code and game rules were not changed for this continuation.

The job still enforces `memory.max=67108864`, `memory.swap.max=0` and one job at a
time. `memory.high` is now unlimited to avoid soft reclaim delays, while the
hard RAM cap remains unchanged. Kernel-recorded peaks for the final simulations
are 64.000–64.426 MiB, including brief kernel accounting overshoot. Five live
containment checks pass, including child-group OOM and deadline cleanup. A
separate fifteen-peer Coagulation stress attempt hits the hard cap and is killed
inside its own group before gameplay; it remains a failed test. No budget was
raised. This is why the completed sixteen-player network cases use the native
four-local-player-per-peer configuration.

These simulations establish the exercised native tag-backed gameplay and
loopback transport paths. They do not test graphics, audio, physical controllers,
independent client physics, WAN or retail Xbox System Link, and they do not
complete the original full campaign/multiplayer goal. The existing rendered
two-player and four-player results above remain separate evidence. Campaign
is still deferred until multiplayer is finished.

## Stable cache ownership and the full retail-map simulation matrix

The next continuation completes 15 strict sixteen-player simulations: FFA
Slayer on all twelve retail multiplayer maps, and native loopback UDP Team
Slayer on Beaver Creek, Coagulation and Headlong. Every participant moves and
consumes ammunition; actual combat, death, salted respawn, final records and
the ordinary results countdown are checked. Each network match uses four
authority seats plus three real `LanClient` peers with four seats each. Peer
identity, combat, position, team, clock and results state match the authority
on every tick. This grouping is now the guarded runner's sixteen-player UDP
default; the explicit fifteen-peer stress path remains available and its
previous memory failure remains retained.

All jobs retain `memory.max=67108864`, `memory.swap.max=0` and one match job at
a time. The selected runs record kernel peaks from 39.45 to 64.81 MiB despite
the configured 64 MiB limit; no budget was increased. Source/binary/input pins,
per-map measurements and exact guarded commands are in
`analysis/tags/gameplay-validation-20261013/cache-snapshot-candidate/report.json`,
`RESULTS.md`, `pins.json` and `REPRODUCE.md`.

Larger map imports required a stable file-backed cache owner. `Cache` now uses
one private byte accessor over an independent anonymous disk snapshot on Linux,
with filesystem cloning or bounded streaming copy inside `CacheFileSnapshot`.
Original title-update writes remain private. Source overwrites, truncation and
unlinking cannot change the owned image. Vista canonicalization and unavailable
or unsupported backing retain the existing resizable vector. Group layouts,
public tag APIs and decoded values are unchanged. Headlong's RSS immediately
after cache construction drops from about 82.7 to 30.4 MiB in the retained
stage probe, allowing its real content import and matches to fit the test cap.

All 444 complete Cache-header consumers in the 38 linked project archives were
rebuilt with the original GCC commands and flags, avoiding a mixed Cache ABI.
The stable file-owner tests cover exact bytes, private writes, retained lifetime,
source truncation/unlinking and both clone/copy backends. Four complete match
traces remain byte-identical to the old byte owner. Eleven retail maps have
byte-identical full tag-directory/group/payload digests; the original-buffer
Zanzibar baseline exceeds 64 MiB before producing a digest, so full baseline
equality is not claimed for that map. Its new full match passes separately.

The current working-tree merge preserves unrelated edits and centralizes all
Cache byte reads behind the private accessor. Sixty-five affected compilation
units pass GCC syntax checks; the final controller change is checked separately.
Fourteen targeted reference checks pass. The unchanged Vista campaign asset
sweep exceeds 64 MiB, and the optional source-machine-scene renderer cohort
exceeds its 1 GiB compiler budget. Both limits remain explicit failed/deferred
coverage, and no partial old/new renderer ABI was tested.

The controller's oversized navigation grid now uses its existing bounded
fallback instead of throwing. It also chooses the nearest visible enemy and
checks candidates in distance/slot order to avoid redundant visibility queries.
These are test-driver changes: engine state, collision, rules and clock are
untouched. The initial Waterworks 300-second deadline failure remains retained;
the optimized strict run completes in about 288 seconds. Its entire old trace
prefix and Headlong's complete trace are byte-identical after the query
optimization. No gameplay assertion was weakened.

These simulations do not instantiate graphics, audio, independent client
physics or real WAN/Xbox System Link. Existing unsupported variants and the
full campaign/multiplayer parity goal remain incomplete. Campaign stays
deferred until multiplayer is finished. Current launchers and other concurrent
engine changes are not replaced by this frozen validation binary.

## Map-pack coverage and packet-fault simulations — 2026-10-13

The next stage adds **21 successful complete simulation executions**, including
reruns with stronger network checks. Across this stage and the previous retail
milestone, all 21 supplied maps have at least one strict sixteen-player Slayer
path. Twenty maps pass FFA; Turf passes TeamUDP while its FFA path remains
failed. Gemini passes FFA while its TeamUDP path remains failed. This does not
establish all-map/all-mode coverage.

Three full matches pass with deterministic downstream datagram loss,
delay/reordering and duplication: sixteen-player Team Slayer on Beaver Creek
and four-player FFA/Team Slayer on Coagulation, all over native UDP.
They verify 31,572 accepted snapshots and recover identical final results.
Maximum peer lag is four scheduler ticks. Client controls and the ordinary
30 Hz game clock remain unchanged; four-player Coagulation FFA traces and final
records are byte-identical with and without faults. The optional runner flag
is `--network-profile loss-reorder` for UDP modes; `clean` remains the default.
The test-only proxy bounds its queues and uses real nonblocking loopback
sockets, without changing host network settings.

The old snapshot comparator missed changed health. The expanded comparator
rejects 32 independent corrupted field groups on the immutable protocol 51
cohort. A separate captured current-source unit rejects 38 groups, preserving
concurrent vehicle-lighting fields. It links actual production FP equality
and grenade initialization with captured current headers, avoiding a mixed
engine ABI. Event/device/body-pose banks retain separate checks, and absent
headless FP clocks do not establish positive animation behavior.

All nine map-pack maps have byte-identical complete old-owner/new-owner tag
payload/directory/group digests, bringing the exact comparison to 20 maps.
Zanzibar's original-buffer baseline still exceeds 64 MiB; its separate passing
match does not establish full baseline byte equality. The explicit original
input for this stage is TU1.1; no OG Xbox TU1.5 visual parity is claimed.

Both remaining full-match failures now have concrete native causes:

- Sixteen-player Turf FFA reaches the existing missing model-region owner when its actual
  panel truck selects region 0/state 4. Independent bounded original-map reads
  match the native response fields exactly, and a real 87-hit damage repro
  reaches the same rejection. Complete region/permutation, effect, collision,
  render and replication owners remain required.
- Sixteen-player Gemini Team Slayer over UDP completes the clock/results with zero shots and correctly
  fails combat coverage. A 600-tick prefix identifies the closed authored
  High Charity doors at placements 27/28. Their automatic proximity/autoclose
  producer is absent from identical pre-migration/current device-step bodies.
  Independent original-map reads retain activation radius 2.6 and open delay
  1.5 seconds. Rejected navigation prototypes remain analysis-only.

Every match/probe retains the verified 64 MiB aggregate cgroup cap, zero swap
and serial execution. Successful match kernel peaks range from 41.20 to 65.17 MiB;
actual kernel accounting can slightly exceed the configured limit, which was
never increased. Compiler jobs are separately bounded and serial. No engine
behavior, door state, collision or rejection guard was changed by this stage.
The current launchers and other threads' engine edits remain preserved.

Reports, exact commands, source/binary/input pins, red/green checks and remaining
limits are in `analysis/tags/gameplay-validation-20261013/map-pack-simulation-continuation/`
(`RESULTS.md`, `report.json`, `REPRODUCE.md`, `pins.json`, `cleanup-audit.json`).
The full goal remains incomplete: these simulations do not cover independent
client physics, graphics/audio, real WAN/Xbox System Link, the 37 unsupported
variants or campaign. Campaign stays deferred until multiplayer is finished.

## Vista/Xbox door and damaged-region owners — 2026-10-13

The subsequent continuation closes both failures above. Vista's actual machine
type table identifies58D8C7 and its automatic-door branch58DB44..58DF44.
Unchanged Xbox115566..11599B instructions establish the eligibility, side,
phase, group and autoclose decisions;1,278 production cases match exactly.
The26 measured Vista differences are x87 timer-precision boundaries. Both
versions close at45 ticks for the actual1.5-second Gemini delay.

Xbox16D660/16DAD0/16DCE0 establishes live region choice, property fallback,
weighted RNG and effect ownership.745 production cases match unchanged
instructions. The weighted-choice boundary extension caught31 early-rounding
errors before the x87-compatible product was corrected. Effect allocation and
retirement remain explicit services; no fake production handles are supplied.

The actual Turf truck changes permutation0→1/state4 after87 focused impacts,
retains all three authored non-removal responses, consumes no transition RNG,
and has no authored looping effect. Its current choices drive collision,
marker posing, CPU geometry and completed native snapshots. Late UDP join,
idempotence and forged-definition/permutation checks pass. All six resident LOD
decodes match their active collectors. CPU submission changes2,946 intact
vertices to2,352 damaged vertices while retaining four material slots; this
does not establish pixel parity.

All21 supplied maps now pass full16-player FFA simulations with both fixes.
Additional local/team/native-UDP/faulted executions and byte-identical authority
trace comparisons are listed in the continuation report. The captured
protocol53 snapshot comparator rejects49 corruptions; a separate captured
current-source unit rejects55. Seventeen changed production units compile
against captured current headers. Concurrent root changes and the compatible
full-match cohorts remain separate, and launchers are preserved.

Native jobs remain serial under verified64MiB aggregate cgroups and zero swap.
Admission now preserves4GiB of host RAM. A shutdown-recording race involving
systemd's retained active/exited state has a deterministic red/green test;
success still requires previously verified kernel controls. A fresh Zanzibar
rerun matches the retained gameplay trace/final bytes. Eight containment checks
pass, including fail-closed, host-reserve, child-budget and cleanup cases.

Exact commands, source/binary/input pins, failures, successful reruns and scope
are in `analysis/tags/gameplay-validation-20261013/vista-mcc-owner-continuation/`
(`RESULTS.md`, `report.json`, `REPRODUCE.md`). Independent client physics,
GPU/audio, real WAN/Xbox System Link, broader variant/weapon/objective coverage
and campaign remain unverified. The earlier2-of39 admission result is a
historical audit; concurrent starting-weapon work is not covered by this cohort.

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

## Fresh engine snapshot and campaign device diagnosis

The later `current-engine/` snapshot builds the complete production dependencies
and all four live verification/prediction targets successfully. Its 14,712
regular source files were rehashed after testing with no changes. This records
that snapshot; the concurrently edited checkout can have later build errors.
`current-engine/report.json` records its binaries, commands and remaining work.

The snapshot completes a new ordinary-time Coagulation Slayer match, 3–2:
1,943 ticks and 7,588 rendered frames. A separate two-process Team Slayer match
also passes, 3–1: host and client reach identical final identity/score records
and complete the results countdown. These results are in
`current-engine/coagulation-ffa.json` and
`current-engine/two-process-match/report.json`.

Normal Cairo startup still fails before combat. The first actual device call
sets machine handle `e37d010e` to position zero immediately; the campaign
import excludes machine owners, producing `map::at`. The unchanged startup
failure and read-only debugger capture are retained. All 133 admitted initial
Cairo machines can be prepared through the shared tag API, but none receives
a live device owner in this snapshot.

A wider read-only comparison covers 984 initial machines across all 15 owned
campaign caches. All 15,744 compared initialization fields match unchanged
retail constructor/controller execution. The oracle supplies an explicit
30 Hz delta for the consumed primary-constructor clock and retains fixture
RNG state; it does not establish live launch chronology. Independent recreation
cases deliberately change named groups, then restore their saved bytes before
the next cold initialization case. The initial mixed-fixture mismatch report
and the earlier missing-clock failure are retained separately. The Great
Journey scarab selects a 120-frame primary animation, so these results do not
justify freezing every campaign machine at its initial pose.

An isolated diagnostic candidate enables machine definition admission and
gets past the device call, then fails on unimplemented `objects_attach` before
combat. Its first actual attachment request is captured read-only. The
candidate is not a production fix: campaign rendering, primary animation,
proximity and physics integration remain unfinished.

The fresh focused reference suite passes seven of nine tests, with no skips.
`source_device_world` and `retail_source_machine_scene` fail with
`Resident source tag metadata is unavailable`. Their cause remains unresolved;
neither a migration regression nor an external-asset cause is established.
`current-engine/reference-tests.xml` and `.log` retain both failures. The
earlier twenty-test pass below belongs to its earlier pinned build.

During this work the active asset directory was moved into a cache-reset
backup by separate work. The current snapshot uses the preserved owned files
directly after verifying their hashes, without restoring the active asset
directory. `current-engine/asset-source.json` records the location and hashes;
the individual run reports contain the exact usable commands. Initial missing
asset skips are retained and are not counted as passes.

At that earlier checkpoint, testing paused at the user's request to hand back
the build. The user subsequently resumed multiplayer work and requested the
memory-bounded simulations described above. Actual campaign combat and complete
original behavior parity remain unverified.

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

Run from the repository root with original imported files and an active display.
Use the guarded runners below for new matches. They enforce the aggregate64 MiB
budget and preserve failed startup as failure; the current rendered runtime does
not complete startup at that budget. Use a fresh output directory each time.
Historical unguarded commands are retained in the dated analysis snapshots,
not the current procedure. Campaign simulation has resumed; see the continuation below.

```sh
python3 tools/run_tag_local_match.py \
  --native analysis/tags/gameplay-validation-20261013/four-player-validation/tag_four_player_match_tests \
  --map assets/disc/maps/coagulation.map --shared assets/disc/maps \
  --xbe assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03 \
  --players 4 --mode ffa --output analysis/tags/recheck-four-local
python3 tools/run_tag_network_match.py \
  --native analysis/tags/gameplay-validation-20261013/collision-fix/build/tag_network_match_tests \
  --output analysis/tags/recheck-two-process
python3 tests/tag_match_memory_budget_tests.py -v
```

The historical campaign commands return failure. The CMake target
`tag_campaign_gameplay_tests` accepts either a squad name or `--normal`.
The fixed normal observer was linked separately as
`tag_campaign_normal_gameplay_tests` to preserve the original tested binary.
Live targets are explicit manual targets in `cmake/Tags.cmake`, so display/asset
requirements and existing campaign gaps do not silently turn ordinary CTest
runs green.

## Campaign simulation continuation — 2026-10-13

Campaign machine owners and model residency now use the shared tag pipeline.
The facing-cone builtin follows the original strict predicate without an added
occlusion ray. Equal-upper direct-damage definitions now use the source
interpolation for units and props; using the lower field alone caused campaign
SMG hits to apply zero damage. Native collision/residency copies were reduced
under the unchanged64MiB cap.

Three final full local/UDP regressions pass with byte-identical tick traces and
results;24459 faulted-UDP snapshot deliveries were verified. The final campaign
survey passes2/15 startup paths. Armory progresses through light calibration
and movement, but mission completion and the full authored combat squad still
fail. Four final wrapper startup jobs hit the contained RAM limit; other paths
retain explicit script-owner failures. The goal remains incomplete.

The [continuation report](../analysis/tags/gameplay-validation-20261013/campaign-simulation-continuation/RESULTS.md)
records exact scope, source authority and remaining failures. The new manual
targets are `tag_campaign_simulation_tests`, `tag_campaign_residency_tests` and
`tag_campaign_encounter_tests`. Use `tools/run_tag_campaign_simulation.py` for
normal startup and the shared memory guard for focused/encounter checks.
`--require-completion` rejects step-budget exhaustion. The isolated tested
protocol53 cohort and captured current-root compilation are separate; current
launchers and concurrent changes are preserved.


### Campaign owner continuation (2026-10-13)

See [preserved results](../analysis/tags/gameplay-validation-20261013/campaign-owner-continuation/RESULTS.md). Campaign completion remains unverified. Original name-search creation and the outro flag producer are implemented, immutable prop/scenery definitions share storage, initial BSP geometry can transfer ownership, and BSP prediction no longer retains an unused display decode. Legacy campaign log bytes produce structured diagnostic reports.

The final guarded startup survey passes 2/15 windows, with three contained memory failures and ten script-owner failures. Armory's explicit player route reaches an authored equipment-case obstruction with no native navigation route. The full five-actor Old Mombasa encounter still fails: authored jackal shields reject SMG damage while the native capsule adapter lacks exposed hit-region selection. No materials or completion assertions were weakened.

Three full multiplayer regression simulations (Turf, Gemini with UDP faults, and Coagulation with UDP faults) retain byte-identical tick/final traces and verify 24,459 UDP deliveries. Native tests remain serial under an aggregate 64 MiB, zero-swap cap. Nine current-root translation units compile against captured headers; current-root full linking/runtime delivery and Xbox TU1.5 visual parity remain unverified. Later unrelated header drift is recorded separately. The goal is not complete.
