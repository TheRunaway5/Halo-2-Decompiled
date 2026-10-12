# Port status — 2026-10-07

The project contains a native level viewer, a simulation workbench and a
reverse-engineering pipeline. It does not execute Halo 2's full recovered game
loop and is not a complete playable game port. Physics, AI, combat, scripts
and audio now have working partial implementations; their fidelity boundaries
are recorded in [NATIVE_SYSTEMS.md](NATIVE_SYSTEMS.md).

The physics/sandbox continuation now has 53,018 bit-exact comparisons for
selected native movement, jump, shape, recharge, projectile, shield and body-damage
kernels. It fixes the zero-gravity map interpretation and integrates original
shield/body armor tables and body-impact protection/stun rules. Full physics/sandbox parity remains incomplete; campaign
work follows those prerequisites. Current evidence and remaining acceptance
work: [PHYSICS_PARITY.md](PHYSICS_PARITY.md).

## Implemented and verified

| Area | Result |
| --- | --- |
| Disc | 81 files, 30 map caches, 4,720,758,479 extracted bytes. Each file has a SHA-256 record. Original ISO preserved. |
| Executable | Retail title `4D530064`, entry `0x002D0AEE`, XDK 5849, exact matching executable hash below. |
| Decompilation | Ghidra 12.0.3 with XboxDev XBE loader; 19,135 identified functions, 19,130 pseudocode exports, five failures. Saved project and logs. |
| Native platform | C++20, SDL2 window/input/gamepad integration, Vulkan presentation, depth buffering, resize/recreation, GPU screenshot readback. |
| Content | All 30 cache headers/tag catalogs; 80 BSPs including menu/shared caches; 3,551,172 expanded triangles with authored geometry instances. All parsed without errors. |
| Textures | Runtime diffuse references; BC1/BC2/BC3 GPU uploads; byte/16-bit/BGRA textures converted to RGBA with rectangular Xbox deswizzling. Authored resident mip chains are uploaded and sampled, with retail `359E0` count selection. Lockout resolves 49/54 material slots. |
| Tests | Latest first-person continuation: 14 normal and isolated ASan/UBSan CTest groups pass, with leak detection enabled. Coverage includes frontend bounds/movies, endian/bounds/overflow/malformed-disc/symlink protection, authored mip preservation, bitmap conversion, shader programs/remapping, first-person placement/action fixtures, shader-audit bounds and actual retail content validation. Dated earlier frontend/rendering runs remain retained with their own scopes. Evidence: `analysis/first-person/verification.json`. |
| GPU | NVIDIA GeForce RTX 5080: textured Lockout rendered 120 frames with resize and screenshot at 960x600; Khronos validation with synchronization checks produced no messages. |
| Render smoke | All 80 BSPs rendered three frames each on RTX 5080 through hidden SDL/X11 windows with synchronization validation; 80 passed, zero failures/messages. Screenshots and per-BSP logs retained. |
| Wayland | Textured Lockout rendered 30 frames with resize and synchronization validation on a native SDL/Wayland surface; no validation messages. |

Hashes:

```text
ISO SHA256:         a5b99c1ce2ee650f12caf342118c716cedd4a797fc369b3f0eba82382734f05c
default.xbe SHA256: 03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d
```

The separately researched 27 level maps contain 77 BSPs. The native validator
also includes three BSPs in `mainmenu`, `shared`, and `single_player_shared`,
which accounts for the 80 total. Triangle totals include repeated authored
geometry instances, rather than just unique mesh definitions.

Evidence in this workspace: `assets/disc/disc-manifest.json`, `analysis/xbe.json`,
`analysis/retail/summary.json`, `analysis/retail/functions.tsv`,
`analysis/map-validation.log`, `analysis/vulkan-textured-validation.log`,
`analysis/vulkan-wayland-validation.log`, and `analysis/screenshots/lockout-textured.png`.
The complete render sweep is recorded in `analysis/render-smoke/summary.json`
with one `.log` and `.ppm` screenshot per BSP. This proves resources can be
uploaded/drawn/presented; it does not assert that every screenshot matches the
original game.

