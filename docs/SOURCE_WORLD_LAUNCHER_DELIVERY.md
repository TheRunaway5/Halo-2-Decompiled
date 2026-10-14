# Normal launcher world-feature delivery

## Current combined normal runtime, October 14

Both current normal launchers select
`.local/mcc-native-20261013/cmake/halo2_native`, SHA-256
`bfda2d9f990ec1c009c10b7995900115d8ea983a242b3efda7e5d9d9ba1b9cff`.
Their current `--launcher-info` outputs confirm that exact runtime. The newer
MCC preview and launcher UI changes are preserved. The MCC-only entry still
reports native gameplay unavailable; direct owned-XBE reference captures do
not establish MCC gameplay acceptance.

The source-created Backwash particle consumer now runs in this normal runtime.
Its camera-only one-view comparison has 14 draws and **8 changed pixels**.
Its enabled repeat is exact, and source/native RNG words agree with the
submission-hidden run. A four-view native movement replay submits seven draws
in view1 but changes zero pixels; visible split-screen glowbugs remain open.
The separate staged v4 sightline's seven unchanged draw packets each render
4-7 pixels against cleared depth and zero against its actual scene depth,
establishing occlusion for that retained case.

Ordinary Lockout snow changes **11,249 pixels with one view** and **23,317 with
four views** in this combined runtime. Both enabled repeats are exact and their
simulation/RNG lines match the hidden-submission runs. Zanzibar also passes
ordinary one/four-view startup. These **14 final-runtime captures** have zero
Vulkan validation errors. They do not renew computer activation/traversal or
independent-client acceptance. The separate staged v4 build completed all21
Xbox map renders and retains the four older lift/snow bank images exactly;
that bank is not labelled as execution of this newer combined runtime.

The previously selected binary crashed in startup `MultiplayerState`
destruction. A one-frame four-player case reproduced it without replay inputs.
A dependency-complete native target rebuild removes that crash; the exact
corrupting write was not independently localized. The failed binary, debugger
trace, build command and passing minimized run are retained. A concurrent
launcher rebuild changed only the launcher pins during the capture bank;
current path/hash checks confirm both launchers still select the tested runtime.
No older launcher binaries were copied over those changes.

[Source frame and rendering implementation](SOURCE_WORLD_SWARM_RENDER_TU15.md)
now advances before game ticks and visibility reset, consumes each view's
original queue, and keeps its source owner independent of vehicle resources.
The protected test passes240 actual match-world frames and240 repeated views.
Complete attachment-graph admission remains **93/96**. Elongation lens, full
Backwash light/lens graphs, independent client particle/PVS histories, source
RNG handoff, remaining interactions, Vista-only maps and Xbox framebuffer
acceptance remain open. The all-map goal stays **active**.

Receipt:
`analysis/map-world-features/swarm-lifecycle-20261014/live-swarm/normal-launcher/rebuilt/installed.json`.
The separate `.local/map-world-live-swarm-v4-20261014` package remains staged
and uninstalled.

## Previous particle-context and source-pool bridge delivery, October 14

Both normal root launchers select `.local/map-world-particle-bridge-20261014/halo2_native`,
SHA-256 `3562494b9839953bb594c9ec121774343c4da0421380c829b603952f02b77bb4`. Both installed `--launcher-info` outputs verify
that path. The existing MCC-only policy still reports native MCC gameplay
unavailable. Direct owned-XBE tests do not establish that separate entry path.

World and muzzle draws now supply the original sprite atlas selection, bitmap
aspect ratio and shared-value prelude. **3,655 original context prefixes across
21 Xbox maps** agree, including 282 non-square ratios and 3,138 atlas selections.
The added 40-instruction view helper passes **1,024 original calls** with full
GP/SIMD/flags, instruction paths, ordered writes and retained banks compared.
The map module now has **188 bodies / 14,808 instructions**; all previous187
bodies remain unchanged. The five swarm bodies /668 instructions remain linked.
[Original particle setup and remaining integration](SOURCE_PARTICLE_SPRITE_CONTEXT_TU15.md)

The source-pool draw bridge is also compiled and linked in this package. It reads
actual completed particle pools and matches **14 callback inputs** from the
whole original1784B0 Backwash render producer. Its stale/foreign rejection and
unchanged-pool checks pass. A declared-camera probe retains14 particles through
240 original frame/admission calls, accumulating478 unconsumed commands below
the original1024 bound. **The normal renderer does not yet invoke this bridge.**
Backwash glowbug rendering and complete source frame/queue scheduling remain
open. Resource preparation stays **93/96**, including85 visual graphs and eight
original simulation damage graphs; Elongation lens and two Backwash graphs
remain outside complete visual admission.

