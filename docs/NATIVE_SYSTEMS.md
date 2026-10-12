# Native simulation workbench

The runtime now exercises physics, AI, combat, scripts and audio in C++20,
inside the SDL2/Vulkan application. This is a development milestone, not a
complete or behavior-matched Halo 2 gameplay engine. Selected movement, jump,
capsule, recharge, ballistic, armor-lookup, shield-damage and body-impact kernels pass
53,018 original-instruction comparisons. Crouch, nine scalar-cast helpers and
fade curve 5 retain their separate evidence. The exact boundaries and remaining
sandbox work are in [PHYSICS_PARITY.md](PHYSICS_PARITY.md).

## Run

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
./build/halo2_native --play --weapon 'objects\weapons\rifle\battle_rifle\battle_rifle'
./build/halo2_native --map assets/disc/maps/01b_spacestation.map --play \
  --weapon 'objects\weapons\rifle\battle_rifle\battle_rifle' \
  --place-squad 1st_hum_floor_mar --script player_count
```

WASD and mouse control movement/look. Space jumps, left Ctrl crouches, left
mouse fires while captured, F reloads, Tab respawns at the next authored
player start, and F2 toggles free flight. Simulation continues during free
flight. The title bar displays normalized vitality, loaded/reserve ammunition,
actor counts and script fault counts. Weapon selection equips a full magazine
and the tag's inventory capacity for workbench testing. A respawn preserves
this selection. The first available player representation supplies the biped;
this does not implement campaign character selection or multiplayer loadouts.

Supported actors now use original highest-LOD render meshes, authored default
variant overrides, bone palettes and skinned idle clips. First-person hands and
weapons use the weapon's animation graph, matched by bone StringID. The normal
weapon action table supplies idle, fire, and reload clips. Geometry and textures
are native Vulkan data; model skinning and animation decoding are C++.
Unavailable models retain colored diagnostic boxes. Projectiles remain small
boxes. This is not the complete retail animation/state/rendering system.

Inspect the actual scenario's content before choosing a squad/script:

```bash
./build/halo2_native --inspect-systems assets/disc/maps/01b_spacestation.map
./build/halo2_native --map assets/disc/maps/01b_spacestation.map --simulate 600 \
  --place-squad 1st_cov_wv1_grt
./build/halo2_native --map assets/disc/maps/01b_spacestation.map --simulate 30 \
  --script difficulty_normal
