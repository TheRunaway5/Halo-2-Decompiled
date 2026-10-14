# Multiplayer world effects and interactions

The requested live completeness has **not been achieved**. [Lockout's snow now
renders in live matches](SOURCE_WEATHER_LIVE_SNOW.md), including normal,
split-screen and LAN checks. [Rain](SOURCE_WEATHER_LIVE_RAIN.md) and
[Lockout/Warlock background layers](SOURCE_WEATHER_LIVE_PLATES.md) now have
live consumers and repeatable pixel comparisons too.
[The authored attachment bridge](SOURCE_MAP_EFFECT_ATTACHMENTS.md) now uses
actual scene lifetimes, WORLD nodes and object origins. Its 21-map bank prepares
**93 of 96** attachment graphs, including 85 native visual graphs and eight
Terminal damage graphs owned by the original simulation. The train bridge
binds those actual allocations without replaying damage or creating native
particles. Original before/after call checks kill a declared near target and
preserve a distant target; normal player traversal and full train acceptance
remain open. Containment, Turf and Relic's named attachments
now consume completed original effect allocations, function controls, flags,
colors and local frames. Actual source-world checks pass, including Turf damage
activation/pause/drain, repeated views and real UDP/four-player late joins.
This does not establish original particle history or independent client PVS.
[Shape 9 and explicit constructor seed locations](SOURCE_MAP_PARTICLE_SHAPE9_TU15.md)
now let all Warlock and Deltatap attachments start and emit particles;
Foundation's lift particles have repeatable GPU evidence.
Both normal launchers now select the coherent 188-map-routine plus five-swarm-routine build in
[the current delivery receipt](SOURCE_WORLD_LAUNCHER_DELIVERY.md). All 21 Xbox
maps pass ordinary source startup, and the short-render bank passes with its
per-build hashes retained. Their MCC-only readiness gate still reports native
MCC gameplay unavailable; direct owned-XBE evidence does not close that gate.
[Terminal retains 1,200 rendered ticks with one view and 240 with four views](SOURCE_MAP_TERMINAL_CONTINUATION_TU15.md).
Ascension and Colossus now render their authored lift distortion through the
[current-surface consumer](SOURCE_MAP_DISTORTION_TU15.md). The complete
[original swarm kernel](SOURCE_PARTICLE_SWARM_TU15.md) is registered and matches
bounded original host execution. The new [original sprite setup and source-pool draw bridge](SOURCE_PARTICLE_SPRITE_CONTEXT_TU15.md)
match the authored atlas/aspect contexts and14 original Backwash render inputs.
The source-pool bridge is linked but its normal frame/queue/graphics consumer
remains unfinished; ordinary Backwash glowbugs and Elongation lens rendering
remain open.
Complete train behavior, all authored effects/interactions, the two Vista maps,
network and full Xbox visual acceptance remain open.
[The original decal cache export](SOURCE_DECAL_VERTEX_CACHE_TU15.md) retains
actual clipped geometry. [Its cached decal GPU consumer](SOURCE_CACHED_DECAL_RENDER_TU15.md)
now has visible ordinary Zanzibar evidence: hiding its draws changes 2,680
pixels and enabled repeats are identical, with zero Vulkan validation errors.
Its draw planner also matches the original 20-draw/22-quad fixture. The fresh
runtime passes all 21 first-source-tick checks and 60 rendered Zanzibar ticks
with one/four views. Both installed normal launchers now select that packaged runtime. Remaining maps and interactions still need acceptance.
[Zanzibar's computer now has native idle button
prompts and player-held interaction](HUD_ACTION_PROMPTS.md), using the source
scalar control gates, authored static model bounds and current device groups.
Complete original player-action admission and attached effect/spatial sound
services remain open. [Device-event dispatch and native PCM/LAN playback now
pass local one/four-view, real UDP and moving late-join checks](SOURCE_DEVICE_EVENTS.md).
Shared groups, ordinary device motion, rendered channels and authored posed
collision now have simulation consumers and a native host/client state bank.
The bounded native input adapter opens the gate through that retained state.
[Visible one-player and four-player SDL tests now verify closed obstruction,
computer activation, gradual motion, one-use rejection and open-gate
traversal](SOURCE_CONTROL_LIVE_GATE.md). Complete original scheduling, effects,
player-action producers and Havok parity remain open.