The context candidate passes **22 selected source checks**, all21 ordinary Xbox
map renders, and four additional ordinary split-screen renders. Its four enabled
lift/snow images and simulation/RNG lines match the protected positive
on/off/repeat banks exactly. A separate sword on/repeat/prior comparison is
also exact; it has zero changed pixels and is not evidence of a newly visible
effect. No Vulkan validation errors occurred in the accepted captures.

The final readback-API extension passes real UDP/four-player late join and both
original train-damage ownership cases: **three final extension source checks**.
Its **six final-package captures** preserve those four enabled pixel-bank images
and source simulation/RNG exactly, and pass Backwash ordinary one/four-view
startup. The separate21-map candidate bank is not presented as execution of the
final extended hash. Visible split-screen lift and Xbox framebuffer acceptance
remain open.

The **12,033-file** manifest combines the protected coherent source
with explicit context/view-helper/source-pool overlays. Only owned consumer
hunks are composed over the tested base; concurrent unrelated source edits are
preserved in the checkout. Exact source, compile, object, archive, shader,
original-input and capture pins are retained. The diagnostic observer/export
and original callback recorder are excluded. Shader-path relocation changes
only native `.rodata`; other allocated ELF sections are preserved.

Receipt: `analysis/map-world-features/swarm-lifecycle-20261014/launcher-extension/installed.json`.
Original graphics/frame scheduling, all map interactions, independent client
PVS/particle histories, resets, Vista-only resources, MCC gameplay and full
Xbox visual acceptance remain open. The all-map goal stays **active**.

## Previous live-distortion and swarm-kernel delivery, October 14

Both normal root launchers select .local/map-world-swarm-20261014/halo2_native,
SHA-256 81090c298fa0c3600637c2d5cb92241b7987b468986f9454593c561cc30502d6. Both installed --launcher-info outputs verify that path.
Their existing MCC-only readiness policy still reports native MCC gameplay
unavailable; owned-XBE validation does not establish that separate path.

Ascension and Colossus now render their authored lift distortion through ordered
original sampler writes, retained SDK state, signed bump filtering and copies
of the current render surface. Resource admission rises to **93/96**: **85 visual
graphs and eight original simulation damage graphs**. Elongation lens1 and
Backwash's two controller attachments remain unavailable. This resource count
does not establish complete map effects or interactions.
[Distortion implementation and limits](SOURCE_MAP_DISTORTION_TU15.md)

The final package also registers the complete original swarm2 controller and
four helpers: **five bodies / 668 instructions**. A source-created Backwash call
matches untouched TU1.5 code executed on host x86, including all GP/XMM/FP
outputs except Linux IF and **58,970,198 retained bytes** outside private stack
storage. The frame probe reaches14 particles but does not establish continuous
glowbug lifetimes or connect their ordinary native rendering.
[Swarm ownership and remaining integration](SOURCE_PARTICLE_SWARM_TU15.md)

The protected rendering base passes **25 selected source/graphics checks**,
all **21 ordinary Xbox map renders**, nine four-view renders, and Terminal
240-tick one/four-view continuity: **32 cases**, zero Vulkan validation errors.
Four on/off/repeat banks show Ascension **32,176** changed pixels, Colossus
**91,151**, ordinary one-view Lockout snow **20,039**, and four-view snow
**24,046**. Enabled repeats are exact and source simulation/RNG lines agree.
Lift close-ups are camera-only one-view diagnostics. The finite camera replay
rejects four-player overrides; ordinary four-view lift startup passes, while
visible split-screen lift acceptance remains open.

The final kernel extension retains byte-identical enabled images and identical
simulation/RNG for all four pixel banks, plus new ordinary Backwash one/four-view
startup. Its linked world/kernel objects pass real UDP four-player late join
and both original train-damage ownership cases: **three extension source checks
and six final-package captures**. The separate32-case bank is not mislabelled
as execution of the final extended hash. Near/far vitality remains
C19D5556/3F800000 with presentation disabled/enabled.

The **12,019-file** source manifest combines the protected coherent source with
explicit swarm registration overlays. The extension compiles those owners and
fully links the native runtime against protected base objects. Its diagnostic
observation wrapper is excluded. Source, compiled object, link input, shader,
original-input and capture pins were checked before installation. Equal-length
shader-path relocation changes only native .rodata; other allocated ELF sections
and all original instructions are preserved.