```

`--simulate` performs fixed 30 Hz ticks without creating a window/audio device.
It uses neutral player input. Script faults are printed and produce a failing
exit status. `--campaign` activates authored startup, dormant and continuous
threads. It currently cannot complete a campaign mission: cinematic, object,
device, dialogue and mission services are incomplete. Cairo now executes the
original skip/save/wait wrappers, player stashing, native asset prediction and
cinematic object cleanup, then stops at `object_set_permutation`. Cross-BSP
collision/navigation/render changes and native checkpoints now execute.
Sound-class fades feed the SDL mixer; objective/flashlight/ambience controls
have saved state but still need their complete UI/playback consumers.
Player input/camera, fade/letterbox and
mission-segment telemetry also execute. A
fault stops that thread at the unsupported operation; earlier effects are not
rolled back. Other scheduled threads continue. No campaign completion claim
comes from running a few supported static scripts.

## Implemented behavior and boundaries

| System | Native implementation | Fidelity boundary |
| --- | --- | --- |
| Physics | Reads collision surfaces/edge loops and instance collision; triangle BVH, rays and subdivided capsule movement; floor support, jump, crouch and velocity sliding. Ordinary player movement uses recovered input and walking/air kernels, continuous crouch blending and cached slope bounds. Gravity is the retail `4.1712594`, without the erroneous zero map multiplier. | Contact generation/solver, support probe, mode selection and actor integration remain host implementations. Havok, moving scenery/devices, vehicles, ragdolls, actor contacts, stepping, breakables, conveyor behavior and retail surface masks remain absent. Rays treat recovered polygons as two-sided. |
| Recovered kernels | `1E6120` player intent, `1F0870` walking/air velocity, `E4680` jump gate, `DD7D0` crouch delta, `DF5F0` capsule height, `FA170` gravity, `F8EB0` ballistic segment, ordinary `D5DE0` recharge, `D5BC0` armor lookup, `D9110` shield damage, `D8CB0/D9020/DB090` body impact, nine `20A000..20A110` casts and curve-5 fade. | Selected branches have bit-exact XBE evidence with named external boundaries. These do not reproduce the whole biped/projectile/damage update. Overshields, full difficulty scaling, actual effects and original engine update ordering remain incomplete. See the per-kernel table in `PHYSICS_PARITY.md`. |
| AI | Authored squad locations/normal count and weapon/character palettes, inherited character parameters, default team resolution, vision range/angle and acknowledgement delay, collision-occluded perception, basic alert/attack states, nearest visible hostile target selection, reloads, and routes through authored navigation sectors/portals. Scripted allegiance changes update the team matrix. | The brain, target selection and steering are provisional host behavior. Only grounded starts in the selected BSP can spawn; unknown BSP membership uses a nearby support ray. Route search uses host geometric costs, not recovered retail path costs. AI orders, cover, vehicles, tactical animation, flying/swarm movement, placement scripts, difficulty upgrades and authored encounters remain missing. |
| Combat | First barrel/magazine fields, rate, burst minimum/recovery, inventory/reload, instantaneous versus ballistic projectiles, ray/capsule hits and recharge. Ballistic stepping, authored shield/body armor lookup, shield overflow, body protection/default section absorption and thresholded stun use recovered kernels. Firing effect lookup finds a sound resource. | Trigger selection and hit generation remain simplified host behavior; region response/state and death/effect services remain incomplete. Authored projectile count, horizontal-fan distribution and minimum cone error are implemented; the random cone sampler remains host behavior. Continuous firing error, difficulty/team scaling, model hit regions/headshots, explosion/radius damage, penetration, tracking, bouncing, melee, grenades, dual wielding, heat/charge, pickup objects and full animation/effect synchronization remain absent. First firing sound/first pitch/permutation is used, not the retail effect/audio selection system. |
| Scripts | Actual salted 20-byte expression nodes, stale-handle/poison checks, string rebasing, literal/arithmetic global initialization, bounded stacks and cooperative 30 Hz threads. Supports `begin`, `if`, `and/or/not`, arithmetic/comparison, `set`, `sleep` (including target threads), `sleep_forever`, `sleep_until` with intervals/timeouts, `wake`, level thread registration and parameterless static calls. Dormancy preserves continuation and thread slots are reused. | Partial evaluator, not a port of the complete HS VM. Parameterized calls, command-script actor contexts, external globals and most of the 390 observed builtins are unsupported and fault explicitly. Global initialization only supports literals, references and arithmetic. |
| Script host | Squad `ai_place/ai_erase`, allegiance create/remove, player input/camera enable and player vitality queries, impulse/loop start/stop/alternate, initial sound attachment position, `print`, mission-segment state, normal-difficulty query, a one-player list/count/get and `unit` cast. Named scenario object creation/destruction, hiding/teleport, supported object-name casts and unattached volume tests connect to the scene. | AI groups/locations, actor object handles, complete object lists, player hide/invulnerability, moving audio attachment, save/revert and later mission services remain missing. The one-player list is a native host handle rather than the retail object pool. |
| Audio | `snd!` and `ugh!` pitch/permutation/chunk resolution, shared-resource reads, FFmpeg Xbox ADPCM/IMA WAV/16-bit PCM and ASF/WMA stream decoding, float stereo 48 kHz resampling, SDL callback mixing, gain/pan, voice handles and distance attenuation. `lsnd` tracks resolve intro/loop/outro resources, repeat across callback boundaries, and support stop/alternate transitions. | Host mixer/attenuation. Loop sequencing is a host implementation. Fade/crossfade envelopes, random detail tracks, synchronization flags, alternate outro selection, dialogue selection, authored music scheduling, DSP, occlusion, retail priorities, runtime permutation/pitch choice and faithful spatial audio remain absent. |
| Models and animation | Original object/first-person model references, compression bounds, highest available LOD, authored placement/default variant overrides, node maps, inverse bind transforms, CPU skinning, action-table lookup, codecs 1/3/4/5/6/7, dense/sparse channel masks and 30 Hz sampling. First-person arms/weapons share one graph, original offset/camera-control adjustment and authored horizontal player FOV. The first-person pass clears depth and binds each actual material. Transparent-only skinned sections use their own maximum influence count. | Variant preview selects the first positive-weight alternative. Random appearances, state permutations, inherited node maps, remaining animation root adjustments, markers/weapon attachment, locomotion, hit/death/overlay/transition blending, sway, root motion and original shaders remain incomplete. Unavailable action/skeleton combinations report a fallback. Placement evidence and gates: [FIRST_PERSON_PARITY.md](FIRST_PERSON_PARITY.md). |
| Scene/presentation | Bounded object-name/placement/palette, camera-point/flag and trigger parsing. Scene objects have stable generation handles, authored Euler transforms, normalized placement scale, BSP placement filtering and original meshes/variants. Animated cameras sample one-node jmad tracks and compose the scenario anchor. Script and player FOV are horizontal and convert with viewport aspect; the player value comes from its biped tag. FOV transitions are linear. Camera timers, screen fades, letterbox and HUD/reticle fade state update at 30 Hz. Vulkan draws full-screen blended overlays. | Scene transforms/lifetime are host services, without dynamic object physics or unit/device simulation. Object-attached volumes, relative/dolly/rolled camera points, object-relative cameras, animated cutscene actors, camera blending modes and full HUD are unsupported. Fade-after-letterbox compositing and HUD interpolation are native presentation choices; full original compositing has not been established. Viewer camera defaults remain native settings; player zoom/magnification remains pending. |

## Audio

FFmpeg development libraries `libavcodec`, `libavutil`, `libswresample`, and
pkg-config are additional build dependencies. The decoder must expose
`AV_CODEC_ID_ADPCM_IMA_XBOX`; a generic IMA WAV decoder produces a different
sample count for Xbox blocks. The local build uses libavcodec `63.1.102`.
SDL output failure is reported and simulation can continue muted; an explicitly
requested `--sound` playback fails if its device cannot be opened.

```bash
./build/halo2_native --map assets/disc/maps/01b_spacestation.map \
  --sound 'sound\weapons\battle_rifle\fire_burst'
