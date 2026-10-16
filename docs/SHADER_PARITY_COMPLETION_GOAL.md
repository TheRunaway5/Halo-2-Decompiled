# Shader and effect completion goal

Authority: unchanged OG Xbox TU1.5 executable, original Xbox cache assets and
authenticated original captures. Vista is not a gameplay visual reference.
Original emulator evidence and physical Xbox evidence must remain identified
separately. No guessed values, synthetic replacement effects, relaxed pixel
thresholds or invisible reference draws may close a gate.

The active goal covers the full shader, texture and lighting scope requested on
2026-10-13, including unfinished world-impact/grenade work. This matrix tracks
that scope. A passing material admission or isolated shader fixture is not a
completed live rendering path.

## Ordered completion checklist — updated 2026-10-15

One item is active at a time. Follow the user's order below; preserve later
items' existing work and evidence without starting another implementation pass.
An item is checked only after its actual live path and matched original TU1.5
rendering pass, including the affected resolution and split-screen cases.
CPU fixtures, resource admission and native on/off images are intermediate
evidence and cannot check a full item. Share genuine captures as visible fixes
are verified. A blocker must identify the missing original owner/input and the
attempted reproduction; it does not silently narrow the requirement.

- [ ] **1. Dynamic shadows — ACTIVE.** Complete original live caster/receiver
  selection, priority, history, fading and Generate/Filter/Apply chronology;
  verify matched TU1.5 gameplay shadows and affected view layouts.
  Verified checkpoint: the guarded v33 live640×480 captures retain exact v30
  RGB, poses and all existing main BSP inputs, with no Vulkan validation
  messages. Shadow-on submits6 casters/19 Apply ranges and darkens2,510 pixels.
  Each capture records128 actual BSP commands covering all89 selected parts
  (87 resident and2 dynamic),119 completed material/vertex uploads and39
  actually drawn logical material owners. All17 used pipelines'61-word
  specialization,112-byte camera pushes, geometry/descriptor bindings, cached
  mip metadata and actual shadow target transitions pass independent checks
  and33 mutation controls. The raw source camera-up matches the current caller.
  GPU texture pixels and positive fog/emissive coverage remain unverified.
  Both genuine native light births, the live
  original light view update, main commands and both per-light commands pass full
  unchanged original i386 memory/ABI/FP/path checks. Camera cluster and four
  source-proven fog caller inputs are repaired. Eight focused regressions and
  the fresh native attachment/phase test with59 rejection controls pass.
  The private full89 native depth route now matches unchanged original shader
  emission exactly in color, D24 and accumulation after restoring the authored
  specialization values omitted by that private consumer. It remains private;
  the private V2 guards retain exact color/depth/accumulation for both expanded
  and resident geometry, with49 negative admission controls. Six literal
  near/far limits and two actual shader straddlers also match exactly with
  positive admitted samples. A separate enlarged-window diagnostic exposes
  differences outside the admitted640×480 surface; broader view/layout support
  remains open. A private mixed source/ordinary/shared-slot/source fixture now
  has798 exact route records and all six buffers exact, with zero validation
  errors/warnings and positive samples for both ordinary draw paths. The private
  World receipt passes18 lifecycle controls with2 actual FP models. A fresh
  coherent v33 checked-use prototype passes22 controls and9 scope rejections;
  the earlier private Renderer passes7 actual consumer rejections with clean
  normal teardown and zero validation messages. The genuine completed-main
  World-to-Renderer submission now passes119 material banks and6 ownership
  rejections with normal teardown and zero validation messages; actual loaded
  validation-library maps are retained. Root checked all264 inputs and20 proof
  files and all3 full output buffers remain exact against both coherent v33
  ordinary-off branches. This uses89 retained caller-selected parts; independent
  selection in that earlier fixture was41 and actual full main-query geometry
  is still unproven. The
  primary FP caller leaves legacy guarded fields unchanged, so no old-camera
  rejection or visual bug fix is claimed. Current-root joins and original
  gameplay frames remain required.
  The later v4 fixture fixes a base-version address/indirection error: actual
  TU1.5 `505E7C` points to the already initialized source portal mask. Independent
  selection now matches all89 ordered parts and220 material/stipple bindings;
  root rechecked all281 inputs. No host mask fallback or extra initialization
  is added. A fresh original TU1.5 capture verifies60 rendering events and the
  SDK swap counter, but its64-byte pending FIFO tail/final-output join is open
  and that scene contains no positive shadow submission. The original gameplay
  gate remains unchecked.
  The computed-selection v4 GPU run now submits all89 independently selected
  parts,119 completed material banks and28,692 vertices. All six ownership
  controls pass with actual loaded validation proof and zero errors/warnings.
  Fresh review checked303 input and23 evidence pins plus six complete
  color/D24/accumulation comparisons: zero differences. The nine completed-main
  ownership/core-wrapper API files are now promoted into the checkout after
  five genuine wrapper controls and both existing memory/continuation
  regressions pass. The current Renderer/producer/layout rebase and actual
  runtime consumer activation remain open; no installed runtime is replaced
  by these source changes.
  A separate private GPU readback now verifies542 constructor image owners
  (476 material stages and66 lightmaps),1,641 mip/layer blocks and43,986,448
  bytes with zero differences. An actual final-mip upload corruption is caught.
  This covers immutable uploads/fallbacks; live-v33 GPU contents, draw-time
  target substitutions, decoded sampling and animated images remain unverified.
  Remaining: finish authentic native light-query/cache and resource ownership;
  preserve ordinary receiver depth together with all submitted environment,
  emissive and planar-fog children; complete enable/priority/history/fade policy and other view/resolution
  owners; pass matched original gameplay frames. Separate original-emitter Vulkan
  proofs do not close the failed OpenGL comparison or original-frame gate.
  Detailed work: [TU1.5 dynamic shadows](TU15_DYNAMIC_SHADOWS_20261015.md).