Evidence and installed.json:
analysis/map-world-features/swarm-frame-20261014/launcher-extension/.
The protected rendering base and failed harness attempts remain under
analysis/map-world-features/distortion-live-20261014/. Original frame and graphics
startup/batching, all interactions, independent client PVS/particle history,
resets, Vista-only resources and Xbox framebuffer parity remain open.
The all-map goal stays **active**.

## Previous guarded distortion-renderer delivery, October 14

Both root launchers select `.local/map-world-distortion-20261014/halo2_native`,
SHA-256 `6bf8421cb8c9652ea2956918bfd301729cae4b32bc0efc52da9949920ee0e257`.
Both installed `--launcher-info` outputs verify that path. The existing
MCC-only entry path still reports native MCC gameplay unavailable; direct
owned-XBE checks do not establish MCC gameplay acceptance.

The production particle renderer now contains the separate authenticated
44-instruction distortion vertex program, signed texture filtering and
dependent BUMPENVMAP consumer. Sixteen declared-input GPU cases pass within
one byte level, with unchanged depth and effective matrix/sign controls.
Missing original bump state, inactive dependencies and different programs are
rejected. The existing original captured particle graphics check still passes.
Live Ascension/Colossus admission remains guarded until the completed original
stage-state handoff and ordered current-render-surface consumption are connected.
Untouched original calls separately prove120 external23 bindings, eight SDK
surface rotations and six authored bump-state updates. SDK indices and NV2A
matrix order remain distinct in those evidence records.
[Implementation and source boundaries](SOURCE_MAP_DISTORTION_TU15.md)

The final package passes22 focused tests without skips, all21 Xbox ordinary
map renders, seven four-view map renders, and Terminal240-tick one/four-view
continuity:30 render cases with zero Vulkan validation errors. Three visible
on/off/repeat banks pass with exact enabled repeats and equal simulation/RNG
lines: Containment core3,427 changed pixels, Relic watchtower19,545 and ordinary
Lockout snow20,039. Core/watchtower retain camera-only diagnostic placement;
Lockout uses no camera or root override. These are host checks, not Xbox pixel
parity or complete interaction/network acceptance.

The coherent11,981-file source snapshot, explicit overlays,39 pinned captures,
tests, original execution evidence and installation receipt are preserved in
`analysis/map-world-features/texture-continuation-20261014/`. The package source,
shader, original-input and capture hashes were verified before installation.
Resource admission remains91/96, and the full all-map goal remains active.

## Previous original train-damage ownership delivery, October 14

The preceding accepted delivery selected `.local/map-world-damage-20261014/halo2_native`,
SHA-256 `2646a5a0164a84810260910ef10dfa266df6e44b6094f9a149b15c4e391e9b18`.
At that delivery, both installed `--launcher-info` outputs verified that path. The existing
MCC-only policy still reports native MCC gameplay unavailable; owned-XBE
acceptance does not close that separate gameplay path.

Terminal's eight pure cow-catcher damage graphs now bind their actual source
allocations. The original `17D730 ->D91D0` owner already executes these parts;
the bridge creates no additional damage, native event clock or particle.
Read-only before/after observation confirms fatal damage inside the actual
call for a declared near target and preserved far vitality. Presentation
disabled/enabled produces identical raw vitality bits and source call counts.
Repeated snapshots and four draw views preserve the checked source effect and
location pools, RNG, clock and target body. This is original ownership evidence,
not player-driven traversal or complete train/crushing/network/reset acceptance.

The resource bank now prepares **91/96** graphs: **83 native visual graphs**
and **8 original simulation damage graphs**. Its declared bind-WORLD fixture
starts 66 unnamed visual attachments and retains 2,716 summed particle peaks.
Actual source allocations are checked separately. **19 focused/particle/event
checks pass without skips**, including both original damage-owner cases.

At this final packaged hash, all **21 Xbox maps** pass ordinary 15-tick render
checks. Seven maps pass 60 ticks with four views; Terminal separately passes
240 ticks with one/four views. All **30 render cases** have zero Vulkan
validation errors. Three final on/off/repeat banks retain identical enabled
repeats and simulation/RNG lines: Containment core3,427 pixels, Relic
watchtower19,992 and ordinary Lockout snow20,028. Core/watchtower use declared
camera-only diagnostics in the actual source world/PVS; they do not establish
ordinary-view or Xbox scene parity. Snow uses no camera/root override.