The failed Ghidra function entries are `00386dba`, `0038709e`, `0039ae7a`,
`004001f0`, and `00404380`; errors include varnode/calling-convention problems
and a timeout. Successful export only means the decompiler produced text.
Its analysis warnings, guessed types, and nonstandard Xbox LTCG register
conventions can still make the output wrong.

## Native systems continuation

All 30 maps/80 BSPs pass system-content parsing and script graph validation.
The optional `--play` runtime integrates authored static/instance collision,
capsule movement, tag-based player tuning, authored navigation portals, skinned squad models and
first-person weapon actions, provisional perception/combat, a bounded compiled-
expression scheduler and decoded impulse/looping SDL audio. It is intentionally described as a subsystem workbench.

Retail `DD7D0` crouch transitions pass 10,384 bit-exact instruction-oracle
comparisons; nine scalar script casts pass another 15,210 comparisons;
`17CA10` fade curve 5 passes 11,059 comparisons under round-to-nearest.
Normal and ASan/UBSan tests cover the implemented systems and selected
actual Lockout/Cairo data. Cairo renders the skinned marines and first-person weapons in Vulkan with
synchronization validation and mixes a decoded Xbox firing sound through
SDL/PipeWire. Source/detailed evidence: [NATIVE_SYSTEMS.md](NATIVE_SYSTEMS.md),
[SYSTEMS_RESEARCH.md](SYSTEMS_RESEARCH.md), `analysis/systems/`.

The campaign continuation adds pending BSP changes with collision/navigation
and Vulkan resource replacement, cross-BSP native checkpoints, scene lifetime
policies, retained asset prediction and sound-class fades in the SDL mixer.
Objective and flashlight/ambience controls have saved state; their complete
UI/playback consumers remain unfinished. Fourteen of 15 authored startup paths
still fault within 60 ticks, and the longer Delta Towers check also faults.
Current implementation, 26-group CTest/four-group sanitizer evidence and
remaining campaign dependencies: [CAMPAIGN_PROGRESS.md](CAMPAIGN_PROGRESS.md).

## Retail frontend continuation

The standard launcher plays the retail intro before the title screen;
the separate title-first launcher skips boot playback. The native UI loads all 84 retail
screen definitions / 150 panes, 31 variant categories, all 92 parameter dialogs
and 39 built-in variants. Original fonts, menu bitmaps, authored rolled cameras,
model viewports, WMA music, menu sounds and Bink movies run in the native host.
The idle attract movie waits the original 75 seconds after input/title/movie
reset. The city sky uses its authored model, clear color and parallax. The beam
uses its original vertex code, normalization cube, texture transforms,
combiners and forward render states. Profiles and
named variant copies persist in a host preferences file. All menu panes/options
have CPU and GPU fixture coverage; this is not evidence of 1:1 retail visuals or
working network/game services. Details: [FRONTEND.md](FRONTEND.md),
`analysis/frontend/`.

## Remaining engine milestones

### Shader and lighting parity

Full 1:1 shader and lighting parity with original Xbox retail is the target,
and **has not been achieved or visually verified**. The default BSP renderer
now uses authored lightmaps instead of its previous invented directional sun.
`lightmap_tex` and `lightmap_tex_detail` use the original pixel programs, authored
base/detail/bump transforms and decoded directional lightmap streams. Other
passes remain an explicit fallback. Static pass selection retains authored
lightmap/specular selectors; remaining switches, view-dependent externs and
layer/LOD scheduling are pending.
The recovered common vertex expressions use the viewer camera and have not
been differentially compared with the Xbox GPU.

The code separates cache format recovery, original shader operations, and host
GPU ownership:

| Module | Responsibility | Current evidence |
| --- | --- | --- |
| `src/material.cpp` | Decode template LOD/layer/pass ranges, bitmap indices, constant remaps, every pass implementation and original vertex payload; bind supported static passes | All 30 caches / 80 BSPs parse; focused Lockout bindings and 16x detail tiling checked |
| `src/render_program.cpp` | Assemble original pixel programs and apply component-preserving constant remaps | Retail `0001BA00`: 336/336 exact comparisons across 183 distinct implementations; vertex remapping `00017000`: 1,672/1,672; pixel values/filtering `00017170`: 1,288/1,288 |
| `src/lightmap.cpp` | Decode the default lighting group and distinct bitmap/palette bindings per cluster and instance | Lockout binds 65 decoded images; shared instance geometry retains independent lighting |
| `src/texture.cpp`, `src/bitmap_mips.cpp` | Preserve authored mip bytes, rectangular swizzle, BC data, P8 indices and BGRA palettes | Retail `000359E0`: 30,140/30,140 exact comparisons; malformed palette/mip/range checks |
| `src/cache.cpp` | Preserve lightmap UV, packed normal/tangent/binormal and incident-direction streams through instance transforms | All 80 BSP geometries checked |
| `shaders/combiner.glsl` | Decode NV2A RGB/alpha inputs, mappings, simultaneous stage outputs, constants and final combiner | Compiled SPIR-V and real Vulkan execution; original hardware arithmetic parity pending |
| `src/texture_state.cpp`, `include/halo2/sampler.hpp` | Decode original sampler records and preserve per-stage addressing, filtering, anisotropy, bias, mip bounds and border colors | Retail `000189A0`: 4,112/4,112 exact state comparisons; real Cairo anisotropic bump and clamped lightmap fixtures |
| `shaders/texture_program.glsl` | Execute projected, cube, passthrough and admitted dependent/dot reflection operations before combiners | Native GPU lookup-coordinate regression covers the actual Cairo bumped reflection stage sequence |
| `src/render_view.cpp`, `shaders/render_view.glsl` | Bind original pixel viewport constants independently of sampled-image dimensions | Retail `00012FA0`: 4,256/4,256 bit-exact comparisons; shared GPU function covers multiple resolutions, offsets, scale flags and fixed shadow mode |
| `src/renderer.cpp` | Own texture uploads, views, authored samplers, material uniform buffers and descriptors keyed by material plus lightmap | All 80 BSPs render with synchronization validation; resolution capture sweep covers 480p through 4K |

The bump palette is read from the user's retail `default.xbe` at file offset
`0x4646C8` (guest `0x468848`), with a full 1,024-byte fingerprint check. It is not
copied into distributed source or generated from an approximate normal formula.
P8 colors are expanded before GPU filtering; lightmap texture coordinates use
the original signed normalized short stream. Baked light is not multiplied by
an added directional light.

The remaining work follows this order, with a validation gate for each stage:

1. **Pass scheduler and extern state.** Select the original LOD, layer,
   implementation, switch, constant and animated overlay for each world,
   object, first-person, sky and effect draw. Preserve pass order and authored
   render/texture states. Gate: instruction comparisons of selection/remapping
   and per-draw original/native state traces.
2. **Complete vertex and texture programs.** Translate the recorded programs
   for all rigid, rigid-boned, skinned and compressed classifications, including
   cube/volume/dependent texture modes. Gate: original input/output vectors
   across classifications and GPU output comparisons.
3. **Remaining opaque and emissive passes.** Add alpha-test, detail blending,
   environment/specular and self-illumination passes with their original blend,
   depth and cull settings. Gate: representative material fixtures and matched
   surface captures, including cutout edges and animated parameters.
4. **Lighting state.** Recover per-vertex/alternate lightmaps, object probes,
   dynamic lights, shadow generation/application and first-person lighting.
   Gate: matched world/actor/weapon captures in each lighting mode.