- [ ] **2. Shields and active camouflage — QUEUED.** Complete clone/override
  ownership, shader functions, distortion scene copies and pass order; verify
  matched shield-hit, recharge, overshield and camouflage transitions.
- [ ] **3. Campaign and first-person explosion lighting — QUEUED.** Complete
  native campaign effect/light production and remaining receiver/base joins;
  verify actual detonation frames with matching poses and source state.
- [ ] **4. Scarab turret PRT — QUEUED.** Connect completed source cache incident
  and coefficient banks for valid/empty/invalid inputs, all affected authored
  sections/LODs, and matched rendered responses.
- [ ] **5. Vehicle/character shader functions — QUEUED.** Complete damage,
  camouflage, engine, weapon, moving-body/root/velocity/freeze, permutation
  and pose inputs; close strict Scorpion/Wraith comparisons and affected families.
- [ ] **6. Weapon effects — QUEUED.** Complete authored contrail/beam/casing/blade
  attachments, birth/sampling/detachment/retirement, lighting and blend/depth
  submission; verify each relevant weapon/material/state against TU1.5.
- [ ] **7. Lens flares — QUEUED.** Complete request-pool/frame lifecycle,
  completed occlusion results, visible geometry, graphics state and GPU
  consumption; verify matched visible and occluded original frames.
- [ ] **8. Scene composition — QUEUED.** Complete transparency/occlusion ordering,
  patchy-fog branches, conditional clears and original render-pass scheduling;
  verify matched composed frames.

After the eight items, the full goal still requires the existing world, ordinary
character/vehicle, water, bloom, decal/impact and weather paths to pass their
remaining original-frame gates across the requested map/entity scope. Complete
resolution/resize/split-screen coverage and the freshly built combined runtime
and installed launcher checks before marking the goal complete.