`analysis/map-world-features/nonvisual-continuation-20261014/` retains the
current red checks, authored part inventory, actual source probes, original
caller identities, 11,921-file coherent source/overlay manifests, test results,
render/pixel receipts and `installed.json`. The 186-body / 14,749-address map
module remains included. Shader-directory relocation changes only an
equal-length `.rodata` literal; all other allocated ELF sections remain
unchanged from the built executable. Source, shaders and prior launcher hashes
are checked before installation. No original/native instructions are patched.

The goal stays **active**. Five graphs remain: Ascension/Colossus external
texture bindings, Elongation's original lens rendering/admission evidence and
two Backwash swarm/wind graphs. The original simulation dispatcher returns
for `lens`; a separate draw owner is not yet proven, so no flare is invented.
Both Vista-only raw caches still fail admission before rendering. Complete
machinery, original scheduling/RNG, independent client PVS/particle history,
network/reset and Xbox visual acceptance remain open.

## Historical completed attachment-control delivery, October 14

Both root launchers now select `.local/map-world-functions-20261014-v2/halo2_native`,
SHA-256 `e8dd87d1e8ff1a34554bf43f668c7ed646053e49f061aed2bdbe3977f0ccfb64`.
Both installed `--launcher-info` outputs verify this exact path. Their existing
MCC-only policy remains, with native MCC gameplay still unavailable. Updating
the selected runtime does not accept that separate gameplay path.

[The completed attached-effect bridge](SOURCE_MAP_EFFECT_ATTACHMENTS.md)
consumes actual original allocations, function controls, pause/visibility/retire
flags, colors and retained local frames. The 21-map resource bank now prepares
**83/96** graphs. Its declared bind WORLD inputs start 66 unnamed attachments;
actual source-world tests separately bind Containment6, Turf4 and Relic3 current
named allocations. Turf's original damage function activates, pauses and drains
its trash effect. Native protocol75 transports the completed packets, including
a real UDP pause/resume and four-player late join. Original particle history,
event/RNG ownership and independent client PVS remain open.

All **21 Xbox maps** pass an ordinary 15-tick render check at this packaged hash.
Zanzibar, Lockout, Warlock, Deltatap, Containment, Turf and Relic also pass
60 ticks with four views. These **28** cases have zero Vulkan validation errors.
Six new focused checks and eleven particle/event regressions pass with no
skips. Those regressions retain the 3,072 exact TU1.5 shape9 births and 2,048
exact seed-location matrices. The coherent freeze includes the 186-body map
module and its 14,749 unique authenticated instruction addresses.

Final-draw-only on/off/repeat captures change **3,427** Containment core pixels
and **19,992** Relic watchtower pixels. Enabled repeats and simulation/RNG lines
match exactly. These are declared close diagnostic cameras in the automatic
source world, with actual source PVS and no root override; they do not establish
ordinary-view or Xbox scene parity. The ordinary Containment spawn view has
zero changed pixels and is retained separately. Ordinary Lockout snow changes
**20,028** pixels with an identical repeat and no camera/root override.

`analysis/map-world-features/function-continuation-20261014/` retains source and
overlay manifests, actual source probes, red/failing fixtures, corrected checks,
resource/render/pixel reports and `installed.json`. The first staged runtime
failed on a weapon attachment alias outside its requesting tag payload; the
resident-owner correction already present in shared source was incorporated
into the isolated build before final validation. Both staged candidates remain
preserved. Shader-path relocation changes only an equal-length `.rodata`
literal, with every other allocated ELF section unchanged. No original or
native instructions are patched. Source, shader and existing-launcher hashes
are checked before replacement; prior launchers remain in the package.

The two Vista-only raw-cache attempts stop at `unsupported or truncated Vista
cache`, before rendering. They are retained as admission failures, not passed
map tests. Their conversion/resource owners remain unresolved. The full goal
stays **active**: thirteen attachment graphs still need their original owners,
and complete machinery, original scheduling, network/reset and Xbox visual
acceptance remain outstanding.

## Historical particle shape 9 and attachment seed delivery, October 14

Both root launchers now select `.local/map-world-shape9-20261014/halo2_native`,
SHA-256 `ef5dde661200f2b3b73261db133be1eaa67d5dc6a6a0a5f72925ff94b47ce441`.
Both installed `--launcher-info` outputs confirm that path. The MCC-only policy
remains and native MCC gameplay still reports unavailable; direct owned-XBE
checks do not establish that separate gameplay path.

