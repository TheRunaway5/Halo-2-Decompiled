# TU1.5 live combat and particle continuation

The authority is the unchanged English OG Xbox TU1.5 executable, SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`,
and the original Xbox map assets. Vista establishes none of the gameplay
visual claims here. All canonical edits, source snapshots, builds and evidence
are inside `/home/eric/Developer/Halo-2-Decompiled`; owned external images and
DLC maps are read-only inputs. Full weapon and framebuffer/PCM parity remains
incomplete; the [weapon goal](WEAPONS_TU15_PARITY_GOAL.md) retains all 63 paths.

The user's acceptance criterion is pixel-for-pixel agreement with OG Xbox
TU1.5, including every third-person weapon holding pose. Source-node tests,
successful draws, and native-only pose galleries do not satisfy that criterion.
The native gallery was stopped on October 14 pending matched original
reference captures. Each accepted comparison must retain the authenticated
original executable, original and native frames, matching camera and pose
inputs, and an exact pixel difference result. Emulator captures must remain
identified as emulator evidence; they do not establish physical Xbox pixels.
No third-person pose currently has a completed matched pixel acceptance result.

## Completed source and native integration

The rebuilt composition contains `EA5A0 ->106510`; its whole-original 120-case
fixture passes. Complete authenticated bodies own live grenade selection,
admission, count debit, release, flight, fuse, radial damage and retirement.
`101DB8 ->178E30` publishes the completed source effect, controls, both colors
and every named location transform. Native presentation and LAN retain that
bank rather than rebuilding locations from one inferred orientation.

Complete `CE3A0` provides grenade selection, including signed count bytes and
positive/negative cycling. Its 735 original cases pass. Keyboard, controller,
native gameplay and UDP deliver a single selector press; the native protocol
press serials recover a short press after a lost packet without replaying it.
The original starting weapon-set parameter83, distinct from starting-primary
parameter89, supplies the plasma-grenade conversion in the source test.

The native bitmap metadata boundary preserves the complete original caller's
CPU outputs without constructing an Xbox device. The cached vertex boundary
at `183E17 ->439C0` resolves the real retained allocation, physical cache pages,
salt, stride and epoch. It replaces only CPU resource resolution; Xbox GPU
cache invalidation is outside that boundary. Independent whole-original
fixtures cover 200 bitmap cases and 180 vertex cases, including GP registers
and defined flags. This removes the live plasma explosion's device-command
fault without fabricating a GPU command buffer or copying vertices into a
parallel substitute cache.

Source action27 owns ordinary melee, sword contact and primary sword attacks.
The authored graph/channel/frame and actual WORLD nodes drive presentation;
native gameplay does not apply another sword strike. `D1FBF ->D13D0` publishes
the real contact material, damage definition, position, normal and mover frame.
Original damage requests feed the existing shield-effect renderer, and
source-owned recharge no longer creates a duplicate native request.

The source lunge regression now establishes a real distant target through
normal delivered movement on an authored clear floor corridor. It observes
source target confidence1 at distance2.40331, actual lunge/attack selection,
1.90626 units of closing movement, closest distance0.496893, shield contact,
damage and completed recovery across180 source ticks. This supersedes the
older failed near-target research fixture. The sword's terrain action15 is
actually twelve sound rows and zero visual rows; no extra sparks are invented.

## Firing and attachment presentation

Prepared firing requests run whole original `108A70`, including its actual
creator/refresh and completed color owner. A64-entry event journal retains
both hands and all requests between snapshots, colors, velocity, source
instance and controls. Actual primary/secondary held-weapon WORLD nodes are
published alongside body poses. UDP retains these fields under fragmentation,
loss, reordering, duplicates and stale snapshots. This is the native protocol,
not the Xbox wire format.

Scheduled firing in the newer canonical source authority now also publishes
completed108A70 requests to that journal. The observer runs the complete
callee, retains its actual salted allocation and colors, and obtains inherited
weapon velocity through wholeBC200 while preserving the caller ABI. Two
formerly internal callers10517F/107C40 retain the same observation boundary.
No second firing or effect-creation call is submitted by this publication.

The host consumes these scheduled journal requests for local first-person
particles. Their origin is retained as a separate bit so existing direct host
callbacks do not replay. Clients consume both origins. The protocol revision
is advanced for each composition. A direct original-command history spends
seven SMG rounds and publishes exactly seven accepted effects, without calling
the native weapon update. The frozen delivery retains its older native
ordinary-firing composition; this direct-source test and canonical source
changes are now also linked against the pinned canonical archive/header bank
`.local/tu15-canonical-archives-20261014`. All15 ordinary starting-weapon choices
pass normal delivered source firing with zero parallel native shots. All32
human/Elite starting-weapon choices pass actual body/held-weapon WORLD-pose
publication through crouch, walk, aim, jump and landing. These finite local
histories exclude the subsequently added LAN session-discovery ABI.

Whole original1046C0/107140 primary-charge arithmetic passes1,584 cases,
including signed timers and ordered clamping. Actual plasma-pistol input holds
for60ticks, releases the charged bolt and debits15percent source battery. Its
authored charging glow uses the attachment's `charging_effect` marker alias;
the240tick GPU check submits2360attachment draws and zero late draws after
draining. Mixed damage/particle attachments delegate damage to the original
source owner. The charged firing effect itself is sound-only and correctly
produces zero FP muzzle particles.

The source animation manager can select authored action variants beyond the
first clip resolved at preload. The renderer now decodes the actual graph and
index when first observed. A150tick Needler capture renders372firing draws and
52live projectile model draws, with a peak of four simultaneous projectiles.
The source projectile publication retains its actual WORLD root, velocity and
traveled distance instead of relying on the inactive native projectile bank.

The shared original weapon/projectile attachment presenter consumes completed
source slots, instance salts, function flags, controls, colors and locations.
It uses whole17BA50/17DDC0/17DD50, including1693AB's encoded FP graph-node
mapping, rather than interpreting the signed location word as a WORLD bone
index. The native caller owns only a temporary stack/output overlay; any write
to a source bank fails. All linked locations are retained. An independent
172-case unchanged-XBE replay compares full instruction paths, GP/SSE outputs,
defined flags and memory for WORLD/FP branches, both hands and four views.

Looking down can make168DED submit a third, mode0 body model. It is the
separate representation+0C render model, not the third-person biped model.
That authored model and its original node output now render in WORLD mode
alongside the two mode1 arms/weapon records. The old exact-two-model check
prevented the live grenade captures from reaching their explosion.

Normal FFA60 contact paths10098F,100A19 and100C1C now publish the completed
original effect after its actual creator returns.100A19 supplies the rocket
explosion.101FD1 and179520 remain observed too. No native projectile/contact,
damage call, effect allocation or random draw is submitted by this handoff.
The historical detonation transport bank retains these generic projectile
effects; diagnostics distinguish explosions by the actual proj+FC reference.
The15-weapon source firing matrix passes again with this shared resolver.

FP particle births, motion and retirement occur on completed game ticks.
`pose()` retains the actual registered FP animation, current weapon markers,
IK/ammunition state and camera adjustment; `draw()` is read-only. Undrawn
lifetimes no longer accumulate pending births. Dead colliding particles retire
on the original flag rather than waiting only for age. The regression includes
600 undrawn SMG and600 undrawn Needler lifetimes, original camera/event fixtures,
and read-only multi-view draws.

Every actual marker location gets its own location/emitter bank while sharing
the original system random fields. The sword blade has eight `glow_1`, eight
`glow_2` and one `flare` marker, not one location per name. Its retained
`blade_on` attachment follows the actual `turned_on` weapon function and the
original stop/restart lifecycle; switching away drains and retires it. Magnum
keeps its other marker systems when an authored `muzzle_flash` name resolves
zero locations; no marker alias is invented.

The original color arithmetic matches3,366 cases. Complete original event
clock execution matches5,917 cases. Collision-death admission matches27,648
cases; collider1's actual material/contact inputs also reach casing action10.
All sixteen original light fade tables are retained and18,592 full `17F8A0`
lookups pass, allowing the Carbine and Beam Rifle's authored alternate fades.

## Impact decal programs and source cache

Whole TU1.5 `43A30` supplies the pixel programs and layer-specific blend states.
Layers2/3 multiply; layer4 uses component max. Layer5 uses `ONE / inverse source
alpha` and its actual two-stage program, rather than a generic additive pass.
The native fallback now preserves tag RGB, changes only alpha while fading,
and schedules2,3,4,5 after lighting and before fog. The same verified programs
and blend equation are used by the actual retained-cache decal renderer.

The independent replay retains six complete pass outputs with eleven explicit
SDK command-observation boundaries and no patched instruction. The RGB
interpolation/packing routines execute537 cases without supplied services;
whole `17FB60` executes2,025 unexpired fade cases without supplied services.
Actual Carbine contacts create green glow vertices and fade their alpha while
retaining RGB. Cached decal layers4/5 are admitted alongside0..3 so plasma
explosion glow pages are not rejected by the source geometry collector.

`13A9CC ->180520` is now included before native source player/action delivery.
Its complete fade/retirement traversal, real cache epoch update and original
release callbacks operate on actual source pools/pages. This is an explicit
once-per-native-game-tick composition, not a claim that the complete original
render/game scheduling loop has been ported. A further160 whole-original
traversal cases compare the instruction path, GP/SSE registers, defined flags
and retained memory, including compound alpha propagation and cache epochs.
The complete32CC0C CRT conversion owner is included.

The strict `plasma-cache` diagnostic checks source-cache glow birth, alpha
changes and retirement. It initially fails because no surface effects are
admitted. Instruction tracing reaches101EA0 but never101FD1 or181500. Native
camera positions were absent from the original observer bank used by1755B0;
the new publication supplies the actual native camera to172200's retained
record. Original admission is retained.

Further tracing establishes a successful original floor query. The later
bitmap-readiness rejection came from the missing original texture-cache frame
owner. Complete12F110 now advances the real lock/load lists and cache epochs,
updates the timed scale and resets its ten-request counter. The native tick
boundary is explicit and does not claim the full Xbox render scheduler.
Forty-five independent whole-original cases compare ordered instructions,
GP/SSE registers, defined flags and memory, including the three original
GetTickCount instructions over supplied clock data. No service or instruction
is replaced in that fixture.

The actual live cache now contains `plasma_grenade_glow`, both
`plasma_explosion_glow_outer`/`inner`, `default_scorch` and `blood_splat` geometry.
Their authored glow lifetimes are1.75,2 and1.5 seconds. The strict test continues
through240 source ticks, including the self-hit death path. It passes glow
birth, packed-alpha changes and retirement of all three glow identities; the
longer-lived scorch/blood records remain. Native GPU captures now draw these
actual source-cache pages.

## Source body support and locomotion

The source biped's physical-result flags at+348 correctly change on takeoff
and landing. The native snapshot previously left `grounded` false even while
the original body was supported. It now publishes the actual unsupported bit,
independently of the source physical-state enum. Crouch continues to use the
actual source crouch amount.

The human source-WORLD regression covers actual standing, crouching, crouch
walk, reverse walk, yaw/pitch, jump and landing inputs across180 ticks. It
observes changing real leg/head transforms,44 airborne ticks and16 supported
ticks after landing. Elite selection is checked against profile+44 and the
actual spawned biped definition; the genuine55-node Elite and33-node human
both pass after complete reached physics callbacks are included.

The starting-weapon sweep currently passes30 of32 species/weapon combinations:
15 choices on each species, including the two rocket choices and sword.
Both Plasma Pistol rows remain gated by the tested composition's unjoined
trigger/energy owner. A strengthened regression additionally compares every
published body/held-weapon WORLD node to its actual salted source owner.

## Coverage and limits

The Lockout prepared-shot sweep passes for BR, Magnum, SMG, plasma rifle,
rocket, shotgun, sniper, Brute Shot, Needler, Carbine, Beam Rifle, brute plasma
rifle and Sentinel Beam. These are actual native prepared-shot callbacks through
the whole source effect owner. They do not prove the complete original trigger,
magazine, charge, projectile or animation manager. Plasma Pistol remains gated
in this tested composition until its actual charge/multiple-trigger owner is
joined; it is not force-enabled. Sword is tested through its own source melee
and retained-attachment path.

The current-main audit completes all38 original campaign/multiplayer/DLC map
loader executions, covering1,583 firing references:950 admit FP particle graphs,
290 are non-particle references and299 contain no FP particle systems. None of
these firing references has an authored effect beam.108 contain acceleration;
the original gameplay owner retains that acceleration while the FP presenter
can admit their particles. Eleven remaining particle-loader references need
campaign Wraith/Scarab coordinate/lifecycle and gravity-throne non-sound part
owners.33 Shotgun-empty references have a separate sound-only lifecycle gate.
Sound-only and non-FP references are not counted as missing particles. Lockout
now admits29 of48 references. These are loader results, not38-map gameplay or
framebuffer acceptance. Exact per-map outputs are in `main-all-map-muzzle`.

Remaining gates include world heat-warp/scene-texture binding, complete beam/
acceleration/force/death-child ownership, charge/alternate firing and overheat
attachments, all campaign/vehicle/objective weapons, original BSP decal clipping
for native-owned projectiles, full model-light/lens and blood/contact feedback,
complete split/LAN effect ownership, and matched Xbox framebuffer/PCM histories.
The separate original-source decal module improves cached geometry delivery but
does not prove its strict pixel equality. The earlier random walking triangle
flash is not reproduced by these combat captures and is not declared fixed.

## Finite GPU and geometry checks

Clean captures of the native composition complete for plasma(155 ticks),
frag(180), sword(120 with two views), Carbine(120), and walking(600). Plasma
publishes one actual detonation at129 and1392 explosion draws over27 frames;
frag detonates at143 and draws545 explosion submissions over38 frames.
Both also render actual retained source-cache decal pages. Sword submits15395
persistent blade draws and the Carbine submits158 firing draws. These counts
are diagnostic observations, not Xbox pixel-equivalence criteria.

The walking history covers21.3374 units with rotating camera and both
HALO2_VERIFY_FRAME_GEOMETRY andHALO2_VERIFY_BORROWED_FRAME_VERTICES enabled.
No upload mismatch, geometry error or Vulkan validation error is reported.
The intermittent triangle flash is not reproduced by this finite history;
its cause and final resolution remain unverified.
The later GPU-assisted run also times out in pipeline startup after600seconds
without drawing a gameplay frame. It establishes no triangle-flash or memory
leak result. Its owned process group is retired on timeout.

The pinned canonical continuation additionally passes a normal180tick CLI
match without custom birth/pre-update callbacks. Its frag capture publishes
one explosion at143,1225explosion draws over38frames, and24ordinary source
contact effects. Plasma publishes one explosion at129,1681draws over27frames,
and three contact effects. Both retain the actual source-cache decals and
looking-down body. A240tick charge capture draws2370charging attachments and
zero late attachments. These are finite observations, not matched Xbox pixels.

The current projectile attachment audit records53authored rows:20cont,
eightMGS2, eightligh, seveneffe, sixlsnd, threelens and oneNONE. The new combat
attachment presenter covers effe. Existing projectile model/light-volume
consumers are separate; this work does not establish complete cont geometry,
standalone object-light/lens/audio ownership or all63weapon parity.

## Reproduction and delivery

The tested coherent source snapshot is `.local/tu15-combat-main-20261013` and
its Release build is `build-tu15-combat-main-20261013`. Evidence is under
`analysis/weapon-effects-20261013/live-tu15`. `snapshot-scope.json` records the
excluded concurrent resource migration and the pinned baseline network test;
canonical main files and newer in-progress charged-weapon APIs are preserved.

```sh
ctest --test-dir build-tu15-combat-main-20261013 \
  -R 'original_tu15_(decal_pass|cache_vertex_view|grenade_selection|combat_action)|live_smg_muzzle_lifetimes|native_disc_player_animations' \
  --output-on-failure
