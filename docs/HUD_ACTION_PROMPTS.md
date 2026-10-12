# Weapon pickup and control action prompts

The native multiplayer HUD now receives an action context while the player is
idle. Previously, the native weapon scan only ran after an interaction button
was held, and the HUD had no consumer for the selected action. This prevented
nearby weapons and Zanzibar's computer from showing their button instructions.

## Original evidence

Authority is the local disc `assets/disc/default.xbe`, SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`,
its decompiled functions, and actual Xbox cache metadata. Existing source
control scalar comparisons also cover the separately pinned executable in
[MAP_WORLD_FEATURES.md](MAP_WORLD_FEATURES.md).

- [151110](../analysis/retail/functions/00151110.c),
  [151780](../analysis/retail/functions/00151780.c),
  [1516C0](../analysis/retail/functions/001516c0.c) and
  [151320](../analysis/retail/functions/00151320.c) identify the weapon/control
  branches, their bounds/aim/usability gates and signed action priority.
- [19188C](../analysis/retail/functions/0019188c.c) selects action messages.
  `weap+CC/D0/D4/D8/DC` supplies pickup, swap, pickup-or-dual, swap-or-dual and
  dual-only string IDs. These strings live in the map locale table outside the
  HUD globals' own `unic` range. `ctrl+124` supplies the control action string.
- Zanzibar's Needler (`E314019F`) uses `nd_pickup_or_dual` (`11000B4E`):
  `Hold <primary> to pick up <Needler icon>` followed by
  `Hold <secondary> to dual-wield`. Its dual-only string is `07000B50`.
  The weapon icon is the authored font glyph `U+E11B`.
- Zanzibar's `house_gate_control` (`EC110A9C`) uses
  `zanzibar_gate_action` (`14001E2F`): `Hold <primary> to open the gate`.
  Its static keypad model has no animation graph or named `use` marker.
  The original `B8D30` missing named-marker result writes the node-zero frame
  and returns count zero. `1078F0` therefore skips its side restriction. A
  marker-name-zero query instead returns count one for that fallback.
- Original `24CF66..24D005` selects font 6 for one player and font 5 for split
  views, safe-area anchor 1, and text origin at safe-left / safe-top +60
  (+35 for split views). `hudg+A4` supplies help-text RGB; `131BB0` ignores
  its packed alpha, and this caller supplies text alpha independently.

## Native integration

[MultiplayerMatch::action_prompt](../src/multiplayer_inventory.cpp) queries
current native item and control owners without waiting for input.
Weapon permission bits are shared with the actual pickup consumer. They
account for inventory space, swap availability, compatible dual wield and
same-definition reserve transfer. Unusable items do not displace usable
candidates. Dead, parented, absent and action-blocked units have no prompt.

Controls use their authored model-node bounds and marker frame, the
current device bank, and the existing source bound, aim, side, power, one-use
and priority gates. An accepted primary hold invokes the same retained
control owner and group update that drives gate motion. Used one-shot controls
stop contributing a prompt. Device position/power animation channels now use
the current simulation values for the targeting node/marker bank. The supplied
placed controls have no moving primary idle clock; a new definition requiring
such a clock is rejected explicitly rather than sampled at an invented phase.

The [HUD](../src/multiplayer_hud.cpp) resolves the map's actual localized string
and preserves weapon glyphs. The native action bindings currently are E for
pickup/interact and X for dual wield on keyboard; controller X for
pickup/interact and the active adapter's secondary-pickup button for dual wield.
The current Vista controller adapter binds that secondary action to Y. Each viewport follows its most recently
used input device. Pickup/interact prompts use the selected new SVG button
artwork through the pack's `prompt-source.tsv` binding. This applies to the
dedicated controller/keycap atlas as well as action text. Original logical
metrics, source messages, weapon icons, and current button bindings remain
unchanged. The four keyboard backgrounds `U+E132..E135` select a wider keycap
for longer binding names. The selected redraw pixels retain their native color,
and the existing layout preserves their aspect ratio and HUD button height.
See [the redraw pack](VISTA_PROMPTS_4K.md) for its masters and provenance.
This binding substitution is native platform behavior.
The unit HUD owns the message once; weapon and offhand HUDs disable it.
The ordinary UI font material retains glyph alpha. Text wraps within its own
viewport, with source fonts, placement and help color.

The supplied PC executable SHA-256
`33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109`
names `U+E132` **keyboard_button_background0** at table record `8649EC`
(UTF-16 key `7D1168`). The following records name backgrounds 1/2/3.
The actual Conduit13/12 glyph is 38x38. Both families decode it to RGBA SHA-256
`46ee0051cd452e9c264e09a587324a21d1c364584618b60c26e2a2c3637b7cec`.
The X-button pixels hash to
`fe099fbd092cf4b005ccac3a7d5182d4c29506d5deb574ce826e6d7f9a8cf284`;
LB hashes to `ea6f46f14eb2468229f447bd4df0d78b46cf1c7373c9de988dff7cf7c3b9a0de`.
These are the supplied Vista pack's real pixels; its PC distribution is
Project Cartographer, as documented in [the existing font audit](FRONTEND_FONT_SAMPLING.md).

Native LAN protocol 39 introduced the authoritative action kind, permission
bits, full object identity, definition and control string ID in each player
snapshot. Protocol 43 also admits the original trade/seat/flip action kinds.
Clients render that context and send the existing held input to the
host. Snapshot decoding validates the context before publication.

Original action kinds 2 (ally weapon trade), 4/5/6 (seat entry/boarding), and
8 (vehicle flip) now resolve their authored messages too. Kind 2 carries the
offered weapon definition and reads `weap+F0`; seat kinds carry the unit
definition and seat index and read `unit+1C8[index]+84`; flip reads `vehi+21C`.
This removes the previous weapon/control-only HUD and transport restriction.
It does not create a vehicle or actor gameplay owner: live vehicle admission,
seat transitions, flip execution, and ally trading remain unfinished elsewhere
in the native port. Campaign world-weapon pickups are implemented in the
continuation below. Boarding grenade/melee
hints also await their seated unit/marker/action owners.

This does not claim complete original `BB050` registration chronology, the
full animation/contact-aware `CAFC0` unit-center producer, all original action
execution types, or equality with Xbox framebuffers.
The live adapters use the existing native mover, collision, eye and item
owners. Complete original effect/event consumption remains separate work.

## Verification

[hud_action_prompt_tests](../tests/hud_action_prompt_tests.cpp) loads actual
Zanzibar content and checks the match-to-snapshot-to-HUD route. Its initial
Needler-only version fails against the previous installed runtime library:
`Nearby Needler did not draw an idle pickup button prompt`.
Evidence is retained in `analysis/action-prompts-20261011/red.log`.

The final regression checks exact authored text/icon selection, idle target
identity, keyboard/controller labels, full-ammo action suppression, the
232/233 ms hold boundary, accepted Needler pickup, full-inventory swap and gate activation,
death/range/aim/use suppression, one-to-four local view bounds at 4:3, 16:9
and 16:10, and actual loopback UDP context publication/removal.
Its Vista variant additionally checks byte-identical decoded button textures,
actual keyboard keycap UVs, exact X/LB source glyph UVs in every view, and
white vertex tint so controller colors survive unchanged.

[hud_action_prompt_runtime_tests](../tests/hud_action_prompt_runtime_tests.cpp)
drives a visible native SDL/Vulkan match with actual map collision, source
assets, diagnostic initial placement and queued SDL look/key input. Optional
capture mode retains the idle prompt; ordinary mode holds E, checks the action
and confirms removal of the consumed target's prompt. These observations are
host-native evidence, not physical-controller or Xbox pixel parity.

The initial October 11 visible 1280x800 runs on the host RTX 5080 captured both idle messages,
then separately used actual SDL E holds to take the Needler and open the gate.
Both action runs confirmed that the consumed target's prompt disappeared.
The final selected-Vista captures are
`analysis/action-prompts-20261011/needler-vista.png`,
`needler-controller-vista.png` and `keypad-vista.png`; original renderer PPMs
and runtime logs are retained beside them. The controller capture attaches a
mapped SDL virtual controller and queues a controller event after the initial
keyboard/mouse placement, exercising the last-used-device switch. It shows
the actual X/LB texture glyphs. The keyboard captures show the actual Vista
keycap texture with E/X labels. These captures were visually inspected.
The local Vulkan validation layer was unavailable, so these live runs did not
enable it. The ten initially selected HUD/input/pickup/control/network CTests passed,
including the selected Vista font variant.

## Expanded content coverage

The read-only [content audit](../tools/audit_action_prompt_content.py) inventories
every weapon, control, seat, flip, and boarding message in all supplied disc and
DLC maps. Both JSON catalogs are retained in
`analysis/action-prompts-all-20261012/`, including map SHA-256 hashes and unused
or unresolved fields. The combined inventory contains 36 maps, 63 weapon
definitions, 17 control definitions, and 38 unit definitions. AI/vehicle guns,
unarmed/objective tags and unused messages are retained in the inventory; a
tag's presence does not imply native player pickup support.

[The catalog regression](../tests/hud_action_prompt_catalog_tests.cpp) checks
1,682 weapon action messages, 27 control messages, and 4,265 trade/seat/flip
message routes against actual metadata and locale text with keyboard and
controller glyph substitution. It samples 92 modeled control placements at
three position phases using their authored channels, bounds and markers.
The remaining model-less placed control has no authored action message.
One unplaced High Charity jail kiosk has a nonzero string ID absent from the
locale table; the test records that source condition and requires it to remain
unplaced rather than inventing text.

The same regression runs 756 native weapon scenarios over all 21 multiplayer
maps: pickup, full-inventory swap, and compatible dual pickup for every one of
the 15 usable weapon definitions. Dual scenarios retain a holstered weapon,
preserve the primary, and exercise a shared Y release without switching after
the transfer. All four native multiplayer control placements are also tested
through idle target selection, source text, held-button activation, actual
device-group mutation and one-shot prompt suppression: Zanzibar, both
Containment switches, and Relic's teleporter switch (`dune.map`).

Campaign maps retain a size-zero multiplayer-globals tag. HUD metadata loading
now reads multiplayer-only team/interface blocks only for multiplayer scenarios;
unit/global action strings can therefore be audited in campaign caches too.
The campaign control integration described below now consumes these messages live.

## October 12 verification and delivery

A frozen source tree at `analysis/action-prompts-all-20261012/source-snapshot`
prevents concurrent edits in other chats from mixing incompatible input/header
layouts within one build. `source-snapshot-manifest.json` records its input
hashes. The initial clean Release build passed all 17 selected tests, including
original Vista atlas bytes and glyph UVs under both Vista and MCC Classic HUD
selection. The delivery build is `build-interaction-prompts-verified-20261012`.
The subsequent integration refresh includes the latest local pause/settings
and other source changes at its recorded cutoff. All 19 integration checks
passed, including pause/settings and controller input. Both standard launchers
now select this tested runtime, verified with `--launcher-info`; their previous
binaries are retained in `previous-launchers`. Exact artifact hashes and
results are recorded in `verification.json` and `integrated-ctest.log`.

The visible native Vulkan probes use actual map collision and SDL input.
A virtual controller Y hold completed a Needler dual pickup while preserving
the primary weapon; an E hold activated Relic's teleporter device group and
removed its consumed prompt. Final captures with the selected MCC Classic HUD
show original Vista X/Y artwork for the Needler, the original E keycap for the
shotgun, and `Hold E to open two-way teleporter` on Relic. The sword was also
captured with the Vista HUD. The PNGs, original renderer PPMs, and runtime logs
are retained in `analysis/action-prompts-all-20261012/`. They were visually
inspected. This is host SDL/Vulkan evidence, not physical-controller or original
Windows/Xbox framebuffer proof.

The expanded work covers every native player weapon and every native
multiplayer control. The broader request for every original interactable
object remains incomplete: vehicle/seat/flip, ally-trade, campaign world-weapon
pickup, and boarding-hint gameplay owners still need implementation. Their authored
messages are inventoried and renderable, but message coverage is not live
action coverage. These omissions remain explicit rather than replaced with
proximity-only prompts that could advertise unavailable actions.

```sh
cmake --build build-interaction-prompts-verified-20261012 --target \
  halo2_native hud_action_prompt_tests hud_action_prompt_catalog_tests \
  hud_action_prompt_runtime_tests -j 6
