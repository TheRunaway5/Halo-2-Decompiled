# HUD, map pickups and dual wield status

The 2026-10-11 [action prompt integration](HUD_ACTION_PROMPTS.md) adds idle
weapon pickup/dual-wield and static control instructions, including the
Needler and Zanzibar keypad, using authored messages and source HUD fonts.

Checked on 2026-10-10. The requested order is HUD, then map pickups, then dual
wield. These are separate live integration gates. Passing the existing source
kernel tests does not make map pickup or two-handed combat playable.

## Vista crosshair artwork

The HUD import includes all 98 pages in Vista's four crosshair banks:
`hud_reticles` (59), `hud_weak_spot_reticles` (26), `rocket_lock` (4), and
`dashboard_new` (9). The main and weak-spot banks were previously excluded
because seven authored Vista page extents differ from exactly twice the Xbox
extent. Setup and the DDS loader now admit those seven exact inspected records;
ordinary page aspect and density checks remain in force for other artwork.
The original page order and sprite metadata match. No artwork is resampled or
regenerated, and Xbox logical quads, registrations, offsets, shader inputs and
full/split/dual-hand selectors remain unchanged. A selected Vista pack missing
a referenced crosshair bank now fails with an asset-setup message.

The source audit checks all 30 supplied Xbox caches and 41 supplied Vista caches
against their shared-bank extents and sprite sequences. The setup regression
compares all 98 exported DDS files with the exact decompressed Vista resources
and rejects an unreviewed extent. The native regression exercises every shared
weapon HUD's crosshair rows in both hands, one through four local views, and
4:3/16:9 layouts, comparing selected textures and unchanged vertices. Evidence
is under `analysis/multiplayer/hud-functionality/vista-crosshairs/`.
These artwork checks do not establish missing weapon, vehicle or lock-on
gameplay producers, or equality with a final Xbox framebuffer.

## Live shield and ammunition correction

The HUD formerly replaced shield-enabled bit `1000` with appearance bit `2000`
for Elite profiles. Original `13A163..13A18F` sets the two independently. Both
actual `ui\hud\masterchief` and `ui\hud\dervish` shield-meter and shield-mask
rows require `1000`. [The HUD consumer](../src/multiplayer_hud.cpp) now retains
shield availability for both representations and selects the species sprite
separately. The profile's species selector is configuration byte `44`; byte
`40` is an appearance color, not the species.

The actual SMG `ammo_meter_right` consumes input `54`, the loaded/capacity
fraction. `bullets_right` consumes input `51`, the reserve count. Firing one
round changes the bitmap meter and leaves reserve text alone. Reloading that
round restores the meter and changes reserve `120 -> 119`. This is authored
Xbox behavior, not a reason to add a second loaded-round number.

The original multiplayer Spartan and Elite HUD tags have shield status, with
no separate body-health meter in these selected rows. Body health still drives
damage/death and the native snapshot. The implementation retains that layout.
See the locally generated `analysis/multiplayer/hud-functionality/authored-hud.json`
and [the metadata inspector](../tools/inspect_multiplayer_hud.py).

[The production gameplay test](../tests/hud_gameplay_tests.cpp) loads actual
Lockout/SMG/HUD data and runs the real match, firing, projectile hit, vitality,
reload, state export and HUD consumers. It verifies 60/120 initial ammo,
59/120 after an actual hit, 60/119 after an admitted reload, independent local
viewports, damage-driven shield shader inputs, subsequent actual body death,
hidden dead-player widgets, and restored HUD state after an actual respawn.
The clear-world/contact fixture is explicit; it does not validate Lockout's
physical player movement or map collision. Lethal body fractions can be
negative in the source and are not clamped to zero by this test.

[HUD asset/layout checks](../tests/hud_tests.cpp) cover real Spartan/Elite
shield visibility, ordinary damage, recharge, overshields, 320 original bitmap
positions and 60 original counter positions. With Vista artwork they retain
1,518 original HUD vertex positions across one to four players and two aspects.
[The Vulkan framebuffer check](../tests/hud_render_tests.cpp) alternates both
species in four views at 640x480 and 1280x720. The Vista run preserved 167,784
transparent padding samples and rendered 100,248 visible-fill samples with
validation enabled. These checks establish those functions and regressions;
they do not establish complete Xbox framebuffer equality.