[The particle and constructor seed continuation](SOURCE_MAP_PARTICLE_SHAPE9_TU15.md)
adds 3,072 exact whole TU1.5 birth comparisons and 2,048 exact seed matrices.
The native 21-map attachment bank prepares 69 of 96 graphs, starts 66 initial
attachments and passes 180-tick lifetimes, repeated views, retirement and salted
recreation with declared bind WORLD/origin inputs. Warlock's 12 and Deltatap's
nine attachments all prepare, start and emit particles. Original eligibility,
scheduling, resource/RNG ownership and network history remain open.

All **21 Xbox maps** pass an ordinary 15-tick rendered check at this exact
packaged hash. Zanzibar, Lockout, Warlock and Deltatap also pass 60 ticks with
four views. Every case has zero Vulkan validation errors. This is one coherent
source build, with base/overlay file hashes checked before installation.
The existing cached decal consumer and 186-body map module remain included.

Shape-9 draw on/off/repeat captures change 3,572 pixels in Warlock and 123,784
in Deltatap, with identical enabled repeats and captured simulation/RNG lines.
These are declared camera-only diagnostic views in the automatic source world.
The close Deltatap camera is in the waterfall volume and does not establish
ordinary view visibility or Xbox scene parity. Ordinary Lockout snow separately
changes **20,028 pixels**, with an identical enabled repeat and no camera/root
override. Neither original gameplay instructions nor native instructions are
patched. Shader-directory relocation changes only an equal-length `.rodata`
literal, with all other allocated ELF sections verified unchanged.

`analysis/map-world-features/attachment-continuation-20261014/` retains the
source/overlay manifests, original helper disassemblies and fixtures, eight
focused test results, native attachment bank, 25 ordinary render receipts,
pixel receipts and `installed.json`. The accepted package records its frozen
shader hashes and prior launchers. The full 23-map effects/interactions goal
remains active, including 27 unprepared attachment graphs, named functions,
nonvisual parts, swarm/wind, complete machinery, Vista resource owners and
matched Xbox/network acceptance.

## Historical cached decal and map delivery, October 14

Both root launchers now select `.local/map-world-20261014-v3/halo2_native`,
SHA-256 `c57be1fc4e0520e03090733f31d55429ffe8d63ab86f8017b113d860c76a3abc`.
Both installed `--launcher-info` outputs confirm the path. Their MCC-only
readiness policy is preserved and still reports native MCC gameplay unavailable;
direct owned-XBE tests do not establish normal MCC gameplay acceptance.

[The original cached decal consumer](SOURCE_CACHED_DECAL_RENDER_TU15.md) is
included in ordinary source-world rendering. Zanzibar now has actual native
GPU cache submissions with one/four views: hiding only those draws changes
2,680/605 pixels at 60 ticks, with identical enabled repeats and zero Vulkan
validation errors. The actual cache, bitmaps and authenticated 13-instruction
vertex program supply the data; no captured geometry or driver is installed.

The map module now contains **186 complete original bodies / 14,749
instructions**, up from 108. The original bounded banks pass 1,745 prior cases
plus 1,038 new continuation cases. The six decal programs/states, original
20-draw/22-quad fixture, real cache export and 180 native vertex-view cases pass.
These checks do not independently cover every new body or full Xbox pixels.

All **21 Xbox maps** pass one ordinary source tick. The 21-map short-render
bank also passes, with hashes retained per incremental callback build; this is
not a claim that every earlier case was rerun at the final hash. The final
packaged hash separately passes Zanzibar 60 ticks with one/four views, Lockout
snow 60 ticks, Gemini 15 ticks and Turf 15 ticks. The continuation preserves
Terminal 1,200 one-view ticks and 240 four-view ticks. Snow changes 20,078 pixels;
Foundation's declared root/camera lift fixture changes 38,829. Enabled repeats
are identical, with zero Vulkan validation errors. The two Vista maps, complete
interactions/effects, original frame/scheduling parity and network acceptance
remain open. The full all-map goal remains active.

`analysis/map-world-features/decal-consumer-20261014/` retains the source and
fixes manifests, original registrations, fixtures, failures before each fix,
focused checks, GPU receipts and `installed.json`. Shader-directory relocation
changes only an equal-length native `.rodata` literal; every other allocated
ELF section is unchanged. Neither original XBE nor native instructions are
patched. `.local/map-world-20261014-v3/package.json` records the exact shader
hashes and accepted scope. Prior root launchers remain in `prior-launchers/`.

## Historical first October 14 combined delivery

