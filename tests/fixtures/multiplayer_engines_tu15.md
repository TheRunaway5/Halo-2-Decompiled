# Original multiplayer engine fixtures

`multiplayer_engines_original_tu15.json` is local original-execution evidence.
`native_tu15_multiplayer_data.original.inc` contains local authenticated original
cold data. The repository intentionally ignores captured original payloads;
keep these generators and documentation with the tests.

Use the pinned English Xbox TU1.5 executable, SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`,
and the offline authored-variant catalogue `assets/disc/maps/mainmenu.map`,
SHA-256 `1b5de78212ee7439b086ef3554885a4188884864ce1040b24ec6604af4a0143b`.
The existing `ctf_initialization_original.json` supplies authenticated MCC
Lockout marker rows; their SHA-256 is checked before the arming sequences.
The oracle reads original instructions unchanged. No game or test loads an XBE
as runtime engine code, and no external service result is fabricated.

The source generators require the project's Python environment with Capstone
and Unicorn. Generate both compositions before running the oracle:

```bash
python3 tools/run_test.py -- .tools/python/bin/python - <<'PY'
import os, subprocess, sys
from pathlib import Path
root = Path(os.environ['HALO2_TEST_SCRATCH'])
for suffix, extra in (('', []), ('/test-services', ['--test-services'])):
    subprocess.run([sys.executable, 'tools/generate_multiplayer_engines_tu15.py',
        '--directory', str(root / ('generation' + suffix)), *extra,
        '--output', str(root / ('services.cpp' if extra else 'engine.cpp'))], check=True)
subprocess.run([sys.executable, 'tools/oracle_multiplayer_engines_tu15.py',
    '--directory', str(root / 'generation'),
    '--fixture', 'tests/fixtures/multiplayer_engines_original_tu15.json',
    '--report', str(root / 'original-abi-report.json')], check=True)
subprocess.run([sys.executable, 'tools/export_native_tu15_multiplayer_data.py',
    '--fixture', 'tests/fixtures/native_tu15_multiplayer_data.original.inc',
    '--runtime-sound-classes', 'src/native_tu15_multiplayer_sound_class_data.inc',
    '--report', str(root / 'cold-data-original.json')], check=True)