The active launcher's 214 HUD/font sampling files match the Vista pack used by
these tests byte for byte. A fresh 1280x720 native Lockout capture renders four
local views with that artwork using the active launcher's rebuilt runtime and
selected TU1.5 executable. That startup check is not complete TU1.5 gameplay
parity. The separate dedicated-host/one-view-client/
two-view-client smoke test also passes with three distinct replicated players.
It exposed an unrelated native startup regression: the active-engine empty
roster check ran before any UDP participant had joined. The dedicated runtime
now starts stepping only after its first admitted participant and retains the
original empty-roster ending after everyone leaves. The successful LAN logs
include unused shader-interface notices; they do not report Vulkan validation
errors, and are not a claim of complete renderer validation or Xbox pixels.
The focused eighteen-test run passes. Hashes, logs and explicit unfinished
feature flags are recorded in the local
`analysis/multiplayer/hud-functionality/verification.json`.

## Elite palette, bitmap edges and live radar

The normal shield layer now selects original `22B3CB`'s Elite colors
`45059A/9C46C1`, while Spartan and overshield palettes retain their original
values. The regression checks the actual animated shader constants, where the
blue palette previously remained bound. Original `22AFF1` now owns every bitmap
quad's independent near/far edge rounding, including ordinary flags. Separate
Spartan and Elite instruction fixtures each verify160 placements across both
display modes. Elite full-screen fill and outline both use authored offset
`(0,-100)`; no compensating outline offset is introduced.

[motion_sensor.cpp](../src/motion_sensor.cpp) implements original:

- `1A1D55`: object velocity, vertical weighting, original player sneak-speed
  threshold and frozen-state gate.
- `1A20E6`: exact unit action/flag, multiplayer and force-visibility gates.
- `1A264E/1A1DFE`: ordered sample construction, strict planar range exclusion,
  control-yaw rotation, clamped byte coordinates and sixteen-contact capacity.
- The sample-history storage phase of `1A22B4`, with ten samples per local
  view and the original descending shared ring index.
- `1A224E`: original 2.1-second sweep timing.
- `1A2823 -> 1A2197`: original history order, size/intensity fading, color and
  size-offset table reads, custom-contact pulse, camera-mode suppression and
  viewport-relative center.

[The executable oracle](../tools/oracle_motion_sensor.py) runs unchanged base
disc instructions from XBE SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
It compares 25,920 movement/visibility pairs, 300 complete quantized samples,
260 sweep-clock samples and 50 complete history-render cases, containing 1,651 actual
contact submissions. Clock cases cover every30Hz phase across two complete
cycles plus negative and long-running clocks, at30Hz and60Hz inputs. Every
represented float and byte matches exactly.
Independent reexecution produces identical fixture/report bytes. Original
`pow`, `fmod` and `fsin` instructions execute; their results are not supplied by
the oracle. No executable instructions are patched.

The explicit inputs are object lookup, velocity, action mode, local ownership,
unit center and contact type/size. The render cases capture the graphics ABI
at `254200`, `254490` and `2548F0`; they do not execute the original graphics
driver. The history-reset helper only clears sample counts; world-list and
refresh-counter ownership remain with the unported world producer.

The live ordinary-player path now enumerates current connected/living player
units in ascending source datum order, applies the original range and movement
gates, and classifies self/friendly/enemy using retained membership. Blip size
comes from each species' actual biped definition. Primary and secondary trigger
holds retain unit `14A` bits `01` and `20`; native snapshots deliver those bits
to LAN consumers. The real `E70E0` lookup reads signed unit+signed346+0C: the
grenade action phase, not zoom or an estimated animation mode. Phases1/2 reveal
a stationary player, while idle0 and post-release3 require the normal movement
or trigger gates. A grenade request alone does not manufacture a trigger bit.
History advances once per simulation tick independently for
each local view; missing LAN ticks age contacts without inventing positions.
Released/dead unit identities clear their view's history, and binocular zoom
suppresses the sensor. Vehicle, projectile, AI, custom action/flag producers,
and the original general-world refresh scheduler remain unported.