The [normal launcher delivery](SOURCE_WORLD_LAUNCHER_DELIVERY.md) now includes
the script/list kernels and ordinary authored root activation. The production
probe runs all 23 maps for 1,200 ticks without VM faults; Colossus cans,
Elongation crates and Terminal trains now have live script lifetimes. This
does not establish conveyor/crate physics, full prop replication/reset or
complete attached effects. The current direct Vista-only cache check stops
earlier at the missing admitted resource-binding owner. The separate Xbox
texture-donor authority guard also remains; neither boundary is bypassed.

All four authored controls now pass actual native prompt/hold/group/motion/
repeat checks with one and four native players: Zanzibar's computer, both
Containment switches and Relic's switch. Original `B8D30` comparisons establish
that a missing named use marker writes node zero but returns count zero;
correcting the native count removes Relic's erroneous side rejection. These
native command tests use diagnostic collision-free placement, including
airborne starts. Zanzibar separately has SDL walking/traversal evidence.

## Coverage recorded on 2026-10-11

`tools/audit_map_world_features.py` reads all 21 supplied Xbox multiplayer
caches and every authored BSP. It records cache hashes, weather palettes and
exclusions, particle and background-plate references, scenario machines and
controls, shared device-group links, placement flags, creation effects, object
attachments, compiled script calls, trigger-volume counts and netgame flags.
It fails on missing required Xbox caches instead of silently reducing coverage.

The inventory contains 1,486 placements in the surveyed scenery, machine,
control, sound-scenery and crate categories, 50 authored machine placements,
four controls and 864 attachment records. Authored records include inactive,
script-created and unused entries; these counts are not simultaneous live
objects or proof that every attachment can be played.

| Xbox cache | Authored weather | Machines | Controls | Automatic script roots |
| --- | --- | ---: | ---: | --- |
| ascension | None | 4 | 0 | None |
| beavercreek | None | 0 | 0 | None |
| burial_mounds | None | 0 | 0 | None |
| coagulation | None | 0 | 0 | None |
| colossus | None | 4 | 0 | Recycling-can spawners and killer |
| cyclotron | None | 1 | 0 | None |
| foundation | None | 2 | 0 | None |
| headlong | None | 0 | 0 | None |
| lockout | Snow, 1,024 particles and one background plate | 0 | 0 | None |
| midship | None | 0 | 0 | None |
| waterworks | None | 1 | 0 | None |
| zanzibar | None | 6 | 1 | None |
| backwash | Rain, 150 particles | 5 | 0 | `backwash_weather` |
| containment | None | 4 | 2 | None |
| deltatap | None | 0 | 0 | None |
| dune | None | 7 | 1 | None |
| elongation | None | 0 | 0 | Crate spawners and eraser |
| gemini | None | 9 | 0 | None |
| triplicate | None | 7 | 0 | `train_of_death` |
| turf | None | 0 | 0 | `mission_main` |
| warlock | Rain, 70 particles and one background plate | 0 | 0 | None |

Dormant scripts are retained separately and must not all be started. The
inventory also identifies the two additional supplied Vista caches, `needle`
and `street_sweeper`: both have zero authored machines, controls and weather
palettes, and one script. Their scenario/BSP counts are recorded separately;
the Xbox executable does not establish their exclusive gameplay behavior.
Campaign maps are outside this multiplayer inventory.

## Shared control behavior recovered

`source_control.hpp/.cpp` imports actual `ctrl` definitions and implements
`11BA50`'s switch selection plus `1071E0`'s group value/flag changes:

- Toggle switches request zero when the existing group is greater than 0.5;
  otherwise they request one. On, off and call buttons retain their distinct
  requests.
- A successful group write clamps to [0,1], sets changed bit 1 and preserves
  every other group flag. A one-use group rejects later changes.
- Unchanged or blocked writes choose the authored deny effect. Successful
  changes select the authored on/off effect from the original request.
- A NONE position group does not activate or emit an effect. Unsupported
  types and nonfinite inputs reject before group mutation.

Actual authored links, rather than map-name exceptions, identify these users:

| Cache | Shared group | Linked machines |
| --- | --- | --- |
| zanzibar | `gate`, initially 0, one-use | `house_gate` |
| containment | `device_groups_3`, initially 0 | Blue gate and its switch machine |
| containment | `device_groups_2`, initially 0 | Other gate and its switch machine |
| dune | `shrouds`, initially 0, one-use | Both collapsible teleporters, gate, base lights and teleporter switch |

The unchanged retail `default.xbe`, SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`,
produces all expected switch/group fixture outputs. The oracle executes complete
`11BA50` and `1071E0`; its object iterator is explicitly empty and its effect
service captures the requested tag. This does not prove notifications to linked
objects, actual effect playback, targeting, player input, motion or networking.

Fresh validation passes 2,716 original/native cases and all 16,296 compared
output words, including initial/repeated activation of all four authored
control placements. The three new CTests pass: frozen original results, actual
cache definitions, and shared one-use/NONE/rejection controls.

## Control target admission

`source_control_target.hpp/.cpp` now retains the scalar control branch of
`151110`, complete `1516C0`, `1078F0`, `107870` and `151320`, plus `11B980`'s
cold control flags:

- The outer query rejects a parented unit and applies the original bound
  distance filter. Its separate spatial query uses mask `FFFFEBCF`, capacity
  64, and `(unit bound radius + .4) + 3.0`; the native kernel requires a real
  candidate from that query rather than manufacturing spatial membership.
- The segment/sphere gate consumes the actual `CAFC0` result and unit `+168`
  vector. The side gate queries marker `0500008F` with maximum one, respects
  the authored both-sides flag, and rejects a positive forward dot product.
- Usability requires a position group, power exactly `1.0`, device disabled
  bit clear, and an unchanged one-use group. A present position group without
  its actual power group is an unresolved input, never implicitly powered.
- An admitted control contributes action type 3. Signed action priorities and
  the original operand order decide replacement. Equal *rounded* distances
  retain the incumbent. The executable accumulates the two distances in
  different axis orders, so equal geometric roots can still replace each
  other at floating-point boundaries; the differential includes that case.
- `SourceDeviceWorld::consider_control_target` binds the current full handle,
  shared position/power groups and runtime control/device flags. A caller
  cannot provide fresh groups to bypass a consumed one-use switch. Stale
  generations, hidden BSP owners and non-controls are rejected before ranking.

`tools/differential_source_control_target.py` executes unchanged instructions
from the same pinned executable. Mode 0 runs complete `151110` with one
explicitly supplied control candidate; mode 1 enters complete `1516C0` with an
explicit incumbent action; mode 2 executes complete `11B980` on a cold control.
The original segment, side, usability and ranking functions run in both target
modes. Every executed original instruction is checked against the executable.
The report is `analysis/map-world-features/control-target-differential.json`;
frozen inputs and original outputs are in
`tests/fixtures/retail_source_control_target.tsv`.

The final comparison passes **6,812 cases / 20,436 output words**, with 412
distinct unchanged original instructions visited and zero mismatches. These
include all four authored control placements, exact-power and one-use gates,
parent and bound rejection, marker-side boundaries, signed priorities, ray
endpoints/tangencies, and cold flag initialization.

The integration batch builds `halo2_native` and passes all 13 selected CTests:
the two target checks, shared device world, ordinary motion, existing control
checks, multiplayer, checkpoints, machine portal/scene, devices and weather
assets. `analysis/map-world-features/control-target-ctest.log` records those
results. `control-target-verification.json` records that batch's source, fixture,
report, test-log and executable hashes. These are CPU/compile checks; this
batch adds no live input, weather rendering or LAN acceptance claim. The
existing launchers have not been refreshed by this batch.

The explicit services remain `BB050` candidate membership/order, `CAFC0` unit
center, `BADC0` successful typed lookup, `B8D30` posed marker and `B9DD0` world
roots. The fixture does not execute their producers or candidate branches for
vehicles, weapons and other object types. Actual authored group/placement flags
and tag radii cover all four supplied control placements, with explicitly
supplied geometry. Simulation tests apply the same gate before/after each
shared control action and reject stale generations. This is not a live
computer interaction, HUD prompt, network request or original action execution.

The subsequent [nearby-object query implementation](SOURCE_OBJECT_SPATIAL_QUERY.md)
now supplies complete `BB050` enumeration over actual registered spatial
pools, including original portal traversal, forward-list order and persistent
visit epochs. All 818 original/native comparisons pass, including six queries
with retained Zanzibar banks. `source_control_spatial_candidates` uses this
owner with `151110`'s literal mask/radius/capacity. Actual live registration,
unit-center/marker production and player dispatch still have to feed it.

The [unit-center implementation](SOURCE_UNIT_CENTER.md) now ports `CAFC0`'s
scalar branches, `DF380` observer correction, `DE190` head-offset publication
and both basis helpers, with the complete `10F820` state-channel predicate.
All 1,678 comparisons match, including exact declared service arguments/order,
64 retained-offset updates and five calls using the retained Zanzibar
unit/definition/node/channel inputs. Live animation/marker, world-root
and contact services still have to feed this owner; it is not connected to
normal control input yet.

Reproduce the new comparison and checks:

```sh
cmake -S . -B build-map-world-20261011 -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_TESTING=ON
cmake --build build-map-world-20261011 --target source_control_target_tests source_device_world_tests -j 6
.tools/python/bin/python tools/differential_source_control_target.py \
  --binary build-map-world-20261011/source_control_target_tests \
  --output analysis/map-world-features/control-target-differential.json