| Work area | Current implementation | Required completion evidence | State |
| --- | --- | --- | --- |
| Explosion illumination | Live BSP/model command and range owner; owned main-view geometry/palette handoff and ordinary vehicle/BLOC join; original command/opaque/palette checks pass; actual original-effect grenade replay visibly lights vehicles and Lockout BLOC power modules; paired controls isolate2,257 vehicle pixels /6,892 BLOC pixels;1080p and four vehicle views verified | Mesa GL/Lavapipe rifle/module illumination now passes exact pixels with source-derived constant-W handling; resolve remaining NVIDIA strict pixel failures and wire campaign event/light collection and finish actor/held-weapon/PRT families with matching base geometry | Queued (item 3) |
| Scarab turret PRT | Per-owner layer5 banks and all six source LODs; upper/rear turret GPU ownership checks pass in three campaign maps; TU1.5 valid/empty/invalid PRT coefficient tests pass | Connect actual retained incident for empty/invalid-bank live owners; finish all affected cache/LOD admission and matched rendered response | Queued (item 4) |
| Dynamic shadows | Vulkan Generate/Filter/Apply stages and bounded draw plans | Live original caster/receiver collection, priority/history/fade inputs and matched gameplay shadows | In progress (item 1) |
| Shields and camouflage | Base and selected overlay/binder paths exist | Live override/clone ownership, original shader functions and scene-copy/pass chronology, matched visible transitions | Queued (item 2) |
| Vehicle/character shader functions | Ordinary lighting and bounded damage feedback exist | Damage/camouflage/engine/weapon inputs; moving-body root/velocity/freeze ownership; original permutations, poses and held weapons | Queued (item 5) |
| Weather | Live Lockout snow, Warlock/Backwash rain and Lockout/Warlock background-layer GPU consumers; source constructors, per-view simulation and clipped cluster admission; bounded native on/off/repeat and local/split/LAN checks | Original platform elapsed/RNG and async-cache chronology, complete current depth/stencil/sky/clear ownership and matched original frames; combined delivery | Open final coverage gate |
| Weapon effects | Selected authored particles and impacts run; original lens request paths have a bounded 72-case CPU checkpoint | Remaining trails, beams, casings, blade/lens attachments, lighting inputs and original lifecycle/blend/depth submission; visible lens geometry, occlusion results and GPU consumption | Queued (item 6) |
| Lens flares | Original request continuation and bounded 72-case CPU comparison | Request-pool/frame scheduling, completed occlusion results, visible geometry and GPU consumption, matched original frames | Queued (item 7) |
| Scene composition | Admitted map lighting, environment, water, transparency, fog and bloom paths; Cairo authored `stencil_env` environment admission repaired | Remaining transparent/occlusion sorting, patchy-fog branches, conditional clears and original render-pass scheduling | Queued (item 8) |
| Existing shader pixel differences | Retained first-person, vehicle and grenade comparisons | Normal filtering differs across GL/Vulkan host backends with identical mip bytes and fixed gradients; source-derived constant-W specialization closes both matched Mesa GL/Lavapipe model illumination draws exactly, while the NVIDIA module gate still fails2,616 pixels; resolve with original evidence, plus remaining depth/arithmetic/compositing differences; preserve exact acceptance gates | Open acceptance gate |
| Coverage and delivery | Bounded campaign/map/vehicle/resolution captures exist | Matched frames for every admitted authored family and requested map/entity scope; resolutions, resize and split screen; freshly built combined runtime and launcher checks | Open final gate |

Work order: the ordered checklist above supersedes the earlier receiver-first
sequence. Dynamic shadows are the sole active item; the remaining seven items
and final coverage/delivery gates stay queued.
Each source fix must retain its failing reproduction, original evidence,
focused validation and resulting screenshot when it changes visible output.

The October15 live unit checkpoint now covers an actual nonlocal Spartan body
and held SMG during grenade/shield response. Its whole-owner overlay rejection
is repaired without admitting unknown overrides. Four pinned native runs retain
identical source/geometry/ordinary-light inputs; independent removal changes
5,399 body pixels and9 weapon pixels, with no Vulkan validation errors.
`TU15_MODEL_LIGHT_RECEIVERS_20261013.md` records the exact scope. Campaign,
first-person, PRT and matched original body frames remain open.

The storage-retention audit also records230 older Wraith snapshot paths
that are not exactly recoverable from the inspected current files, JSONs and
completed archive indexes. The follow-up exact path/size join matches all230
to the separate weapon cleanup's explicit retired-test list at2026-10-16
00:03:32 UTC; the current shader cleanup did not touch those paths.
Expected checksums and `retirement-reconciliation-v3.json` are retained under
`analysis/shader-parity-20261013/shadow-cleanup-partial-archive-audit-v1/`.
Missing historical banks cannot serve as present reproducible proof. Re-run
affected cases or recover exact expected bytes before relying on them for a
later weapon/vehicle completion gate.

The first implementation checkpoint is `TU15_SKINNED_LIGHT_VERTEX_20261013.md`:
819 authored Lockout/Cairo sections, original compression/declaration banks and
6,353,172 exact original-emitter/native GPU output words. This closes the bounded
raw input/vertex consumer gap, not live model illumination or the full goal.

The Cairo environment checkpoint is `TU15_CAMPAIGN_ENVIRONMENT_20261013.md`:
thirteen authored materials retain their original environment program across
twenty-six LODs. Native720p/1080p captures are available; matched original
campaign-frame validation remains open.

Current detailed evidence is maintained in `TU15_MODEL_LIGHT_RECEIVERS_20261013.md`,
`TU15_GRENADE_LIGHT_FRAMES_20261013.md`, `VEHICLE_SHADING_TU15_20261013.md`,
`CAMPAIGN_CHARACTER_SHADERS_20261013.md`, `WEATHER_RENDERING_PARITY.md`,
`WEAPON_EFFECTS_20261013.md`, and `MULTIPLAYER_SHADING_PARITY.md`.