`tools/oracle_motion_sensor_controls.py` independently executes the original
two-hand input mapping and radar admission in retail, TU1.1 and TU1.5. Each
image agrees on all512 declared hand/input controls and16 trigger/throw-phase
controls. The input comparison stops before the weapon consumer; the radar
comparison executes the actual component-byte lookup and supplies stationary
movement at its separate owner boundary. No original instruction is patched.
The native regression follows actual offhand pickup/firing through Match and
loopback UDP; the SDL runtime regression also acquires a real authored battle
rifle, switches back to its retained SMG, dual-picks another actual map SMG,
fires each hand separately, reloads and drops the offhand on grenade throw.

[The graphics constructor oracle](../tools/oracle_motion_sensor_graphics.py)
executes unchanged `254200/254490/2548F0` instructions and captures their pixel
definitions, vertex words and sampler/blend calls. The native adapter rasterizes
the64x64 contact, sweeper and mask passes on the host using the authored mip
chains, original UV transforms, separate color/size arguments and blend
equations. Original `254200` clears its format11 A8R8G8B8 target to `00000000`
and sets channel mask `00010101` (RGB only). `2548F0` retains that mask for
sweep, mask and final passes. The host compositor now retains cleared alpha
instead of writing sweep alpha into the target. The prior implementation
missed this state, which darkened the radar and exposed its square during a
pulse despite the earlier functional contact/pulse tests passing. The graphics
oracle now records clear arguments and channel masks at every pixel-program
binding; independent reexecution is identical.

Vulkan uploads that page once per tick and applies the original
final opacity program/blend. The separate `13954B` HUD owner supplies original
species sweep colors, viewport center offsets and full/split-screen extents.
Host rasterization and8-bit rounding do not establish original Xbox GPU pixel
equality or execute its graphics driver.

[The live radar test](../tests/hud_sensor_tests.cpp) covers pulse, moving and
stationary primary/offhand trigger and grenade-phase contacts, sneak threshold, strict range, released units,
ten-sample fade, zoom and four independent views with actual Lockout artwork.
The Vulkan framebuffer test also draws actual Elite fill/outline together and
compares radar contacts/sweep against idle at640x480 and1280x720 with1–4 views.
The original functional Vista checkpoint detected9,197 contact-pixel changes
and87,370 sweep changes; those checks alone did not catch incorrect opacity.
The subsequent regression checks every63 source phases of a complete sweep,
zero intermediate alpha and transparent square corners with actual artwork.
The Vulkan test also checks complete sweep cycles for both species at both
aspects: no overlay pixel may darken the backdrop, and all four square corners
must preserve it exactly. These are regression checks, not a comparison
against original Xbox screenshots.

### Original Xbox rendering comparison,2026-10-11

The expanded graphics trace also captures `222D0`, `224F0` and
`3F5EA0` state calls. Unchanged `254A67/254A7A` sets texture0's border color
to `46000000` before the sweep, retaining it for the mask. The native
compositor previously sampled transparent black outside the sweep texture,
erasing contacts outside its expanding ring. It now uses the original
black border with alpha70/255. A focused regression failed before this change
and passes afterward. Target alpha remains zero: sampler alpha blends RGB;
it never becomes HUD opacity.

`tools/inspect_motion_sensor_capture.py` extracts original-game64x64 targets,
vertices, shader constants and sampler/blend state from retained RenderDoc
captures. Original-game xemu0.8.136 Vulkan capture20, SHA-256
`d99a5c97c85916c7b07e30dc1f181d6db81b3a5246686bcbeebebbc5445642ee`,
independently confirms border alpha70/255, linear minification/magnification,
nearest mip selection, zero mip bias and RGB-only writes. Sweep9328 and mask9335
use actual UV endpoints5.24107551574707/-4.24107551574707. The native
intermediate matches all16,384 captured channel bytes exactly for that phase
after converting the target's BGRA byte order to RGBA. This is one measured
emulated phase, not a physical NV2A framebuffer claim.

A fresh original-game xemu OpenGL capture also records moving self-contact
history: `radar-gl_capture.rdc`, SHA-256
`397d459ae63cfc1d918b4311b693b8a2dd245c10b746d0d07aea6212373a7d3e`.
Events5130/5214 draw two orange history samples with effective colors
`(1,.5,0)` and `(.810000062,.405000031,0)`, followed by sweep5300 and mask5386.
The second quad's actual edge29.995573 rounds to29.9375 in the captured
reference shader: screen coordinates are truncated to four fractional bits.
The native compositor previously interpolated across unrounded edges. Its
captured-contact differential failed in16 channels, maximum error2/255.
It now rounds each edge independently before coverage, UV interpolation and
mip selection. The corrected complete target matches all16,384 channel bytes
exactly, including both history dots and the sweep, with no error tolerance.