At this earlier receipt, both root launchers selected `.local/map-world-20261014-v2/halo2_native`,
SHA-256 `b65799ae9fdb0314e9f4a7dd74b89491fe4870acc47537013a11b2d9df781208`.
Both installed `--launcher-info` outputs confirm that path. Their current
MCC-only import/readiness policy is preserved: native MCC gameplay remains
unavailable, and `--launch-game` rejects that unresolved state. The direct
original-XBE captures below do not establish MCC launcher gameplay acceptance.

The combined runtime uses one frozen current source set, preserving peer
networking and combat changes, plus the authenticated 108-body / 10,172-instruction
map module. Source, overlay, compiler/link inputs and original registration
receipts are under `analysis/map-world-features/coherent-peer-20261014/`.
The final original banks pass **1,745 bounded cases**. Native physical byte
protection, the actual decal cache export and real encrypted direct UDP also
pass. These are scoped tests, not complete game or Xbox visual parity.

`captures/ordinary-verification.json` records **15 ordinary rendering cases**:
all 13 maps that complete the first source tick, plus four-view Lockout and
Foundation. Six additional captures establish Lockout snow (**20,078 changed
pixels**) and Foundation steam (**38,829**), with byte-identical enabled
repeats and zero Vulkan validation errors. Foundation's steam camera/root
capture is diagnostic; its ordinary startup/rendering is checked separately.

The runtime's shader-directory literal is relocated to a frozen packaged set
using an equal-length path, preserving compiled literal extents. Every other
allocated ELF section is unchanged; original XBE and native instructions are
unchanged. `.local/map-world-20261014-v2/package.json` records that relocation,
shader hashes, accepted scope and prior launcher hashes. `installed.json`
records both installed launchers. That earlier receipt is under
`analysis/map-world-features/terminal-continuation-20261014/installed.json`. The former launchers remain in the new package's `prior-launchers/`. The
100-body package and its earlier 98-body candidate remain preserved separately.