5. **Transparency and scene composition.** Recover sorting, water,
   active camouflage, sky, fog, particles and postprocess. Gate: matched camera
   views and overlapping surfaces/effects over time.
6. **Sampling and final output.** Match all sampler states, LOD behavior,
   gamma/brightness and framebuffer precision. Gate: same-scene, same-camera,
   same-settings original/native image comparisons across campaign and
   multiplayer caches. Only this completed gate permits a full parity claim.
   Every future effect pass must use its active target's dimensions for pixel
   conversion and the actual sampled image's dimensions for texel offsets;
   resolution-scaled effects and fixed retail targets need separate checks.

Evidence is in `analysis/rendering-audit/` and `analysis/rendering-parity-smoke/`.
`tools/audit_rendering.py` records 11,495 shader entries across 30 caches (shared
entries counted per cache), all template/pass/vertex payloads and lightmap
bindings, with `retail_visual_parity_verified` explicitly false.
`tools/differential_combiners.py` and `tools/differential_rendering.py` use the
local retail instructions through Unicorn only in the validation harness.
The runtime executes native C++ and Vulkan. Parsed assets, exact CPU kernel
comparisons, passing tests and validated GPU execution each prove their stated
scope; none substitutes for the final matched-frame gate.
The pixel remapping comparison covers constant values, not the retail upload
dirty masks or timing, which remain part of the pass scheduler milestone.

### Engine recovery

1. Recover and validate engine data structures, globals, initialization, memory
   ownership, object/tag handles, and the original simulation update order.
2. Port asset resource management and original shader/pass behavior, texture
   tiling, lightmaps, sky, transparencies, effects, and complete animated render-model behavior.
3. Recover collision/Havok behavior, player movement, units, weapons, projectiles,
   vehicles, damage, inventory, AI, and animation state transitions.
4. Complete the script VM, mission/cinematic triggers, dialogue/audio, saved
   state, menus, and input settings. Complete one authored campaign path first.
5. Recover networking and multiplayer behavior; validate campaign/multiplayer
   parity against the original game before claiming a complete port.

The initial free-flight camera is a viewer tool, not recovered player movement.
No Xbox binary, emulator, or translated x86 game loop runs in the native
application. The separate differential harness executes one original kernel
under Unicorn only for testing. Runtime audio decode/output is verified, but
retail mixing/spatialization and whole-game audio parity are not. The new host
collision/AI/combat/evaluator code does not establish gameplay or mission
parity. Retail shaders, complete animation behavior, live controller hardware and
cross-platform behavior still require verification. Cairo startup currently
faults at `cinematic_skip_start_internal`; full mission playthrough remains unverified.
Model/animation format evidence is in [PLAYABLE_RESEARCH.md](PLAYABLE_RESEARCH.md).
First-person arms/weapon placement now uses a shared animation graph, the
original weapon-offset/camera-control inverse order and the player's authored
horizontal FOV. Its three transform helpers pass 30,048 exact instruction
comparisons; both meshes have fixtures across 407 battle-rifle/SMG action
frames. Remaining motion and matched-frame gates are in
[FIRST_PERSON_PARITY.md](FIRST_PERSON_PARITY.md).

The next scene milestone implements script player-camera gating, camera control,
cinematic state, HUD fade state, fades and letterbox, named scenario placement
creation/destruction with generation handles, hiding/teleporting, unattached
trigger-volume point tests, and object sound positions at event start. Scenario
meshes render in native Vulkan with authored placement variants/transforms.
Cairo's real `x02_01` camera track samples its graph and anchor, uses the original
forward/up axes, lasts 515 ticks and clamps its final frame. This is a camera
preview, not a completed opening cinematic: relative actor/scenery animation,
device motion, checkpoints, BSP transitions and mission execution remain missing.
Research and validation boundaries: [CAMPAIGN_RESEARCH.md](CAMPAIGN_RESEARCH.md)
and [NATIVE_SYSTEMS.md](NATIVE_SYSTEMS.md).