`tools/prepare_motion_sensor_contact_fixture.py` converts recorded vertex
positions and effective colors into the optional comparison plan. These are
captured rendering inputs; they do not certify native world contact production
against a recorded Xbox session. The CPU oracle independently checks25,920
visibility/movement cases,300 sampled-contact cases,260 pulse clocks and50
ten-sample history submissions containing1,651 draws against original code.

Reproduce that differential with:

```sh
build-frontend/hud_sensor_tests assets/disc/maps \
  analysis/multiplayer/hud-functionality/xbox-capture-comparison/gameplay
python3 tools/prepare_motion_sensor_contact_fixture.py \
  analysis/multiplayer/hud-functionality/xbox-capture-comparison/contacts
build-frontend/hud_sensor_tests assets/disc/maps \
  analysis/multiplayer/hud-functionality/xbox-capture-comparison/contacts
```

The corrected live renderer also passes complete63-phase sweep cycles at
640x480 and1280x720, for both species and1–4 local views:2,016 square corners
remain identical to the backdrop, no pixel darkens it, and moving contacts
and pulsing remain visible. Both root launchers use the rebuilt executable.

Actual Lockout metadata has range `8`, minimum object speed approximately
`.03`, and player sneak-forward speed approximately `.9`. The moving-player
threshold uses that player-information value times `1.05`; it is not the
vehicle-speed field suggested by an older decompile label. The CPU fixtures
also supply other explicit speeds/ranges to exercise the arithmetic.

This is base-disc CPU evidence. It does not certify every TU1.5 radar routine,
Vista gameplay activation, physical Xbox floating-point execution or GPU pixels.

## Map weapon pickup and dual wield

The 2026-10-11 native adapter now connects actual map items, independent hand
state, authored action clips, secondary HUD inputs and native UDP delivery.
See [current implementation, tests and source limits](MP_WEAPON_GAMEPLAY.md).
The following paragraphs retain the earlier source-recovery checkpoint.

### Earlier source checkpoint

The current native scene admits the recovered stationary levitated stock
weapon branch. Ground items, controller/alignment services and other branches
remain explicitly unported. The original map row/weapon timer and visibility
tests pass, but there is no complete live targeting/request/carry route from
the player's controls to those map objects.

The existing source evidence establishes held-button admission, stable full
weapon datums, ascending four-slot selection, owner assignment before world
unregistration, sequence advancement, and hand attachment. The original map
row retains its carried datum and starts its respawn timer. It must not be
deleted or replaced with a fresh default-ammo weapon on pickup. Required live
roots, node/animation state, registry membership and ordinary item motion
remain documented in [the measured transfer contract](MP_PICKUP_TRANSFER_MEASURED_RESEARCH.md).

`Unit` has retained four-slot inventory fields, but `Controls` and the live
single-SMG update do not provide a second-hand firing path. A valid source
mode-5 pickup is not the complete dual-wield implementation. Each carried
weapon needs its independent ammo, trigger, reload and own-animation clock;
both hand selection/attachment states, first-person bindings, HUD inputs and
LAN delivery must follow their original owners. The native HUD still uses
the single-hand condition/input path. No fake second weapon, instant switch,
shared ammo clock or successful admission callback is added here.

## Reproduction

```sh
.tools/python/bin/python tools/oracle_motion_sensor.py
.tools/python/bin/python tools/oracle_motion_sensor_graphics.py
.tools/python/bin/python tools/oracle_motion_sensor.py --output analysis/multiplayer/hud-functionality/reexecution
cmake --build build-frontend --target hud_tests hud_gameplay_tests hud_render_tests motion_sensor_tests hud_sensor_tests -j4
ctest --test-dir build-frontend -R '^(retail_hud_gameplay|original_motion_sensor|native_hud_motion_sensor|retail_multiplayer_hud|original_hud_bitmap_geometry)$' --output-on-failure
VK_LAYER_PATH="$PWD/.tools/validation/usr/share/vulkan/explicit_layer.d" LD_LIBRARY_PATH="$PWD/.tools/validation/usr/lib" ctest --test-dir build-frontend -R '^retail_hud_framebuffer$' --output-on-failure
```