./build/halo2_native --map assets/disc/maps/01b_spacestation.map \
  --decode-sound 'sound\weapons\battle_rifle\fire_burst' \
  --audio-output analysis/battle-rifle.wav
```

`--sound` plays the first permutation once at reduced gain in the window.
`--mute` disables runtime audio. Complete tag names avoid ambiguous substring
matches. WAV export is stereo 48 kHz PCM16. A local Cairo battle-rifle
permutation contains 142,200 encoded bytes across four chunks. It decodes to
126,400 source stereo frames at 44.1 kHz and 137,579 frames after resampling.

## Verification

```bash
ctest --test-dir build --output-on-failure
./build/halo2_native --validate-systems assets/disc/maps
cmake -S . -B build-asan -G Ninja -DCMAKE_BUILD_TYPE=Debug -DHALO2_SANITIZERS=ON
cmake --build build-asan -j
ASAN_OPTIONS=detect_leaks=1 ctest --test-dir build-asan --output-on-failure
.tools/python/bin/python tools/differential_kernels.py
.tools/python/bin/python tools/differential_physics.py
.tools/python/bin/python tools/differential_damage.py
```

The differential harness requires Unicorn `2.1.4` in the separate
`.tools/python` environment. It pins the retail XBE hash, maps the original
sections, constructs synthetic tag/object/time pools, executes `DD7D0` with
its Xbox register calling convention, and compares the return's float bits
with `libhalo2_kernels.so`. It passes 10,384 crouch boundary/random cases, including
held/released/forced crouch and positive zero. Another 15,210 cases cover nine
retail cast helpers and 15 type pairs, preserving raw high bytes, signed
shorts, enum-to-real conversion, negative zero, infinities and NaNs. Test-only x86 execution is not
part of the native application. Its scope excludes the external actor-speed
query and does not prove full physics parity.

The system checks include frontend checks, bounded cache/disc/texture parsing, all original map
geometry, all 30 maps/80 BSPs of system content, synthetic floor/wall/jump/
crouch, damage/recharge, AI visibility through occluders, sleep continuation,
branch suppression, low-word global handles, assignments and explicit script failure, Xbox ADPCM's 64 frames per
block, and an SDL dummy-device callback. Retail checks exercise Lockout ground/
jump and Cairo marine placements, battle-rifle burst/reload, original static
scripts (`player0`, `player_count`, `difficulty_normal`) and actual firing
resources. Native and retail physics/system tests pass normal and separate ASan/UBSan
builds. Current full-suite results, kernel differentials, gravity/landing
regression and authored shield/body armor fixtures are detailed in [PHYSICS_PARITY.md](PHYSICS_PARITY.md).

The latest integrated Cairo runs rendered battle-rifle and SMG first-person
arms/weapon idles plus three skinned marines on RTX 5080, resized to 960x600,
and emitted no Khronos synchronization validation messages. The SMG run played
an actual spaceship-roomtone looping track through SDL/PipeWire: 61,952 output
frames, peak 0.00457679. This verifies native GPU/audio execution; it does not
prove original visuals, gameplay/mission parity, controller hardware or output
through physical speakers.

The expanded tests cover connected/disconnected authored portals, multi-projectile
shots per cartridge, dormant wake/continuation, interval polling, loop wrap/stop,
Marine inverse binds and sparse pelvis keys, first-person graph bone matching,
and SMG geometry/idle fixtures. Normal and ASan/UBSan CTest runs pass all six groups.

Latest evidence: `analysis/systems/{playable-ctest.log,playable-asan.log,
models-smoke.log,smg-audio-smoke.log,cairo-models.png,cairo-smg-audio.png,
campaign-startup.log}`. Existing kernel oracle/audio evidence remains separate.
Detailed primary research: [SYSTEMS_RESEARCH.md](SYSTEMS_RESEARCH.md) and
[PLAYABLE_RESEARCH.md](PLAYABLE_RESEARCH.md).

For the verified SMG roomtone run:

```bash
./build/halo2_native --map assets/disc/maps/01b_spacestation.map --play \
  --weapon 'objects\weapons\rifle\smg\smg' --place-squad 1st_hum_floor_mar \
  --loop-sound 'sound\ambience\roomtones\spaceship_roomtone\spaceship_roomtone'