ctest --test-dir build-map-world-20261011 --output-on-failure \
  -R '^(source_control_target_guards|retail_source_control_target|source_device_world)$'
```

## Remaining live work

1. **Weather ownership and submission.** The running multiplayer loop now
   retains `SourceWeatherWorld` and feeds each view's actual clipped portal
   visibility into weather admission. Its native synchronous resource owner
   publishes and retains the literal cache allocation and records resource-use
   ages. Simulation's existing intensity/command state remains the authority.
   Cold initialization and per-view simulation now run in ordinary matches
   through the persistent session auxiliary stream and existing native elapsed
   policy. See [the native producer integration](SOURCE_WEATHER_NATIVE_OWNER.md).
   [Lockout snow now renders in live matches](SOURCE_WEATHER_LIVE_SNOW.md), with
   a repeatable on/off pixel comparison and normal/split/LAN validation.
   [Warlock and Backwash rain now renders in live matches](SOURCE_WEATHER_LIVE_RAIN.md)
   through a dedicated original-program line pass.
   [Lockout and Warlock background layers now render](SOURCE_WEATHER_LIVE_PLATES.md)
   from current owned state, authored mip chains and actual native depth.
   The submission kernel matches 161 original cases/27,048 words; fourteen
   focused and 25 live checks pass. Complete original clock/random,
   async resource and depth/stencil writer chronology remain open.
   The retained caller composes
   wind, plate and particle updates over current owned banks and connects the
   native camera inputs. Seven unchanged original producer prefixes match
   95,473 output words. An explicit xemu 0.8.136 arithmetic profile calculates
   all 2,018 observed reciprocal-square-root results and preserves those full
   outputs; physical Xbox approximation precision remains unverified. Its isolated
   point and plate GPU tests do not supply those live inputs. Native rain line
   coverage is verified separately; physical Xbox raster parity remains open.
   See [weather rendering](WEATHER_RENDERING_PARITY.md).
   The original time-governor arithmetic, speed ramps, admitted tick loop and
   final double-clock store now have 11,712 original comparison cases. Actual
   platform elapsed admission, network budget and world-update scheduling must
   still feed that owner; the native loop has not been replaced by test inputs.
   The new resource owner matches 1,020 original ready-publication/linked-touch/
   age-rebase calls. Ownership checks cover all 23 supplied maps and 92 actual
   portal-clipped local views. Original async allocation/eviction chronology
   and live submission are not claimed by those ownership-only checks.
   The earlier resource/admission-only integration passed eight actual native
   startup runs for the three weather maps with one
   and four local views, including Lockout UDP clients. Their 20 view admissions
   use published resources but still report `initialized=0`; they do not prove
   live weather rendering. See `weather-world-verification.json` in the audit
   directory for the tested executable and shared-checkout drift boundaries.
   The new caller and calculated-profile evidence is documented in
   [the retained weather caller](SOURCE_WEATHER_WORLD_CALLER.md). Its follow-up
   admission-only build and eight frozen-executable startup checks passed, including both
   UDP joins. All 20 views retain their current 244-byte weather camera bank;
   owners at that stage still reported `initialized=0`. That record is
   `weather-world-caller-verification.json`; that earlier checkpoint did not
   submit weather. The subsequent visible snow delivery is described above.
2. **Admitted control actions and shared state.** The control bound, segment,
   side, power, one-use and priority gates now have original/native comparison
   coverage and actual simulation group binding. The original spatial query
   now retains candidate order over actual input banks; supply its live
   registration chronology, posed control markers and unit-center services.
   `CAFC0`'s scalar/observer kernels now have original comparison coverage;
   actual animation/pose and world-contact owners remain required. Integrate the original
   `186AB0` command target and `14F820` action order (`151C90` tries the selected
   action before weapon pickup), preserving its existing hold/consumption flags.
   `DE190` publishes the marker-derived `+3C8/+3CC/+3D0/+3D4` observer inputs
   consumed by `DF380`; a cold-zero approximation would miss that live producer.
   Connect actual wake/physics notifications, effect playback and control requests to the
   existing simulation owner. Calling its explicitly admitted-action API from
   a fixture cannot constitute an interactive computer.
3. **Machine motion and collision acceptance.** The ordinary common device
   progression (`106790`, `11EED0`) and subsequent gear update now run in the
   simulation. Bound local/host/client rendering and portal inputs consume that state, and
   the existing native triangle backend consumes authored collision polygons
   transformed through the same position/power animation nodes. Original
   automatic-door proximity/autoclose, scripted animation channels, animation
   event dispatch, child physics and crushing/riding behavior remain open.
   Zanzibar's local one-/four-player traversal checkpoint passes; other
   machinery still needs live acceptance. Native triangle queries do not emulate retail Havok.
4. **Scripted and attached world behavior.** Preserve original activation,
   object lifetimes and random ordering for trains, spawners, weather and Turf's
   startup path. Complete missing object-list, spatial-volume, material,
   attachment and physics consumers. See [map script parity](SOURCE_MAP_SCRIPT_PARITY.md).
5. **Authority and acceptance.** The retained group/motion/machine-lifetime bank
   is carried by native LAN snapshots. Verify its rendered network flow and
   complete control requests and required world-event delivery. The authored
   device sound journal now passes actual UDP loss/reordering/duplicate tests,
   ordinary joined playback and a four-view moving-gate late join. Exercise every map's authored features
   in local, split-screen, host and joining-client sessions, including resets
   and late joins. The live Lockout snow checkpoint and Zanzibar's local
   computer/animated-gate/traversal checkpoint pass. Rendered LAN control
   requests and the remaining map features still need acceptance checks.

The three world test helpers now forward to the normal root launcher. Both
installed root launchers select the combined world-feature runtime described
in [normal launcher delivery](SOURCE_WORLD_LAUNCHER_DELIVERY.md). Earlier
GPU-only captures do not establish the live input and traversal results above.

## Simulation device integration

`SourceDeviceWorld` retains scenario shared groups and per-generation private
groups, channels, velocities, initialization flags and signed delay ticks.
`Simulation` synchronizes actual scene lifetimes after HS dispatch, runs common
device progression followed by gear progression, and checkpoints the owner.
Named-group recreation reads the current shared value and preserves its changed
flag; it cannot reset a one-use gate. Gear target writes retain `1152E0`'s direct
write semantics rather than setting `1071E0`'s changed marker.

The new ordinary-motion oracle executes complete unchanged `106790` and `11EED0`,
verifies every executed instruction against the pinned XBE, and compares 22,252
cases / 222,520 output words exactly. Coverage includes 180-tick opening/closing
sequences with independently imported settings for all 54 authored machine and
control placements, delay boundaries, zero rate/acceleration, looping channels,
power transitions, signed delay wrap and initialization state. The animation
bank is explicitly NONE; `107520`, `107980`, `BD020` and `115B10` capture dirty/event/physics
requests at stated service boundaries. These comparisons establish scalar state
and event requests, not animation event execution or Havok responses.

`SourceMachineScene::synchronize(scene, &devices)` observes simulation channels,
group targets and runtime device flags. Bound rendering does not advance its own
gear clock. The unbound overload remains for retained diagnostic image/gear
fixtures. Native LAN clients bind the replicated device owner, including host
handles that differ from their local scene allocator. Control-model animation
and attached effects still need their own complete pose/effect consumers.

## Native LAN device state

Native protocol 35 carries a complete bank of the **currently retained** device
state: scenario group values/flags; device definition, global placement, full
handle and creation tick; position/power channels and velocities; common flags,
signed delay, control flags, private groups, root transform and visibility.
This is the port's transport, not the Xbox System Link wire format or complete
original object-body storage. It uses the existing bounded, fragmented snapshot
publication; no machine bank is silently omitted to fit a datagram.

`SourceDeviceWorld::apply_snapshot` validates the entire bank before replacing
client state. Structural checks reject duplicate handles/pool slots/placements,
future births, nonfinite channels/groups and invalid visibility. Finite source
values are retained exactly; the gear reducer's single upper wrap does not
establish a universal [0,1] invariant for channel/direct group writes. Level
checks match actual device definitions, group count and authored group links.
A retained full handle cannot change its placement/birth tick. Older world
ticks reject; same-tick updates are permitted by the native transport. A fresh
map constructs a fresh replica baseline. Mid-session clock rewinds/new-round
reset orchestration still need their original session owner and acceptance.

The replica is independent of client `SceneObjects` allocation. It rejects
device stepping and group writes; `Simulation` and `MultiplayerMatch` also
reject an authoritative step after a replica is bound. Prepared machine
geometry, portal inputs and posed collision consume its actual host state.
Repeated identical snapshots preserve collision instead of rebuilding it.
Hidden rendering preserves collision, while BSP-hidden/deleted owners remove
it. Rendering never runs a second gear/motion clock.

`source_device_world_tests` exercises exact bank import/export, atomic rejection,
deletion/recreation, same-tick visibility, stale banks and a fresh-map baseline.
All 21 Xbox map owners are exported/reimported with their collision inventory.
`source_machine_scene_tests` binds a Zanzibar replica to an empty client scene,
compares the opened gate and portal values, and exercises authoritative removal
and recreation. `source_device_network_tests` uses the actual Zanzibar cache
and real loopback UDP for moving/settled gate state, one/four-player late joins,
one-use authority, same-tick group updates, fragmented publication, reordered/duplicate/lost/stale and
conflicting fragments, nonfinite device values, and recovery.

The current device-network batch builds `halo2_native` and passes all 14
selected CTests, recorded in `analysis/map-world-features/device-network-ctest.log`.
Fresh original/native comparisons also pass all 6,812 target, 2,716 switch/group
and 22,252 ordinary-motion cases. The `device-network-*-differential.json`
reports retain their exact scope and service boundaries.
`device-network-verification.json` records source, executable, report and log
hashes; the earlier verification manifests remain historical records.

The actual application also joins a dedicated host and renders three hidden
frames with one and four local views on the RTX 5080, at a 1280x720 presentation
size. Both runs use the bundled Vulkan validation layer and report no validation
errors. Their logs, raw P6 captures, losslessly converted PNG previews and
commands are under `analysis/map-world-features/device-network-live/`.
The recorded executable hash identifies the exact build used for these captures.
These are joined-client startup/submission checks: no control action is
injected, and no moving-gate traversal or Xbox framebuffer comparison is
established. Existing animation-action, gait/deadzone and scenery-RNG limits
remain in the logs. The harness terminates only its own dedicated host after
each client exits.

Those tests call the explicitly admitted control API; they do not establish
normal keyboard/controller computer admission. They do not establish a rendered
player-driven computer/gate session, original effect playback, Xbox wire parity, full
HS/Havok object lifetimes, or live acceptance for every map. Complete attached
world-event services and scripted/automatic door state remain required. The
subsequent [device-event checkpoint](SOURCE_DEVICE_EVENTS.md) verifies native
Zanzibar button/gate PCM in local one/four-view sessions, real UDP playback
and a four-view moving late join; it does not close those broader requirements.

Machine collision uses the authored collision/model region selection and the
existing source position/power overlay hierarchy. Hidden rendering does not
remove collision. Destroyed generations stop submitting collision; recreated
owners read the current shared target. The existing host collision backend is
rebuilt when these poses change. Full live acceptance and performance evidence
remain required before launcher delivery.

Focused integration checks cover all 21 Xbox maps / 53 initially admitted
machine/control owners. The four authored controls update their actual linked
groups; machines progress and settle, repeated on/one-use activations preserve
the changed marker, and named-owner recreation retains shared state. Zanzibar's
computer is unnamed, so its lifecycle fixture recreates the authored named
`house_gate` rather than adding an invented scenario name. A collision ray that
hits the closed gate clears after opening while the collision surface count is
preserved. The simulation gear advances without a renderer and checkpoints
restore its exact phase. Renderer checks move the gate from this same owner,
retain current portal inputs, and verify repeated-view stability. These are
focused native tests with explicitly admitted fixture actions; they do not
establish player-driven live interaction, Havok traversal, split-screen input,
rendered network acceptance or the two Vista-only maps' acceptance.

The earlier simulation-integration batch built the RelWithDebInfo application,
and its 11 selected control, motion,
world, renderer/portal, initialization/weather-asset, multiplayer and campaign
checkpoint CTests passed. Three-frame, muted, hidden one-player and four-player
Zanzibar startups also completed on the host RTX 5080 using the bundled Vulkan
validation layer, without validation errors. The four-view run reaches tick 6.
The system layer lookup failed first; the successful runs explicitly use
`.tools/validation/usr/share/vulkan/explicit_layer.d` and
`.tools/validation/usr/lib`. This is startup/submission evidence, not a live
computer interaction or an Xbox image comparison. Existing animation-action,
gait/deadzone and scenery initialization-RNG limitations remain in the log.

## Reproduce

```sh
python3 tools/audit_map_world_features.py
cmake -S . -B build-map-world-20261011 -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_TESTING=ON
cmake --build build-map-world-20261011 --target source_control_tests -j 6
.tools/python/bin/python tools/differential_source_control.py \
  --binary build-map-world-20261011/source_control_tests \
  --output analysis/map-world-features/control-differential.json