`hud_tests` accepts Vista artwork as argument four; `hud_gameplay_tests` accepts
it as argument two; `hud_render_tests` accepts it as argument three. All paths
follow the executable name. Original game files and generated oracle output
stay local. The motion-sensor test skips with code 77 when its local original
fixture is unavailable.


## Live trigger and split-screen followup, 2026-10-11

The live snapshot now maps primary and secondary weapon trigger holds to the
original unit+14A bits0/5. A grenade request is separate. The HUD radar consumes
the actual retained grenade component phase:1/2 can reveal a stationary player,
while0/3 still require the original trigger or movement gates. The unchanged
retail, TU1.1 and TU1.5 instructions agree on512 two-hand input cases and16
stationary trigger/phase cases per image. The old archive fails the new offhand
regression; the patched archive passes it, including the loopback UDP path.

[The radar viewport oracle](../tools/oracle_motion_sensor_viewports.py) executes
retail video/grid construction, the139614..139685 center corridor and the final
2548F0 graphics constructor. It captures80 source quads across1–4 participants,
both appearances and video flags0/1/8/9 without patching original instructions.
The final quad is84x84 logical pixels for one participant and64x64 for split
screen. The native fixture checks all40 current-layout cases against those
vertices, including the spanning view in three-player layouts, and checks that
each quad remains inside its own cell. Desktop presentation retains the uniform
artwork scale and places corner HUD groups eight logical pixels from every edge
of each player's viewport. This applies to single-player and all split layouts,
including the spanning view in three-player layouts. Grenades and ammo use the
top corners; shields/radar and scores use the bottom corners.

The other40 original cases use the alternate32/24-pixel overscan margins.
They remain recorded as reference; the current HUD has no option selecting
that profile. The source48/36-pixel safe rectangle still owns camera/target
projection. Center-anchored widgets retain their previous adapted midpoint, and
explicit player-aim positions retain their physical-pixel meaning. The corner
layout changes presentation padding without changing those source inputs.

The Vulkan regression uses actual Xbox radar artwork plus Vista HUD artwork
in all1–4-view layouts at640x480 and1280x720. Both species, independent trigger
holds and grenade phases, contact confinement, a complete63-tick sweep,
RGB-only alpha semantics and transparent square corners are exercised. Original
64x64 empty-sweep and moving-contact captures still match with zero channel
error. Matched original split-screen final-framebuffer captures remain needed
before claiming100 percent Xbox pixel equality in split screen.

The real SDL runtime probe now first holds E to acquire an authored Lockout
battle rifle and verifies the transferred world datum, switches back to the
retained SMG, holds X to acquire another map SMG, fires the two hands separately,
reloads both, then throws a grenade and verifies the retained phase and offhand
drop. The HUD gameplay fixture lets the original thirty-tick player+194 spawn
damage countdown expire before its ordinary SMG hit check and resets its supplied
contact velocities after that wait. It retains the original damage expectation;
shield/ammo state, reload, subsequent actual body death and respawn pass.

Followup receipts and logs are under
`analysis/multiplayer/hud-functionality/live-followup/`. The tested delivery
relinks only the two changed gameplay/HUD objects into preserved original
build archives, retaining the rest of the selected runtime and its shaders.
Broader original pickup admission, general dropped-item Havok, non-SMG weapon
specializations and complete first-person action/animation parity remain subject
to the boundaries in the weapon status documents.


## Vista shield alignment and full-scoreboard artwork, 2026-10-11

The supplied Vista `11081` shared cache authors the full-screen shield fill and
outline at different relative offsets from the Xbox cache. At Vista's actual
2x page density, the Spartan fill needs an artwork-local `(0,-1.5)` logical-pixel
correction and the Elite fill `(-0.5,-1)`. The split-screen pair needs no
correction. Setup now extracts those relative offsets into `hud-alignment.tsv`,
with raw owning records and source hashes in `hud-alignment.json`. The renderer
applies the correction to fill UVs only, including overshield copies; original
widget positions, outline bounds, radar geometry and texture pixels survive.
The selected installed Vista pack has also been refreshed from its retained
original cache, without another installer import.