```

## Scene continuation verification

```bash
./build/halo2_native --map assets/disc/maps/01b_spacestation.map \
  --camera-animation 'objects\characters\cinematic_camera\x02\x02' x02_01 17
./build/halo2_native --map assets/disc/maps/01b_spacestation.map \
  --script cinematic_snap_to_white --simulate 3
```

The preview invokes the same native camera sampler as script animation calls.
It uses a 60 degree horizontal FOV, as set by Cairo's first scene script;
the viewport conversion follows reconstructed `171D90`. This is not a full
retail projection/letterbox/observer port. It does not start the campaign or
animate cinematic actors. Cairo's camera graph
has 11 clips; the first has one node and 516 authored frames, hence 515 script
ticks. Frame 0 and the final frame are checked against independently decoded
local bytes. Camera forward/up axes and anchor composition were checked against
original instructions, but the complete camera/animation module has not been
differential-tested. Script camera timers and presentation advance after VM
execution in the native tick.

The expanded six test groups pass normal and ASan/UBSan builds. Tests cover
481 Cairo object names/1,239 placements, 34 flags/52 volumes, script-created
objects, stale handles, authored scale-zero normalization and bounding offsets,
oriented strict trigger bounds, anchor composition/final-frame clamping, and
both actual Cairo `cinematic_snap_to_white` / `cinematic_fade_from_white` scripts.
An extra Brute model check covers its transparent-only skinned sections.

The native fade kernel receives the original 1,024-byte curve from the local
`OriginalData` provider, which reads XBE VA `46D148` in the supported base disc
or `4745E8` in the supported 1.1 image. The original bytes are not embedded in
the source repository. It reproduces `17CA10`'s
index rounding and interpolation. The instruction oracle passes 11,059 fade
cases, alongside the prior 25,594 crouch/cast cases. This proof requires default
round-to-nearest mode; other rounding modes are rejected by the native kernel.

RTX 5080 runs rendered the actual Cairo camera track and scene placements,
resized to 960x600 and passed Khronos synchronization validation. A separate
run executed the real snap-to-white script: all 576,000 captured pixels were
RGB(255,255,255), including the clear background and bars. These checks establish
GPU/presentation execution, not a complete cutscene or retail visual match.

Evidence: `analysis/systems/{cinematic-ctest.log,cinematic-asan.log,
cinematic-kernels-differential.json,camera-smoke.log,fade-smoke.log,
cairo-camera.png,cairo-fade.ppm,campaign-startup.log}`. Primary research:
[CAMPAIGN_RESEARCH.md](CAMPAIGN_RESEARCH.md). The recoverable native
`game_save_cinematic_skip` / `game_revert` implementation and original compiled
skip-wrapper verification are described in [CAMPAIGN_PROGRESS.md](CAMPAIGN_PROGRESS.md).
Remaining dependencies include resource prediction, relative animations, devices,
ambient audio, BSP transitions, encounters, objectives and mission transitions.

The retail title screen and submenu implementation, resource coverage, controls
and remaining visual/backend limits are described in [FRONTEND.md](FRONTEND.md).