ctest --test-dir build-interaction-prompts-verified-20261012 \
  -R 'native_hud_action_prompt' --output-on-failure
```


## Campaign controls, October 12 continuation

[The shared control query](../src/native_action_prompt.cpp) now supplies both
multiplayer and campaign control prompts from the actual retained device owner.
Campaign content retains its authored device groups and `ctrl` definitions;
`Simulation::step` consumes the actual primary hold after input/training
suppression, using the same source hold threshold, action edges, control gates
and group mutation as multiplayer. A held action is consumed once until release.
Checkpoint restore retains the owner generation, group state and action edges.

[The action HUD adapter](../src/native_action_prompt_hud.cpp) loads the actual
player's unit/global message bank without requiring an equipped weapon. It
renders through the existing action-message layout with raw original Vista
controller/keycap textures, current keyboard remapping and controller layout.
The native campaign loop follows the last-used input device, hides prompts in
pause/cinematics or when player input is disabled, and recreates its HUD material
bank after a BSP/renderer replacement. Campaign training/subtitle vertices are
appended so an empty text layer cannot erase an interaction prompt. Campaign
text sampling now selects the supplied Vista font table's own ordinal rather
than requesting Xbox subtitle font9's `conduit-16` from Vista's font directory;
Vista's table names `conduit-13` at that ordinal.

[The campaign regression](../tests/campaign_action_prompt_tests.cpp) scans all
15 actual campaign maps. It checks 88 modeled control placements with authored
messages: 80 are usable with their initial authored group values and complete
the source hold, actual group change and checkpoint restore; eight unavailable
controls remain suppressed. Each admitted placement checks exact locale text,
keyboard E/controller X substitution, a remapped Space binding, dead/parented/
action-blocked/distant suppression, and cinematic/scripted-camera suppression.
The hold is tested immediately below and at the authored threshold, with no
repeated mutation while the button stays held. These are isolated per-control
fixtures: mission scripts, AI and automatic/manual placement birth chronology
are outside that exhaustive fixture's claims.

[The live campaign probe](../tests/campaign_action_prompt_runtime_tests.cpp)
uses the tutorial's actual automatically placed shield-test switch and unmodified
initial groups, real collision, native simulation, visible Vulkan presentation,
and actual SDL keyboard/controller input. Diagnostic initial player placement
is the only placement override. Both E and a mapped SDL virtual controller X
hold change the actual device group and suppress its used one-shot prompt.
The rendered source message is `Press and HOLD <button> to start energy shield
test`. Original renderer PPMs and visually inspected PNGs are retained in
`analysis/action-prompts-campaign-20261012/`. This establishes the displayed
control action and group mutation; campaign machine motion, mission-script
continuation, control effect/audio consumers and a full mission playthrough
remain separate integration work.

The continuation is built from a frozen 4,585-input source manifest in
`analysis/action-prompts-campaign-20261012/source-snapshot`, with the Release
runtime in `build-interaction-prompts-campaign-20261012`. Exact tests, live
results, launcher selection and artifact hashes are recorded in that directory's
`verification.json`. The broader every-original-interactable request remains
open for the missing gameplay owners listed above; rendering an inventoried
message alone does not establish an interaction.


## Campaign weapon continuation and selected redraw controls

The shared native weapon query/transfer now serves both multiplayer and
campaign. Campaign weapons retain their real salted identities, authored
scenario ammunition (`scnr` weapon +4C/+4E), inventory membership, world
visibility, script birth/destruction, and checkpoint state. A displayed held
pickup transfers that actual item; same-definition ammo transfers and compatible
secondary pickups share the multiplayer consumer. Carried placed weapons survive
BSP changes. Dropped weapons retain their ammunition and become world targets.
The campaign loop preloads pickup world/first-person models and animation clips,
then selects resident geometry when the held definition changes.

The user's latest artwork instruction supersedes the earlier raw-Vista-only
requirement: the selected new `vista-prompts-svg-4k` pack now supplies controller
buttons and keyboard keycaps. The HUD regression compares the selected atlas
pixels and sampled UVs, including both raw and redraw pack configurations.
Earlier original-atlas hashes and captures above are historical evidence.

The continuation regression covers 281 authored weapon placements in all 15
campaign maps, including 17 definitions, 281 held pickups/checkpoint restores/
script birth-rebirth routes/held BSP transfers, 146 compatible dual pickups,
and 207 ammo-only transfers. Isolated fixtures use actual per-placement cache
content, while mission scripts, AI chronology, and collision are separated for
catalog coverage. The visible runtime probe separately retains actual Cairo
collision and an autoplaced Magnum. Actual SDL E and mapped virtual-controller X
holds take its real item generation and remove its prompt. Captures and logs
are retained under `analysis/action-prompts-remaining-20261012`.

Original XBE direct ally-trade and vehicle targeting decisions are now retained
in `source_unit_interaction_target.cpp`. A literal-XBE Unicorn comparison checks
3,072 trade and 3,072 vehicle scenarios, boundary operands, full identities, and
reached service order. External carry, center/facing, flip, occupancy, animation,
and seat-selection services in that differential are fixture boundaries.
It does not prove a live vehicle or ally-trade owner. The native runtime still
lacks the full `CD7B0` carry/actor admission and type21/type28 action callbacks,
vehicle seat construction/transition ownership, and seated boarding grenade/
melee hint producers. Those remaining routes are not presented as finished or
advertised with synthetic successful proximity callbacks.


The final launcher delivery integrates these fixes into the newer normal-world
snapshot in `build-interaction-prompts-integrated-20261012`. It retains 397
other systems archive members byte for byte and imports its other runtime
libraries, frontend and multiplayer-loop objects unchanged. The three systems
replacements preserve campaign loadout ammunition, keep synthetic/native
weapon admission independent of first-person renderer metadata, and correct
the scene inventory/BSP aggregate initializer. The newer solved-transform
scene-center branch is retained. The campaign main loop additionally prepares
arms when there is no initially equipped gun. Source, archive and link command
identities are recorded in `integrated-build-provenance.json`.

The original isolated build passed 24 focused checks. The combined runtime
passed 14 further checks, including the full weapon/control catalogs,
checkpoints, campaign skip, native weapon gameplay, newer script lists,
prop scene, and device world. Keyboard/controller pickup and terminal probes,
main-loop rendering, and BSP checkpoint replay were repeated on the combined
runtime with the selected bundled redraw pack.