The full scoreboard previously appended the Vista font atlas but continued to
sample the Xbox glyph regions. Its shared painter now uses the existing
`retail_font_sample_uvs` contract after original clipping, for text and shadows.
This covers both held and automatic postgame scoreboards while preserving the
current layouts, colors, ranking and fade behavior. Full-scoreboard cells are
procedural rectangles, not a cache bitmap backdrop; the raster artwork consumed
by that initial fix was only the imported font. The complete GUI followup below
adds the Vista bitmap owners as well.

Focused red/green regressions, source records, Vulkan captures and the shield
before/after comparison are retained in
`analysis/multiplayer/hud-functionality/shield-scoreboard-followup/`. The CPU
artwork checks cover 240 fill and 240 outline vertices across both appearances,
all one-through-four-player viewports and both supported aspects. The font
check covers 98 admitted Vista glyph/shadow quads with original geometry and
colors. The existing original-only layout and painter oracle checks still pass.


The final focused CTest slice passed 7/7 and setup checks passed 8/8. Live Vulkan
probes passed manual four-player widescreen F3 hold/release/resume and automatic
one-player time-limit completion; both production paths submitted admitted
Vista glyph UVs. The automatic probe counted 21,660 Vista artwork vertices.
Both root launchers select `build-armor-emblem-20261011/halo2_native`, whose
three affected object `.text` sections and three HUD/UI shader binaries match
the tested components. The newer concurrent delivery was preserved. Details
and exact hashes are in `shield-scoreboard-followup/verification.json`.

## Complete Vista scoreboard GUI, 2026-10-11

Both held and automatic pre-carnage scoreboards now bind the complete supported
Vista full-scoreboard artwork. Setup exports `scoreboard.tsv`: the actual
`mulg` waypoint emblem/death bitmap owners plus the background and foreground
emblem banks, totaling four banks and 98 unchanged DDS pages. `scoreboard.json`
retains Vista's actual appearance/team palettes and the five verified Vista
cell paint words. Selecting Vista artwork requires all banks and its font;
missing inputs fail explicitly instead of silently falling back to Xbox pages.
The existing roster, cells, text bounds, safe areas and viewport transforms stay
with the current layout. Each local view/player has independent emblem material
state, including page selection, colors, toggle and fade.

The supplied executable's `4749F7` renderer calls `46BE60` for bitmap icons and
`672D24` for solid cells. It does not draw a mainmenu panel bitmap behind this
table. Its `46BF66` connection indicator also uses solid strips. The native
strip painter matches 129 original submissions across 27 source-executed
quality/opacity cases; missing native quality measurements remain absent.
The source owner audit, live captures and delivery receipt are in
`analysis/multiplayer/hud-functionality/vista-scoreboard-gui/`.

The complete GUI test checks all 6,208 per-view bitmap bindings against the
actual Vista DDS files and preserves every existing player-cell rectangle in
40 held/pre-carnage cases covering all one-through-four-player views and both
aspects. These are CPU submission and source instruction checks; live SDL and
Vulkan checks separately exercise the production display paths. They do not
claim a matched original Vista framebuffer or complete Xbox raster equality.


## Source multiplayer event messages, 2026-10-28

The generic event journal now reaches local HUD message queues across all seven
original mode families. Actual MCC localized responses supply text, player/
team/territory names and medal glyphs. Original audience, split-screen,
postgame and explicit-target rules apply independently of audio and network
role. Dead players retain message text under the ordinary scoreboard gate;
unit artwork remains hidden. Per-view queues and delivery cursors survive
receipt continuation, merge existing literal notices in source order, and
reject duplicate or already expired packets.

Eight focused checks pass, including a normal SMG fatal hit with killer/victim
messages, all seven mode scenarios, real UDP late-join/delivery, 4,960 original
admission cases and four-view Vulkan pixel isolation. See
[the implementation and bounded validation](MP_EVENT_MESSAGES_SOURCE_RESEARCH.md#native-implementation)
and [the retained acceptance report](../build/test-results/mp-event-messages-20261028/implementation-acceptance.json).
The selected launcher was not changed by this work.