PY
```

The first fixture compares selected callbacks, ordered writes, registers and
declared banks. It includes all 39 original option buffers and 1,164 complete
cases, including repeated Assault callbacks through arming thresholds and
arm-progress decay, placed-item fuse/score boundaries with no home/effect/damage
inputs, empty-pool preserve-world round resets, and complete shared frames for
seven empty client selectors and two empty host selectors, plus96 ranking cases
covering both indirectly published comparison callbacks and four original lists.
It also exercises post-object phase requests, five-second intermission and
one-second round-start delays, round-index limits, selected complete reset
sequences, and repeated offline finalization. Host objective reset reaches the
real item factory, which the isolated composition does not replace; the native
world test owns those actual tag/object services.
It includes56 later-player1503D0 host/client timer cases and42 simultaneous-unit
cap cases over all seven selectors, limits3/4/16 and team/global counting.
Another 74 cases execute the authenticated14E73C post-allocation continuation,
including all39 presets, lives0/1/3, departed-player and failed-allocation inputs.
They compare the actual saved-ESI stack ABI, returned datum, complete stores,
initial lives and census. The datum is a declared allocated input; native
world acceptance separately performs actual allocation. Static registration
uses its own ordinary caller frame before the continuation frame is applied.
The test-only composition now includes81 unchanged service bodies, including
profile/team updates and the original32A660 stack probe. It supplies no service results.
Clock/position changes are declared caller inputs between
callbacks, separate from original instruction/store traces. The second fixture
independently reads59 exact original data regions
(2,264 bytes), including loader-zeroed destinations, King's geometry operand,
ranking lists/table, response-curve channel8, thirteen placement capacities,
spawn-zone mode flags, game-over sound-class metadata and retirement policies,
for native ownership checks.
These cases establish bounded source correspondence; complete mode/round/world
acceptance is tracked in
`docs/MULTIPLAYER_ENGINE_GOAL_CHECKLIST.md`.

The separate `native_mcc_multiplayer_engines_tu15` test uses an imported MCC
map supplied through `HALO2_MCC_MULTIPLAYER_ENGINE_MAP`. It executes genuine
native construction, session setup, round activation and the full shared frame
for all 39 variants, with no players. It also moves and returns any registered
CTF/Assault items after that frame. On Lockout only Neutral Bomb has a registered
item without a roster; its tested return occurs after match-over. This does not
qualify occupied objective lifecycle or active-round damage/effects. The normal
CTest wrapper owns all save files, and `--retain-report multiplayer-engines-world.json`
keeps the small result before build cleanup.

The additional `native_mcc_multiplayer_engines_occupied_tu15` CTest uses the same
map and genuine native owners with actual allocated/joined player records:
two for 34 variants, four for five Juggernaut variants. All39 setup/activation/
shared-frame runs pass; all 88 current-unit handles remain NONE. It includes12
active-round physical home returns of original-factory CTF/Assault items to
exact MCC marker positions. This does not qualify spawned players, full
objective lifecycle, normal scheduler integration, later rounds or replication.
Retain its small report with
`--retain-report multiplayer-engines-occupied-world.json`.

`native_mcc_multiplayer_engines_rounds_tu15` adds the original 1628D0 pending-state
input, 181 complete15F450 phase calls through the150/30-delivery delays into the
next round, and original13B4A0/15F450 offline finalization. It checks exact phase,
timer and round-index boundaries and the completed engine bank's once-only
guard. These phase calls retain a fixed world clock; they do not simulate181
complete world ticks or a played match. Its report is
`multiplayer-engines-rounds-world.json`.

`native_mcc_multiplayer_engines_spawn_tu15` uses actual native player allocation,
platform-derived IDs, the constructed simulation machine key, original14FEC0
player scheduling and15EBA0/15F450/1503D0 phases. It records genuine current
units and their actual MCC weapon definitions. Parameter90 controls the allowed
simultaneous units globally or per team; every waiting participant must have
the source exclusion flag and pass the real1601B0 predicate. In2 on1, four
joined players correctly produce three units and one waiter. Caller clock
inputs are explicit; object/physics/light phases and full played matches remain
separate acceptance. Retain its small report with
`--retain-report multiplayer-engines-spawn-world.json`.
The current bounded cohort passes all39 presets on the first delivery in round
zero:87 units and the legitimate2 on1 cap waiter. It checks initial lives from
the actual fresh-player15A660 owner and rejects any round-index advance before
spawning. The native constructor retains its genuinely allocated datum and
resumes14E730 at the authenticated14E73C post-allocation continuation, with the
original saved-ESI caller frame. This closes the former182-delivery startup
defect in Elimination, Phantom Elim and Blast Resort. Ordinary full-world
scheduling, player control, deaths/respawns and complete matches remain separate
acceptance.

`native_mcc_multiplayer_engines_world_tick_tu15` uses the same actual source
players with PhysicsLiveWorldTu15::step_objects, allowing that world to advance
its own clock. It checks13 selected original entry calls and eight real store
anchors in order, then checks10 entries/five anchors when MatchAdapter owns
the engine/player interval. The default World policy runs15EBA0 before
PVS/objects and15F450/1503D0 after effects/lights; the policy is a per-call input,
with no added checkpoint state. Its report is multiplayer-engines-world-tick.json.
The first-person call retains its original empty-local-view guards. This is
selected source-world scheduling, not every13AAC0 owner or a complete match.

`native_mcc_multiplayer_match_adapter_tick_tu15` constructs a real prepared MCC
Slayer match and runs two ordinary ticks through MultiplayerMatch and the
object runtime. It checks the adapter ownership, both clocks and genuine current
units, retaining multiplayer-match-adapter-tick.json. Canonical Slayer now
selects the World owner and checks14 scheduled entries plus real lifecycle and
score-bank readback. Declared placements retain10 adapter-owned entries. King
and other mode/rule consumers remain separate migration work.

`native_mcc_multiplayer_player_factory_tu15` uses a real prepared MCC Slayer
match with canonical starts. Ordinary spawning now calls complete151DA0,
using genuine14FDC0 activation if needed. The match adopts the actual resulting
unit, equipment and completion fields. The test delivers movement, sends a
whole15F8B0 engine death event followed by native fatal-body cleanup, observes
death and waits for genuine factory
respawn. Its report is multiplayer-player-factory.json.

`native_mcc_multiplayer_player_factory_lives_tu15` adds two configured lives,
verifies one remains after replacement and zero/NONE/finished after the next
death. Its report is multiplayer-player-factory-lives.json. The team variant
uses selected teams0/1 and retains multiplayer-team-player-factory.json.
These verify original-engine life/timer/lifecycle consumption in canonical
Slayer; they do not simulate a projectile contact or complete attribution.
The declared-placement regression retains
multiplayer-declared-player-placement.json and verifies exact caller XYZ bits
while excluding those starts from the scenario-owned factory path.

`native_mcc_multiplayer_player_ending_tu15` and the team ending invoke original
15E1E0 at the configured score limit, verifying actual15DDC0 winner publication,
one win award, finalization, countdown and fade completion. The tie ending
delivers original15DDC0 with NONE. Reports are multiplayer-player-ending.json,
multiplayer-team-ending.json and multiplayer-tie-ending.json. These native
closure tests are separate from the original/native ABI fixture cases.

The cold fixture contains59 exact declarations/2,264 bytes, including the
original54-entry sound-class pointer table, complete unique class names and
the indexed item/corpse retirement policies. Its ownership test includes
partial overlaps, aligned uncommitted reservations, repeated mutable
publication, read-only bytes and neighboring independent owners.

`native_mcc_multiplayer_prepared_engines_tu15` passes each authored preset to
the ordinary prepared MCC Match. It compares the complete variant input,
actual selector, initial source frame, joined/current-unit counts, subsequent
frame order/clocks, score and life/team mirrors, actual King bank and camouflage
values. This is ordinary startup/frame integration, not played matches or
objective/event/transport acceptance. The separate FFA/team cap cases set
parameter90 to1 with four joined players and verify original unit admission.
Reports are multiplayer-prepared-engines.json and the two
multiplayer-prepared-*-cap.json files. `HALO2_ENGINE_WORLD_VARIANT` selects one
case for diagnosis; an unfiltered cohort requires all39.

`native_mcc_multiplayer_prepared_finalization_tu15` adds the actual shared
13B4A0 match-over request, followed by211 ordinary source frames per preset.
It checks selected-engine finalization, stable completed scores/wins, matching
native countdown/fade consumers and unchanged frame/clock ownership. This
explicit request does not establish an objective-driven winning outcome. The
bounded report is multiplayer-prepared-finalization.json.

`native_mcc_multiplayer_prepared_views_tu15` attaches the real replicated and
local first-person definitions after the initial source frame, then advances
the ordinary frame cohort. The prepared world's presentation setup boundary
follows its actual startup tick; it cannot assume the simulation clock is zero.
The report is multiplayer-prepared-views.json.

`native_mcc_multiplayer_objective_flow_tu15` covers the15 authored Oddball,
CTF and Assault presets through genuine canonical item birth, normal delivered
pickup input, completed parent/inventory/active-hand binding, ordinary holding
and primary-trigger release. The six Oddball presets must reach their actual
periodic score owner. `native_mcc_multiplayer_objective_goals_tu15` covers the
four CTF and five Assault presets through their original selected goal markers,
actual capture/arming/plant/fuse/score owners and matching native score banks.
Both use `HALO2_MCC_MULTIPLAYER_ENGINE_MAP`; their reports are
multiplayer-objective-flow.json and multiplayer-objective-goals.json.
`HALO2_OBJECTIVE_VARIANT` selects a single diagnostic case; unfiltered runs
require all15 or all9. Explicit initial/goal placement uses wholeB95D0.
Inventory, objective flags, arm/fuse timers, target or scoring results are never
supplied. This does not qualify a played approach, full match, rendered/audio
outputs, disarming/damage acceptance or network transport.

The ordinary first-active owner runs after roster registration, matching the
actual191DF0 ->13A920/15AFB0 order. This matters for the Single Flag defender
selection at2495A7/2495B2. Neutral Bomb uses objective row8, which the retained
HUD reader must admit alongside team rows0..7. `tools/oracle_hud_objectives.py`
retains168 unchanged disc/TU1.5 query cases, including eight neutral-row cases;
the previous160 queries and geometry fixture remain unchanged.

`native_mcc_multiplayer_zone_flow_tu15` exercises all ten King/Territories
presets. Canonical source-born units receive explicit B95D0 positions; the
original2C8640 query selects valid King inputs and the original authored
Territories regions supply their centers. Normal full frames own solo,
contested/defended, exit and retake state. Every source/native score bank is
compared, including King inline state and actual territory capture HUD progress.
The ordinary cohort selects one territory per preset. The separate
`native_mcc_multiplayer_territory_regions_tu15` sweeps all15 configured regions
in independent source worlds, using the unchanged authored variants.
The region sweep also checks each real unparented flag tag and registry entry,
the old marker's removal and full-generation retirement on neutralization,
and the new registered generation on recapture.

`native_mcc_multiplayer_hill_movement_tu15` additionally waits through each
King preset's original movement interval. It checks the exact timer rollover,
new authored hill publication and the following player containment phase.
No timer or geometry output is supplied. Reports are multiplayer-zone-flow.json,
multiplayer-territory-regions.json and multiplayer-hill-movement.json.
`HALO2_ZONE_VARIANT` selects a diagnostic preset; unfiltered ordinary/movement/
region runs require10/5/15 cases. These checks use the prepared MCC map via
`HALO2_MCC_MULTIPLAYER_ENGINE_MAP`, with declared placement rather than a played
approach route. Full edited-rule, match, presentation and replication acceptance
remain separate.

Original2C9F5A/2C9F60 first changes an opposing territory to ownerNONE after
bank+110's neutralization timer;2CA034 then uses bank+112's capture timer.
The tests record and compare both transitions rather than treating retake as
one direct ownership assignment. Current3 Plots inputs take90+150 frames;
the original callback and authored clocks compute those thresholds.

`native_mcc_multiplayer_frame_events_tu15` uses prepared MCC Beaver Creek via
`HALO2_MCC_TELEPORTER_MAP`. Both units come from canonical source player birth;
the test then provides explicit initial positions through wholeB95D0. Two
local views and a recipient without a local view exercise the real event22
constructor, occupied-exit message, actual sound submission, retained-heading
correction and release after ordinary blocker movement. The actual1A6833 stack
store proves that1A6830 runs exactly once between15EBA0 and13B6F0. The report is
multiplayer-engine-frame-events.json. This focused placement stimulus does not
establish a played approach route, rendered/audio hardware or network acceptance.
At actual15EBEE, the test also records the shared frame's still-live caller
stack, then compares it at15EBF9 after the teleporter and output helpers return.
The intervening15EBF3 only reads the player-pool pointer. All54 focused frames
preserve this span, detecting helper-frame overlap independently of native
presentation copies.

All generated binaries, assembly listings and oracle intermediates belong in
`HALO2_TEST_SCRATCH`. The wrapper removes them on completion. The small
reference fixtures are intentionally retained for regression tests.

`native_mcc_multiplayer_role_flow_tu15` exercises all five authored Juggernaut
presets with canonical source-born rosters/units. Ordinary frames own role time,
actual membership/trait banks, respawn admission and departure reassignment.
Shared15F8B0 receives declared killer/victim/relationship/reporting inputs before
D8C20/D8D50/CCF40 physical cleanup; upstream damage attribution is not claimed.
Checks include actual retained role-seconds40, transfer38 and role-kill39,
plus native/source statistics throughout. The2 on1 cap case observes the
waiting fourth participant's admission, verifies original1601B0 blocks the
absent victim, then normally departs only the spare to permit genuine respawn.
`HALO2_ROLE_VARIANT` selects one diagnostic preset; an unfiltered run requires
all five. The report is multiplayer-role-flow.json.

`tools/oracle_result_recording_support_tu15.py` retains816 complete unchanged
returns:144 recipient admissions through878C0/877C0 for declared session/mode/
mask/identity inputs,576 calls to19BAA0 and96 signed player/team retained
comparisons through19DA10/19DA60. All eight GP registers, retired stacks and
tracked arithmetic flags are compared; no instruction or callee return is
supplied. Writer cases compare the entire statistic bank, including untouched
neighbors; comparator inputs must remain unchanged. The actual19BAA0 owner is
physics-live-support, while the separate recording-support module owns878C0
and the two indirectly published retained comparators.

The fixture independently records the20 newly declared recording/context/
channel/projection/formatter spans and all7 weapon,2 PvP and32 award descriptors
with their pointed names. `original_result_recording_support_tu15` compares
51,707 native cold bytes and their original writable/readonly access.
`tools/generate_result_recording_support_tu15.py` regenerates the three complete
bodies,76 instruction sites, in owned scratch. The authored data include is
regenerated by `tools/generate_result_award_data_tu15.py`. Production startup
calls whole692D0, including original context initialization and19A430 itself.
The adapter supplies no successful recording flag. Enabled finalization also
runs the original23975C display projection and its true retained sort callbacks,
writing actual names/counts/counters into the original result-row destinations.

`native_mcc_multiplayer_engine_combat_tu15` covers representative Slayer,
Oddball, King,3 on1, Multi Flag CTF, Multi Bomb and3 Plots matches. Canonical
source-born units, original weapons and normal aim/fire deliver actual fatal
hits. Only initial positions use B95D0 at an authored clear spawn pair. No
vitality, damage, collision removal, killer or callback result is supplied.
Raw look input stays accumulated; feedback reads the completed source look
state rather than replacing its orientation. Actual1F2280/15F8B0 stores and
matching live/retained kill/death banks qualify each hit. The3 on1 hit must
transfer its actual role. The report is multiplayer-engine-combat.json; the
focused Slayer case uses multiplayer-engine-combat-slayer.json. The report's
bounded native diagnostic shot/hit counters are retained separately from the
source-owned firing proof. All seven cases take1,113 ordinary frames.

The objective-flow checks now require enabled recording and matching complete
retained banks. All four CTF cases record one actual flag-grab award at field63;
the five Assault goals record one planted-bomb award at field66. The prepared
finalization check also requires enabled/frozen recording, original header
completion and projection row counts matching occupied retained properties.
The focused Slayer and King/time probes share that full211-frame ending path;
the full preset test requires all39.