build-tu15-combat-main-20261013/capture_source_melee_contact \
  assets/disc/maps/lockout.map assets/disc/maps /path/to/owned/TU1.5/default.xbe \
  analysis/weapon-effects-20261013/live-tu15/source-check plasma
```

Other source modes are `grenade`, `melee`, `sword`, `primary`, `target`,
`sword-target`, `primary-target`, `lunge` and `fire-N`. GPU reproduction uses
`debug/build_probe.py` after Ninja completes, then `debug/run_probe.py` for a
finite native input capture. Each capture records the exact binary hash.
All such inputs are explicitly native diagnostic histories, not matched Xbox
recordings. Vulkan validation and nonzero draws do not prove visual parity.

The current root launchers select the separately evolving MCC runtime. This
combat continuation preserves them and stages its tested runtime and dedicated
launchers separately inside the main project folder. The completion receipt
records the final artifact hashes, selected paths and validation results.

The newer local candidate is staged at
`.local/tu15-combat-delivery-20261014/halo2_native`; both dedicated launchers'
`--launcher-info` select that exact executable. Its source/object/archive/shader
hashes are recorded in `current-main-source-manifest.json`. The candidate
excludes subsequent session-discovery and vehicle base/model-light API joins;
canonical main source retains those changes and passes the five-unit syntax
check. Its compiled build shader/data paths remain dependencies. This is a
local combat candidate, not a completed whole-main/Online/vehicle delivery.
Use `.local/tu15-combat-delivery-20261014/Run-TU15-Combat` for the original-Xbox
local match. The copied C++ menu launchers retain the separately evolving
MCC-only startup gate; they do not establish playable TU1.5 menu delivery.


## Contrail continuation in the main checkout

`source_contrail_geometry_tu15_generated.cpp` lowers whole42CE0,42C40,BCD80,
17F8A0 and13A070. The unchanged executable passes229 geometry cases using
all23 authored Lockout contrail state definitions and explicit five-orientation,
interpolation, endpoint and fade boundaries. Comparison covers the original
instruction path, GP/SSE/flags, immutable source banks, complete geometry output
and output canaries. Dead scratch below the returned caller stack is excluded;
that storage does not belong to the completed output contract. Four runtime
point chains represent marker chains, not four independent observer views.

Whole52D30 CPU state with explicitly declared native GPU service returns supplies
352 pixel-store cases. Its bitmap/vertex binding and SDK submissions are external
boundaries in that fixture. These cases do not prove those native GPU services.
Contrails use the bitmap reference atcont+34 and rasterizer-global vertex
program4/class0 (`effect`), rather than a material-template weapon shader.

The new source reader validates full contrail/point identities, linked counts,
original bitmap frame selection and the FFFFFFF3 ordinary-world mask. It runs
42CE0 in a read-only source overlay and expands the completed24-byte strip
vertices with original winding. The Vulkan path retains the authored mips,
original pixel combiners, original blend-table operands, RGB-only write mask,
W-depth and source effect vertex program. These submissions join the transparent
effect queue. The actual main runtime and probe now build against current
main headers/archives, including subsequent session/vehicle API joins.

The missing original global frame owner13A9C7 ->17EAD0 now runs once per source
game tick, immediately before13A9CC ->180520. Whole17F3D0 advances/retire points
and reaches actual219330/219E00/219E40/21A2A0 PPHY/environment helpers; these are
lowered from complete authenticated bodies too.414 independent unchanged-XBE
cases compare complete tick instructions, GP/SSE/flags, point stores and pool
retirement, including authored PPHY states and original gravity constants.

The240tick current-main Needler GPU capture publishes88 contrail draws over34
frames, starting81 and ending114, with zero late trails after160. It also draws
52 actual projectile models and372 FP firing particles; all particles drain.
The earlier no-global-tick capture and the intervening missing219330 failure
are retained as diagnosis, not acceptance. Vulkan validation reports no error.
The source-owned Scorpion/Wraith acceleration gate is independently checked:
unowned FP gameplay parts are rejected; original-owned particles are retained.

The latest1,200-tick synchronization/geometry walking probe completed43.8368
world units without a reported validation error. It did not reproduce the
intermittent triangle flash and does not establish a fix for that report.


## Current-main firing location publication

Actual source requests now enter the global weapon journal even when the parent
is a vehicle/AI object with no native player slot. The separate per-player
latest request still requires its real full player identity. A diagnostic
history removes only the native player mapping and verifies that actual source
SMG firing remains in the global journal without publishing a per-player event.
No original object, firing return, effect allocation or damage is substituted.

The world presenter accepts completed named source locations without requiring
a separate native model-marker bank. Equivalent authored Needler marker banks
produce identical particle vertices, including moving-parent updates and
retirement. Retained repeated requests are resolved once per full instance.

A current-main startup integration failure exposed double translation of pool
instruction-observer addresses: the generated pool owner already emits selected
TU1.5 PCs. The live join now passes those PCs directly. The failing and corrected
integration logs are retained. Five focused tests pass, including both new
regressions and the three original contrail suites. The15-weapon current-main
firing matrix passes, with zero parallel native shots. The155tick plasma capture
publishes one actual explosion,1681 explosion draws over27 frames, source-cache
decals and110 shield draws over11 frames. These remain finite native captures.

## Completed render-pose handoff, October 14

The standing SMG diagnostic exposed a concrete missing presentation step:
the published simulation WORLD bank still had the unsampled finger pose.
Complete original BF640 changes twelve Spartan finger nodes and eighteen
Elite finger nodes before the model palette is uploaded. The third-person
renderer was skinning its earlier snapshot instead of that completed bank.

The canonical world now copies the effective model-order WORLD inputs at the
original model-node handoff, after BF640 and the deferred-pose gates. This
also retains weapon inputs whose deferred gate returns without calling BF640,
before shared scratch is reused. Frame reset and authority import discard
that transient bank. The ordinary model-builder handoff retains these matrices
for the third-person body and held weapon; source-authoritative left-hand
weapons also consume their published WORLD nodes. Finite third-person captures
explicitly request the same complete original builder for the local body.
No replacement grip or added native IK correction supplies these source poses.

The broader render-pose sweep also reached original 1112E0/110C10 overlays
and 2130C0 mask combination, absent from the previously joined render module.
The model generator now retains every nonzero callback from all nine literal
TU1.5 codec descriptors at 48C330..48C497. Descriptor names, bytes, callback
addresses and complete instruction bodies are recorded in
`analysis/weapon-effects-20261013/live-tu15/render-pose-fix/model-render-generation-v3`.
This closes the observed Elite Magnum overlay and Elite battle-rifle decoder faults.

The CPU sweep passes 30/30 species/starting-weapon combinations, comparing
all selected uploaded palette matrices against the retained completed render
nodes, including original region maps and compound nodes. The next-frame
absence check also passes. Three focused CTests cover the Spartan SMG, Elite
SMG and the previously failing Elite Magnum. An actual 100-frame native
third-person capture checks both body and held-weapon node banks against the
completed builder on every frame. The ordinary two-player capture checks 200
third-person body/held draws and 200 first-person draws. The refreshed v4
matrix and captures include the effective-input observer after all gates. These are pose-handoff and finite GPU checks;
they do not constitute matched original/native pixel acceptance.

The private unchanged TU1.5 reference session produced authenticated live
body/weapon/camera/cache records and an actual front-view SMG screenshot.
Its optional full-RAM QMP export deadlocked after 46,243,840 bytes; the partial
file is explicitly marked incomplete. The owned session was stopped, and a
fresh private session was started. No guest code or guest pose memory was
patched to obtain the reference. All mutable session files remain in this
checkout.

`held-pose-acceptance-inventory.json` conservatively retains 31 authored
weapon paths with first-person model references, including objectives and
special campaign paths omitted from the fifteen starting choices. A model
reference alone does not prove player pickup/holding admission. No candidate
is silently excluded and zero matched pixel pairs are accepted so far.

The October15 continuation below resolves the earlier normal dual-pickup
NULL binding at4DC04C. Dedicated/client render-pose production, the complete
original pose gallery, the walking triangle-flash reproduction, and pixel
equality remain unfinished.


## Original third-person geometry and matched input capture, October 14

The source-palette distance probe selects biped LOD4 at normalized distances0
and2, LOD2 at5 and10, and LOD0 at20,50 and100. The body/held legacy path was
using preview-loaded geometry instead. Third-person resource preparation now
retains all six authored geometry levels and their region permutations. Every
completed original collector copies its actual model, LOD and ordered section
bytes for the current view. Bodies and held weapons skin and submit that bank;
shared split-screen pose nodes no longer imply shared per-view geometry. The
source-authoritative secondary held weapon uses the same section service.
A missing completed collector still reaches the prior presentation path; this
join does not certify client/departed-body admission or original class1 passes.

The single and split Spartan SMG GPU probes now independently read the actual
source cache's per-view LOD and reject vertices from another LOD. They pass100
and200 body/held draws respectively. Elite Brute plasma rifle and Spartan
energy sword also pass100-frame geometry/pose captures. These remain bounded
native checks; zero pixel pairs have been accepted.

The private unchanged TU1.5 session was restored from its saved front-pose
snapshot. `tools/capture_xbox_tu15_held_pose_reference.py` observes original
13A2E0, the effective input at13A357, and its completed return for both the body
and held weapon in one source view. It retains exact WORLD nodes, ordered
sections, palettes, object/cache records, colors, lighting and camera inputs.
The completed view1 SMG capture selects body LOD3 (sections12..15) and weapon
LOD1 (section1). Its live text authenticates to the owned TU1.5 executable.
Hardware breakpoints only observe the guest; guest code and pose memory are
unchanged. The capture remains emulator evidence, not physical Xbox proof.

`HALO2_HELD_POSE_SHADING_REFERENCE` is a compile-time diagnostic seam used only
by the held-pose shading probe. Ordinary binaries have no retained-pose lookup.
This replay supplies the captured render inputs to isolate native renderer
residuals. It is explicitly component evidence, never independent gameplay
animation acceptance. Its first SMG body/weapon palettes reproduce the captured
original13A2E0 output bit for bit. Strict GPU coverage and RGB comparisons are
now retained below; the full acceptance gate remains open.

## Strict held-pose pixel gate, October 14

The retained SMG replay reproduces all44 original palette matrices, all48
camera-bank words and all16 uploaded c0..c3 words exactly. The original2C470
split-screen policy is retained separately; its coverage/secondary/effect
scales are0.8/0.5/0.3. Matching original GPU draws use exact c0/c50 bytes;
native draws are selected by exact WORLD-position words. Neither selection
uses image alignment or an arbitrary screen-space crop around the character.

The declared source viewport comparison covers1,364 triangle pixels with
zero coverage differences. Strict RGB still fails: the initial NVIDIA replay
has1,025 unequal pixels, max13 channel levels. A fresh Lavapipe replay reduces
this to485 pixels, max2. Decoding compressed textures with the existing pinned
decoder reproduces all130,944 original environment-cube bytes across six
faces/five mips; the isolated decoded-storage control reduces the software
residual to59 pixels, max2. Its NVIDIA control still fails967 pixels, max13.
These are diagnostic controls, not accepted poses or a production decoder
change. The original normal-map mip0 also equals native mip2 byte for byte.

`tools/compare_held_pose_pixels.py` rejects any changed RGB channel, dimension
mismatch or empty mask. Its negative controls include a one-level channel
change and a one-pixel shift. The acceptance tolerance is zero. The generic
world shader now receives the source-derived unwritten texture-W mask, reusing
the independently checked original-program analysis; this preserves original
initialized W=1 through projected sampling. Eight focused checks pass, but
the retained SMG decoded-storage replay still fails59 software pixels and967
NVIDIA pixels against the Mesa original. Replaying the original draw on NVIDIA
as well reduces the NVIDIA residual to538 pixels, max10; it still fails. The
constant-W preservation does not close this pose gate.

`render-pose-fix/weapon-pose-reference-matrix.json` retains actual original
render inputs for all15 resident Foundation starting-weapon choices. Seven
have GPU exports; all15 retain completed nodes/palettes and screenshots. The
fourteen additional weapon fixtures explicitly declare original B9960/B9B70
constructor inputs, then use normal controller pickup. Constructors preserve
the original executable and player pose. These fixtures are not described as
normal-play-only provenance. Elite, dual wield, remaining authored inventory,
independent native animation and the complete strict pixel matrix remain open.

The full-framebuffer/resident-image control improves the retained SMG RGB
residual to26 software pixels (max2) and528 NVIDIA pixels (max10), with the
same zero-difference1,364-pixel geometry coverage. These results still fail
the user's zero-tolerance requirement. Retained resident-image controls do
not establish production texture residency or independent animation.

`debug/probe_raw_held_pose_draw.cpp` isolates original event143568: body
section12/part0, compiled vertex class4, shader LOD1,52 original instruction
words and the exact consumed constant bank. Its269-pixel contribution passes
RGBA exactly on the software backend, with zero validation errors. The same
draw fails58 pixels/106 channels (max39) on NVIDIA, also with zero validation
errors. Seven mismatches retain the seed scene color;51 change to a different
color. This is one retained-input draw consumer, not an accepted live pose.

Raw vertex compiler controls (ordinary port source, precise outputs, and an
explicitly adapted retained original shader confined to the diagnostic folder)
all reproduce the same NVIDIA58-pixel failure. Direct fetched bone-weight and
index outputs match for all842 vertices. Isolated post-vertex readbacks and
post-geometry readbacks give different residuals; the isolated readback alone
is therefore insufficient to certify the actual draw. The original reference
remains emulator output, with no physical Xbox framebuffer comparison.

## Retained-input validation correction, October 15

The later live unit base/light join can submit its own completed simulated
pose and LOD after the retained-input animation observer. This invalidated
the newer diagnostic scene comparisons: metadata still described6,498 body
vertices, while actual GPU body draws used a different geometry bank. The
new GPU WORLD-word check rejects the capture with zero matching body/weapon
draws. The1,347-pixel NVIDIA and1,345-pixel software residuals from those
captures are not qualified retained-pose shading measurements.

A single-shader isolated A/B confirmed that projection ordering changed only
one masked pixel by one level in that invalid scene. It did not cause the
larger geometry/pose discrepancy. Original texture normalization before
projection remains in the ordinary shader; no tolerance or color correction
was introduced.

The compile-time retained-input diagnostic now keeps its captured bodies and
weapons on the existing explicitly supplied node/section path. Ordinary
gameplay continues through the live unit base/light owner. The observer also
checks every retained body position and its exact extent. GPU verification
remains separate and mandatory.

`compare_held_pose_pixels.py` now requires a same-capture actual GPU geometry
receipt linked by hash to its coverage comparison. It rejects incomplete GPU
geometry, another capture's screenshot, another coverage receipt, or another
declared viewport. A regression test supplies identical RGB images with stale
geometry and requires rejection. Earlier receipts retain their historical
scope; the complete all-weapon pixel acceptance count remains zero.

The corrected current-main captures pass actual GPU geometry validation for15
draws on both backends. Their fresh coverage comparisons have1,364 original
and native pixels with zero differences. The same-capture RGB comparisons
still fail26 software pixels (max2) and528 NVIDIA pixels (max10). The separate
current locomotion matrix passes30/30 Spartan/Elite weapon cases through180
ticks of standing, crouching, forward/reverse walking, yaw/pitch, jump and
landing. Eight focused runtime checks and three comparator tests pass.

The user selected a physical original Xbox TU1.5 framebuffer as the final
pixel authority and confirmed no physical Xbox is available. The retained
original-executable emulator captures remain diagnostic references. Neither
the passing source/geometry checks nor any future emulator-only zero
difference can be labeled physical Xbox pixel parity without that reference.

All30 current Spartan/Elite rendered-palette cases also pass across the15
resident weapons, including completed deferred finger nodes and frame cleanup.
The gravity-lift animation question now has a dedicated Lockout ride check.
The authenticated complete TU1.5 airborne producer atE6620 contains133
instructions and requests `airborne` (0800001E) atE682B. The new
`capture_source_lift_animation_tu15` fixture supplies an authored approach
position and ordinary forward/look controls, with no jump input. Original
field membership, physics, animation selection and render construction execute
in the actual source world. It records the original manager's graph, packed
clip, frame, mode, weapon class and weapon type on every tick.

Both registered Spartan/Elite SMG checks pass. The broader matrix passes all30
species/weapon cases across the15 resident starting weapons, including the
energy sword:9,000ticks,18,000 body/held-weapon model checks and404,700 uploaded
palette matrices. Each case rises5.090715885 units, reaches41 distinct ascending
airborne frame values, and selects soft landing (0900001F) after actual support
returns. Completed render nodes match the original builder's matrices bit for
bit, and published physical WORLD nodes match the current source owner.
The transition follows the original scheduler; the fixture does not force
airborne on the first unsupported tick or invent a separate gravity-lift clip.
The recorded original SMG selections are `combat/rifle/smg` for Spartans and
`combat/pistol/smg` for Elites. Plasma rifles also retain the original
species-dependent class. The complete30-row selection inventory is retained in
`weapon-pose-selection.json`; these selector names establish source binding,
not final visual agreement with an Xbox framebuffer.

Evidence is retained under
`analysis/weapon-effects-20261013/live-tu15/render-pose-fix/gravity-lift-animation-20261015/`:
`main-build-verified-receipt.json`, `verified-focused-tests.log`, and
`weapon-matrix-v1/receipt.json` with per-tick frame and lift-member traces.
The preliminary captures and failed build logs remain labeled separately.
One redundant HUD `representations` declaration was removed to restore the
current main build; both users retain the same authored table.

These are finite source-world and render-palette checks for Lockout, not GPU
framebuffer comparisons, other lift placements, dual-wielded lift poses or
physical Xbox evidence. No production animation-selection override was needed.
Full weapon-pose pixel acceptance remains zero.


## Normal dual pickup and original firing particles, October15

The main checkout now executes startup row23, complete original69220, and
69FF0 with the authenticated692D0 caller's actual world/watcher/registry
inputs after159FF0 resets the engine. The local game-options mode constructs
world type1 itself. No world type or pickup result is supplied. Complete
6A8E0/88470/158240 and their reached world continuations register real players;
complete25AAC2 supplies the pickup feedback wrapper. This bounded join does
not include692D0's separate lobby/watcher callbacks or establish original
network simulation modes3..5.

`tools/capture_source_dual_animation_tu15.cpp` picks up a matching authored
map weapon using normal source controller input, then exercises right, left
and simultaneous fire, reload, aim, crouch walk and jump. Its pinned matrix
passes72 histories: six dual-compatible weapons squared, for Spartan and
Elite. Brute plasma-rifle placements use the original Turf cache. The real
salted offhand, both FP registrations, completed body/weapon WORLD nodes and
original uploaded render palettes are checked. It observes1,244 completed
firing requests and checks920,400 palette matrices. The original focused
Spartan and Elite CTests also pass.

The historical source particle diagnostic completed72 pair histories with
34,214 FP submissions, but advanced particles before the game tick. That
ordering differs from original12E142/12E151 and is not accepted as the final
particle continuity check. The corrected `source-particle-matrix-v5` passes
all72 pairs with29,345 FP draws,39,893 observing-view WORLD draws and zero
draws beyond the expiry deadline. It checks920,400 palette matrices and
1,244 completed firing requests. Births, random streams, movement and retirement come from the
original particle pools and179DF0 phase;1770C0 selects the view/pass. Admitted
local source firing graphs replace the legacy reconstructed muzzle particle
history. The resource loop now includes authored firing
banks even when the specialized stock SMG does not use the generic barrel
presenter. A GPU regression that requires original salted particle identities
first failed on the legacy fallback and now passes with one and two local
views, including expiry. Concrete impact debris is checked separately from
SMG muzzle materials; its authored class8 is valid.

WORLD emission was suppressed because the native observer published position
without its lens:251C80 multiplies distance by observer+50, then stores the
resulting LOD factor in location+1C. The zero scale produced zero emission.
The native graphics boundary supplies its actual observer packet in retained
caller-stack scratch and executes complete174BF0, including the original174C01
FOV/baseline division. Only its completed+50 scale is published to the real
observer, preserving its controller/aim/displacement history.
The generated draw module contains1,887 authenticated instructions across19
complete bodies. Source SMG WORLD location factors change from0 to1 and the
original emitters create live particles. The split render camera's separate
lighting scale is not substituted for the observer's unscaled FOV ratio.

The corrected diagnostic follows actual location+10 emitter positions;
newly allocated location+20 bounding spheres are not yet computed. Its expiry
deadline also allows Magnum's authored3..5-second casing lifespan, measured
from the final completed firing request. Both fixes correct diagnostic inputs
or assertions, without supplying particle births or visibility results.
The retained Backwash test separately passes240 game-then-particle frames,
14 final swarm draws, repeated-view particle/RNG invariance and bounded queue
reuse and unchanged observer history. Seven focused original/native
registration, dual-effects and swarm tests pass.

The current native render collector also reached an unbound4B96E ->13A2E0
palette path. Complete13A2E0 and its13A1A0 inverse-node helper are now lowered
from the authenticated executable, adding347 original instructions. The
model-render module contains15,269 instructions. Its BF640 overlay call,
compound weights,255 sentinels and section/resource requests remain original.

Evidence is under
`analysis/weapon-effects-20261013/live-tu15/effects-dual-20261015-v1`, including
pinned binaries, original instruction listings, matrix receipts and GPU logs.
The expanded WORLD firing-particle route and its second-view culling fixture
pass the bounded72-pair source sweep. The final graphics regression uses a
declared original30Hz source cadence with actual SDL input and a500-frame
budget. Primary-only, offhand-only and simultaneous firing have separate
particle-owner assertions, alongside animation, split-view and expiry checks.
Free-running host-frame checks are separate: under heavy load, brief100ms
muzzle effects can reach their transparent endpoint before submission. The
controlled-cadence checks do not certify real-time frame pacing.
All three final GPU cases pass (`gpu-v11-receipt.json`) with geometry and
borrowed-vertex upload validation. The dual run observes58 primary-only,
36 offhand-only and26/28 simultaneous source hand-particle submissions.

| Weapon | Source dual graph/model checks, both roles and species | Source FP/WORLD firing particles | Controlled30Hz SDL GPU |
| --- | --- | --- | --- |
| Magnum | Pass with all six partners | Pass | Not separately exercised |
| SMG | Pass with all six partners | Pass | Pass: single, split and dual |
| Plasma pistol | Pass with all six partners | Pass | Not separately exercised |
| Plasma rifle | Pass with all six partners | Pass | Not separately exercised |
| Needler | Pass with all six partners | Pass | Not separately exercised |
| Brute plasma rifle | Pass with all six partners | Pass | Not separately exercised |

Concurrent edits to shared weapon/camera headers produced mixed-layout
validation executables. Those failed runs are retained as diagnostics and
are not accepted. Final validation uses a frozen copy of the main checkout
inside `.local/effects-validation-source-20261015` and
`.local/effects-frozen-build-20261015`; the implemented source changes remain
in the main `src`, `include`, `tools`, `tests` and `cmake` directories. The
frozen manifest records the tested source hashes, and the effects/dual/palette
implementation files match the main checkout. The frozen native runtime and
all four validation targets build successfully.

The complete original particle draw consumer,
client effect ownership, all impact/explosion graphs and exact framebuffer
comparisons remain open. These finite local/source/GPU checks establish no
physical Xbox pixel acceptance.