ctest --test-dir build-map-world-20261011 \
  -R '^(source_control_guards|retail_source_control|retail_source_control_assets)$' \
  --output-on-failure

cmake --build build-map-world-20261011 --target source_device_motion_tests \
  source_device_world_tests source_device_network_tests source_machine_scene_tests source_machine_portal_tests -j 6
.tools/python/bin/python tools/differential_source_device_motion.py \
  --binary build-map-world-20261011/source_device_motion_tests \
  --output analysis/map-world-features/device-motion-differential.json
ctest --test-dir build-map-world-20261011 \
  -R '^(retail_source_device_motion|source_device_world|source_device_network|retail_source_machine_scene|retail_source_machine_portal)$' \
  --output-on-failure
```

The generated inventory and differential report are under
`analysis/map-world-features/`. Frozen original results and independent control
metadata are in `tests/fixtures/retail_source_control*.tsv`; ordinary device
motion results are in `tests/fixtures/retail_source_device_motion.tsv`.

## Retained event clock continuation

The world effect player now uses the original retained selection/traversal/tick
clock; 5,917 exact original comparisons and four native integration groups
pass. See [SOURCE_WORLD_EFFECT_EVENTS.md](SOURCE_WORLD_EFFECT_EVENTS.md) for
scope, reproduction and boundaries. Fresh normal-launcher snow captures still
pass. The original TU1.5 scenery-lightmap lookup 10F960 reached by Zanzibar
construction is now in the source factory with 512 exact independent original
comparisons. The later isolated runtime stops at the shared startup's unresolved
input/camera constructor 1888A0 before scene construction, so live Zanzibar
acceptance of this factory change is still unproven. Full normal-launcher
acceptance and all-map completion remain open. Earlier accepted snapshots
remain archived; their receipts do not establish the current launcher state.

## Ascension and Colossus distortion continuation, October 14

The production particle renderer now contains the separate authored distortion
vertex program, signed texture filtering and dependent BUMPENVMAP consumer.
Sixteen declared-input graphics cases pass within one byte level, alongside
the existing captured particle graphics check. Untouched TU1.5 calls establish
the rotating framebuffer alias and authored bump writes, including retained
SDK state24. Live admission remains guarded until the original completed stage
state and ordered render-surface consumption are connected. The resource bank
remains91/96; this does not close the full map-effects/interactions goal.
See [SOURCE_MAP_DISTORTION_TU15.md](SOURCE_MAP_DISTORTION_TU15.md) for source
identities, complete-call evidence and the remaining native handoff.