The 21-map first-source-tick bank now passes 13 maps, including Terminal, with
no regressions against the coherent 98-body baseline. Eight still fail: seven
at the absent native decal command owner, and Gemini at separate missing
backing. [Terminal's eight additional original routines](SOURCE_MAP_TERMINAL_CONTINUATION_TU15.md)
now pass 1,200 source/rendered ticks with one view and 240 rendered ticks with
four views; complete train behavior remains unproven.
Zanzibar's earlier computer/barrier receipts remain historical and do not
close the current graphics owner. The full all-map goal remains active.

An earlier copied-input relink was rejected before installation: baseline and
candidate both crashed before world startup in `NativeMeleeAnimation`.
`.local/map-peer-20261013/package.json` is explicitly marked unaccepted. The
coherent rebuild eliminated that failure; it did not replace the current MCC
launcher policy with an older one.

## Historical October 13 map attachment delivery

At this historical receipt, both root launchers selected `.local/map/halo2_native`, SHA-256
`14eadcc50f0522fd05c3d7f6644d715c8ca4ecb495ffa5879e5c710138e47efe`.
Both recorded `--launcher-info` outputs confirmed that exact path. This is a **partial**
[map attachment continuation](SOURCE_MAP_EFFECT_ATTACHMENTS.md), preserving the
installed death/spectating libraries with the explicit source overlays. The
original decal constructor/reset and 82 authenticated map-support bodies are
included; the experimental unsubmitted graphics adapter is excluded.

`analysis/map-world-features/launcher-attachments-20261013/verification.json`
records four ordinary Lockout/Foundation one/four-view cases and six effect
captures through the packaged launcher. All ten pass with zero Vulkan
validation errors. Ordinary 60-tick Lockout snow changes **20,077 pixels**;
Foundation's declared-root/camera effect capture changes **38,778 pixels**.
Both enabled repeats are byte-identical. Foundation's diagnostic capture does
not replace its separate ordinary source-world startup/render checks.

The package retains its tested shaders in `.local/map/s`. Its own native ELF
read-only shader-directory literal was relocated to that frozen directory;
allocated-section verification before and after stripping finds only that
explicit `.rodata` change. No original executable or native instructions were
patched. `.local/map/package.json` records the exact relocation and hashes.
The previous launchers remain in `.local/map/prior-launchers/`.

The later 96-body first-source-tick comparison has no regressions against the prior
installed death/spectating runtime and seven improvements. **Ten maps still fail** that bounded
check, including Zanzibar's absent graphics binding. Earlier successful native
gate/traversal/script receipts below do not establish acceptance of this newer
automatic-original-world path. The full all-map goal remains active.

## Historical 2026-10-12 delivery

At the historical receipt, both root launchers selected
`analysis/map-world-features/normal-world-snapshot-20261012/build/halo2_native`.
The three world helpers forward to the ordinary root launcher:

```sh
tools/run_lockout_snow_test.sh
tools/run_lockout_snow_test.sh --local-players 4
tools/run_rain_test.sh warlock
tools/run_rain_test.sh backwash
tools/run_zanzibar_world_test.sh
```

At that historical receipt, the executable SHA-256 was
`2e108f4aa286e292ba372522e7722bbc09ee00996856ad4af248e53bbfe096d5`.
Its 12,734-file frozen input manifest SHA-256 is
`ba87bf2b7e7e3b731c89201fddec4f9b8f0aeca7ec9920743e957833b8051d70`.
The source freeze retains the combined menus/HUD, online features, campaign
inventory and world-feature code. `phase3-freeze.json` records the three-file
change from the preserved phase-two source: the missing control-marker count
and its two focused fixtures. Only one runtime source file changes in this
phase. Independent later changes in the shared checkout were not overwritten.

## Authored controls and the Relic correction

The native adapter previously reported one marker when a named `use` marker
was absent. Original `B8D30` writes node zero for that fallback but returns
**zero** for a named query. `1078F0` applies its directional gate only when
one marker was returned. A marker-name-zero query instead reports one.
Keeping the fallback matrix while manufacturing a count of one incorrectly
rejected Relic's valid exterior approach.

`analysis/map-world-features/control-marker-20261012/verification.json`
executes unchanged `B8D30`, `B8CA0`, `1D8F00`, `1D8F50`, `BADC0` and `1078F0`
with the supplied four control definitions/render-model marker banks. Eight
marker calls and twelve side calls pass, with no hooked routines. The current
native node matrices are explicit inputs; this does not certify their original
construction or animation clocks.

`normal-world-live/marker-fix-controls/verification.json` records all **eight**
ordinary `MultiplayerMatch` input cases: Zanzibar's computer, both Containment
switches and Relic's switch, each with one and four native players. Each checks
the actual idle prompt/message, rejection of an unheld input, an admitted held
`Controls` command, the retained shared group, gradual linked machine motion,
its endpoint and repeated input. Containment observes 302 intermediate machine
states across each linked pair; Relic observes 502 across its four machines.
These counts are observations across machines, not per-machine durations.
Placement is diagnostic and collision-free, including airborne starts for
elevated switches. These tests do not establish a walking route or SDL input
for Containment/Relic, or complete original unit-center/spatial-query producers.

All **25** selected CTests pass in `phase3-focused-tests.log`, including both
control catalogs, the eight native input cases, shared device/network state,
weather, prop/machine scenes, script/list fixtures, LAN startup, campaign
checkpoints and pause preferences.

## Script and prop integration

The ordinary authoritative match starts authored startup/continuous roots
once its host services are installed. Script execution precedes object/device
updates; observing clients do not independently advance those roots. The
original 48-header/128-reference salted list bank and bounded static-volume
collector now feed production multiplayer dispatch. Named NeverPlaced props
are prepared before their scripts create them, and rendering follows their
actual full-handle births/deaths.

`normal-world-live/marker-fix-scripts/` runs all **23** supplied maps for
**1,200 ticks** with ordinary construction and zero VM faults. Colossus creates
cans, Elongation creates crates, and Terminal/Triplicate removes and recreates
trains. These are script lifetimes; they do not prove falling/conveyor motion,
crushing, full prop lifetime replication/reset, constructor RNG chronology or
all attached effects. See [script evidence and service boundaries](SOURCE_MAP_SCRIPT_PARITY.md).

The earlier list-bank comparison retains 213 operations / 115,659 output words
against original routines with real datum pools, with zero mismatches. The
volume collector retains 84 cases / 45,612 words with explicit spatial services.
Its current native adapter uses authored static volumes, actual scene centers
and native placement order, rather than original cluster-registration order.
Attached volumes remain explicitly unsupported.

## Launcher and live acceptance

`normal-world-live/final-weather/verification.json` preserves the complete
**25-case** bank on phase-two executable
`af882d09546c8afe83092ba13fdf7d64680eb86d73ba2cf68cb8180a339b5f62`.
All cases pass with zero Vulkan validation errors, including ordinary one/four
views, real four-view UDP joins, odd 641x481 four-view rendering, Backwash with
no background plate and a deliberate unsubmitted-frame abort. Finite enabled
repeats are byte-identical. Snow changes 2,450 Lockout pixels; rain changes
2,754 Warlock and 531 Backwash pixels. Plates change 512,612 Lockout and 69,959
Warlock pixels. Backwash's ordinary authored weather script is now active.

The same phase-two normal launcher renders all **21 Xbox maps** without Vulkan
validation errors (`normal-world-live/final-maps/`). Needle and Street Sweeper
fail explicitly because their exclusive Vista lightmap textures have no Xbox
donor. All 23 maps passing CPU script checks does not remove that GPU blocker.
The preserved phase-two source is `source-v2/` with `input-manifest-v2.json`;
its binaries are in `phase2-binaries/`. The current runtime changes only the
control marker count from that accepted rendering base.

The corrected executable's normal-launcher snow on/off/repeat and affected-map
one/four-view checks are retained in `normal-world-live/marker-fix-launcher/`.
The SDL device replay is retained in `normal-world-live/marker-fix-devices-v3/`.
Their final receipts, including launcher/runtime/shader pins, are referenced by
`normal-world-live/verification.json`.

All nine corrected-executable launcher cases pass with zero Vulkan validation
errors. Lockout snow again changes 2,450 pixels and its enabled repeat is
byte-identical. Zanzibar, Containment and Relic each pass one/four-view startup.
All four final X11 device/audio cases pass too: one/four-view SDL hold and
walking traversal, ordinary UDP device PCM and moving four-view late-join PCM.
The local tests record `Computer test SDL driver=x11`, retain 147 intermediate
gate ticks and keep the one-use group after repeated input. Their observed
holds are 248 ms and 333 ms, rather than original timing constants.

The phase-two SDL device bank passes actual Zanzibar computer hold/one-use
rejection, closed/open walking traversal and three authored PCM requests in
one/four views, plus real UDP audio and a moving four-view late join. The gate
retains 147 intermediate simulation ticks. Zanzibar has no authored portal
associations, so its unchanged portal/acoustic mask is expected. LAN actions
in the audio fixture enter through already-admitted host service calls.

The first corrected-build SDL replay failed before gameplay because its window
never received focus. That failure is preserved in `marker-fix-devices/`. The
second harness passed one view but lost focus during the four-view hold; that
failure remains in `marker-fix-devices-v2/`. Direct SDL initialization under
the Xvfb wrapper confirmed that the inherited Wayland environment still
selected the user's desktop Wayland driver. The final runner explicitly
selects X11, clears `WAYLAND_DISPLAY` and verifies the actual local test driver.
The harness retries an actual window focus request only before its gameplay
sequence; it does not synthesize focus events or relax later focus-loss checks.
Its separately compiled source and unchanged runtime object/library hashes are
in `marker-fix-devices-v3/harness-provenance.json`. The exact earlier desktop
focus-loss trigger is not identified. Earlier accepted wrapper-based tests
remain native SDL/Vulkan evidence but do not establish display isolation.

Earlier concurrent-launcher replacement failures and incomplete banks remain
preserved. The accepted base bank pins launchers, runtime and shaders around
each case. A snapshot-copy error was stopped before source edits; its partial
copy was removed, and the original phase-two source/binaries were preserved.

The full all-map goal remains **unfinished**. Remaining work includes attached
effect/function/marker lifecycles, original scheduling/resource/RNG producers,
automatic doors, falling/conveyor/riding/crushing physics, full prop/control
network/reset acceptance, Vista-only texture import and rendered original parity.

## Subsequent shared-build state on 2026-10-13

The root launcher pair now selects `build/halo2_native`, following concurrent
work. The frozen phase3 runtime above remains preserved evidence for its exact
build; it is no longer the current launcher selection. The retained event-clock
continuation passes 5,917 original comparisons and 21-map native contact/effect
regressions. Current launcher snow on/off/repeat captures retain 2,450 changed
pixels and identical repeats. Ordinary Zanzibar startup initially failed at
10F960; its complete original scenery-lightmap lookup is now added to source
and passes 512 exact comparisons. A later isolated runtime aborts at the shared
startup's missing 1888A0 input/camera constructor before scene construction.
Thus live factory acceptance and the full current normal-launcher bank are
still unproven. See `SOURCE_WORLD_EFFECT_EVENTS.md` and
`SOURCE_SCENERY_LIGHTMAP_LOOKUP_TU15.md`; do not treat the earlier phase3 receipt
as acceptance of these later source/runtime changes.
