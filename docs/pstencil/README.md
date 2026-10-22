# pStencil graphics mode

The installed gameplay runtime is **surface-alignment-live-v1**, via
`./Halo2-pStencil`. It retains the live-v14 source/ABI and materials, with the
[shadow-cap transform fix](SURFACE_CAP_ALIGNMENT.md). The actual TU1.5 Lockout
captures pass in Original, pStencil and third person without script/validation
errors. Original is byte-identical to live-v14. Its manifest verifies 65 SPVs,
including the unchanged resident environment compute shader omitted from the
older 64-file manifest; only the volume vertex shader changes.

`./Halo2-pStencil-Campaign` selects **campaign-v10**. It adds the recovered
[texture accumulation](TEXTURE_ACCUMULATION_2003.md) to the
[normal accumulation and world-space light response](NORMAL_ACCUMULATION_2003.md),
[combined Chief visor response](COMBINED_VISOR.md),
[utility/dependent-light passes](LIGHT_RESPONSE_2003.md) and
[projected gels/specular masks](PROJECTED_LIGHT_RESPONSE_2003.md). The normal
target is generated once per view and consumed by the actual lights. Compatible
Elite, Arbiter and other current assets retain their own bump maps/material values.
The completed regular-Elite neutral-detail input survives the texture conversion;
its native blue armor response matches the accepted v8 capture. Compatible
one/four/seven-combiner texture programs preserve current color-change inputs.
The complete native opening passes 20 exact Original roundtrips, 20 material
restores and 14 positive light/shadow pairs with no shadow brightening. Script
and Vulkan validation errors are zero. Four fresh actual F1/mouse switches pass
on v10 in140.73 seconds. Fresh group and visor photos use 5120×3840 internal targets;
Original is byte-identical to v8 (and its prior v5 control). See [opening presentation](OPENING_CAMPAIGN.md)
for evidence and remaining presentation limits.

The latest actual opening recording is
`analysis/pstencil/campaign-live-v10/recording-window-native/opening-presentation.mp4`
(1920×1080,200.80 seconds, actual game audio). Its1128 native frames have zero
script/Vulkan errors and nonzero game-only audio. The explicit recording-only
immediate presentation option leaves default game presentation unchanged.
Measured native interval median131.38ms,p95374.25ms,max686.53ms under current
host load shows remaining hitches. No video interpolation or simulation changes
are used. Final audio synchronization and full original material/scheduling
parity remain open. See [capture evidence](CAPTURE_PRESENTATION.md).

Campaign-v3 updates the actual submitted material roots and their shared-pass
closure. All 20 same-frame images match the previous full-bank upload exactly.
A matched local run lowers median native interval from 44.06 to 31.09 ms;
the actual F1 selector passes. Large stalls remain. See
[recording pacing](CAMPAIGN_PACING.md) for measurements and limits.

The [2003 light-response component](LIGHT_RESPONSE_2003.md) passes
all 560 original constructor rows, 1,023 original RGB packing cases and 4,480
two-stage GPU arithmetic fixtures. Its actual sphere input/lookup crosswalk
passes in Cairo and the opening cache. Campaign-v4 uses an actual RGBA8 utility
target and the full current texture-mapping target under the existing stencil
test. Its fresh high-resolution group and visor images remain pixel-identical
to the sphere integration candidate; Original also remains exact. The general
gameplay launcher retains its separate live-v14 source cohort with the cap fix.

A working hybrid graphics mode is connected to the native game renderer.
**Original** uses the existing rendering path. **pStencil** adds the May 2003
build's capped stencil-volume point-light shadows, separate baked-light shadow darkening, authored direct specular,
and retained environment reflectivity to the current levels, textures and posed
models. Direct sphere lights include moving grenade glow and native function-driven attachments. Original is the default without a saved
preference. Switching requires no level reload.

## Run and switch

From the repository root:

```sh
./Halo2-pStencil
```

This retained, tested runtime starts Lockout with one local player and the
existing HUD/pause assets. Open **Settings → Video Settings → Graphics Mode**
or **Pause → Settings → Video Settings → Graphics Mode**. The first row cycles
**Original** and **pStencil**, applies immediately, and saves the choice in
`menu.cfg`; Reset restores Original. Fullscreen Mode follows it, with Windowed,
Fullscreen, and Borderless Fullscreen choices and the existing preview/rollback.
The Upscaling submenu replaces SSAA with FSR 3.1 / DLSS 4.5 and Native AA / DLAA
through Ultra Performance; see [native upscaling](../UPSCALING.md) for binary
provenance and the remaining vendor validation diagnostics.
The F1 overlay and debug cheats are removed.

```sh
./Halo2-pStencil --graphics-mode pStencil
./Halo2-pStencil --map assets/disc/maps/coagulation.map --local-players 2
./Halo2-pStencil --launcher-info
```

The launch route verifies the tested runtime, TU1.5 gameplay XBE and all 64
shader files against `analysis/pstencil/live-v13/runtime.json`. It preserves the
other launchers and their separately retained runtimes. The new route retains
its executable and shaders in `analysis/pstencil/live-v13/build/`; keep that
folder. It reads current maps directly from `assets/disc/maps/`.

Live-v13 reads completed native attachment functions, colors and transforms.
It also restores the complete original global light tick at `13AB4E -> C3430`;
normal-tick activation and timed explosion retirement pass. All ten focused
checks pass. Fresh game, three-character and visor captures restore exact
Original pixels; isolated stencil controls only darken output. The64 runtime
shaders remain byte-identical to live-v11. See [native attachment evidence](ATTACHED_LIGHTS.md).
Full original presentation parity remains open. Work on the complete opening
campaign cinematic is tracked [separately](OPENING_CAMPAIGN.md); its first
full pStencil recording is a diagnostic with remaining visual defects.

## Rendering and organization

| Directory | Responsibility |
| --- | --- |
| `include/halo2/pstencil/` | Graphics mode, geometry, frame and GPU interfaces |
| `src/pstencil/` | Source-backed stencil system and current-asset adapters |
| `shaders/pstencil/` | Volume extrusion and integer-W depth shaders |
| `tools/pstencil/` | Reference inspection, comparisons, frozen builds and launching |
| `tests/pstencil/` | Original-code, posed-position, GPU, renderer and live-UI checks |
| `cmake/PStencil.cmake` | Separate libraries, shaders and tests |
| `analysis/pstencil/2003/` | Local disassembly, pseudocode, shaders and provenance |
| `analysis/pstencil/live-v13/` | Current tested runtime, native light tick/readback, fresh game/group/visor controls |
| `analysis/pstencil/live-v11/` | Retained material masks and rank/armor/vehicle/weapon controls |
| `analysis/pstencil/live-v10/` | Retained standard Elite neutral-detail delivery |
| `analysis/pstencil/live-v5/` | Retained frozen runtime with the default 2003 resident environment response and fresh armor/game controls |
| `analysis/pstencil/live-v4/` | Retained independent caster/opacity delivery and comparison baseline |
| `analysis/pstencil/live-v3/` | Retained high-detail/darkening delivery and comparison history |
| `analysis/pstencil/live-v2/` | Retained direct specular and plasma-light delivery |
| `analysis/pstencil/live-v1/` | Retained first-delivery source/build and comparison history |

The small shared renderer seam supplies each actual admitted raw point-light
bank and its actual origin. The adapter executes the authored vertex program's
world-position prefix, including decompression and current skin palettes.
It uses the current raw triangle indices without position welding or replacement
meshes. Closed triangle prisms supply caps and stitches; shared internal sides
cancel in stencil. This adapts retail resources, which lack the 2003 build's
precomputed ISQ adjacency, at the cost of submitting more triangles.

The live pStencil path collects an independent indexed caster bank from the
actual native light query before color availability, shader-pass selection or
camera receiver filtering. Current selected model sections, poses, source
vertex classes and literal list/strip rules survive. Position-only resources
load lazily, including owners without a resident color material bank. Known
part types 1/2 use the recovered 2003 casting predicate; type 3 remains a
receiving surface and types 0/4/5 do not cast. Expanded CPU fallback geometry
preserves the raw type/flags; only generated geometry without that provenance
uses the earlier shader-layer fallback. This is an explicit cross-version
adaptation, not proof of complete 2003 ISQ/manifold/object admission parity.

## Default 2003 environment response (live-v5)

The [presentation reference](PRESENTATION_PIPELINE_REFERENCE.md) traces the
actual default layered renderer and extracts its resident six-combiner shader.
The matching current independent tangent-space environment family now uses
those exact control words: quartic Fresnel, separate facing/glancing RGB and
brightness, midrange tint, and environment powers through eight. Completed
current material constants, maps, poses and destination-alpha ownership remain
its input adapter. The campaign-v2 Chief visor now has a separate recovered
response; other combined families retain their current path. This does not
establish the full original accumulation pipeline or asset values.

The 4,096 default-response scalar/GPU cases pass against the resident program
and source formula. A separate older callback program remains reference-only;
4,105 original CPU construction fixtures and 4,096 GPU/math cases pass for it.
All seven focused tests and nine native captures pass. Original's real-match
image and all five armor photos are byte-identical to v4; every photo restores
Original exactly after pStencil. Shadow-isolation controls only darken output.
The new environment response changes 104,053 pixels in the declared group
photo. Launcher verification and a three-frame native launch pass. See
`analysis/pstencil/live-v5/validation.json` and `verification.json`.

All-map metadata declares this pass on 62 unique materials linked to 129 object
definitions, including Spectre, Banshee, Wraith and Covenant weapon/prop
families. These are [candidates](../../analysis/pstencil/environment-response-test/material-candidates.json),
not 129 rendered certifications: the live implementation must also match the
supported pass/control shape and destination-alpha blend. Current values are
retained for later equipment; no faction preset is substituted.

## Current delivery: separate material masks (live-v11)

pStencil now retains an independent authored texture mask for direct specular,
including combined current shader LODs that leave scene alpha zero. This fixes
the Banshee's missing highlights while preserving its authored normal map,
seven-combiner lobe, textures, light inputs and stencil test. Materials without
an admitted independent mask, including Spectre, keep their existing current
accumulation behavior. See [mask evidence and boundaries](MATERIAL_MASK_ADAPTATION.md).

The cleaned standard Elite response is retained across eight native ranks.
Fresh Chief, Arbiter and Elite photos in Cairo use 2560x1920 output with
5120x3840 internal sampling. All eight ranks, five canonical photos, two front
photos and Original gameplay restore/match live-v10 exactly in Original mode.
Isolated highlights only brighten and isolated shadows only darken. Visor
bitmap removal changes the measured image. Gameplay RNG and detonation inputs
remain equal across the Original/pStencil/control captures.

Six native vehicle controls pass: Banshee, Warthog, Ghost, Spectre, Scorpion and
Wraith. Four 1920x1440 native weapon controls pass: Battle Rifle, Plasma Rifle,
Needler and Brute Shot. They exercise current-asset material adaptation and
independent dynamic self-shadowing, not matched 2003 framebuffer parity.
Nine focused checks pass, including 9,216 armor/mask GPU cases and 5,120 Elite
detail GPU cases. Receipts are in `analysis/pstencil/live-v11/validation.json`,
`verification.json`, `variant-verification.json`, `front-verification.json` and
`dynamic-verification.json`.

Full original MRT/utility scheduling, other combined material families,
animated object-function light attachments, ISQ/manifold compatibility and
original object admission remain open. The
current delivery is a working source-backed hybrid; it is not a complete
one-to-one reproduction of the original presentation.

## Retained Elite response delivery (live-v10)

Eight native Elite variants were compared in the same Cairo scene. The shared
standard body shaders use a dark charred detail texture in current assets,
while the five exact 2003 shaders bind a neutral detail map. pStencil now uses
the proven original RGBA(128,128,128,255) response at the isolated current
detail multiply. Current base/bump/change-color images and rank colors remain
resident; Original is unchanged. Ranger and special rank helmet materials keep
their separate authored responses. See [Elite evidence and scope](ELITE_VARIANT_PRESENTATION.md).

Nine focused checks pass, including 5,120 exact CPU/GPU detail cases over ten
shader LOD passes. All eight rank controls restore Original exactly. Canonical
photo controls use identical corrected shader-LOD/object-function staging with
only the neutral adapter bypassed. All four ordinary gameplay frames remain
byte-identical to live-v9. The tested launch route selects this frozen runtime.

![Eight actual Elite variants under the same declared point light](../../analysis/pstencil/live-v10/all-ranks.png)

![Chief, Arbiter and Elite together in Cairo Station](../../analysis/pstencil/live-v10/armor-group-front/closeup.png)

The broader 2003 utility/MRT, combined response and manifold work remains open.
Vehicle diagnostics now also use native shader coverage, owner colors and
independent native point casters. Five vehicle fixtures pass; Banshee's direct
specular alpha is being investigated separately from this installed delivery.

## Retained stronger Arbiter metal delivery (live-v9)

Arbiter's armor and helmet now have four contributions from their authored
sphere specular shader under the same stencil test, with doubled combined
reflection strengths (.8 facing / 1 glancing after clamping). The original
25% treatment was too subtle in the reviewed screenshots. Engraving, surface
masks, normal input and highlight shape remain authored; cloth, skin and
ordinary Elite materials receive no extra polish. See the
[metal adaptation](ARBITER_METAL_ADAPTATION.md).

Eight focused tests pass, including 5,632 armor/combined-consumer GPU cases.
All nine canonical native captures and two additional light-angle captures pass.
The canonical photos use the same camera/light as v7: Original is byte-identical
and restores exactly after switching. All four ordinary gameplay captures also
match v7 exactly. Point-shadow controls only darken output, and the isolated
armor polish only brightens it. The additional front angle changes the declared
point-light position and has its own controls and exact Original restoration.

Current captures retain 2560x1920 output and 5120x3840 internal
stencil/color/depth. These are staged actual Cairo Station assets and native
actors. [Validation](../../analysis/pstencil/live-v9/validation.json),
[canonical controls](../../analysis/pstencil/live-v9/verification.json),
[additional angle](../../analysis/pstencil/live-v9/front-verification.json).

![All three characters together, additional front light angle](../../analysis/pstencil/live-v9/armor-group-front/closeup.png)

![Arbiter metal close-up, additional front light angle](../../analysis/pstencil/live-v9/armor-arbiter-front/closeup.png)

Full 2003 accumulation, utility, combined response and original ISQ/manifold/
object admission remain unfinished. Unmodified original-build framebuffer
capture remains blocked by the local boot configuration; see the
[reference capture status](REFERENCE_CAPTURE_STATUS.md).

## Original values and initial Arbiter metal (live-v7)

The supported independent environment family now uses exact static original
values on literal shader-path matches. The [reference bank](REFERENCE_MATERIAL_VALUES.md)
contains 159 unanimous paths; 12 intersect this current family. Later assets
retain their authored inputs under the same recovered response.

Arbiter armor and helmet have a separate [metal adaptation](ARBITER_METAL_ADAPTATION.md):
a quarter-strength authored specular companion under the same stencil test,
plus a 25% reflection-strength increase on the recognized combined family.
Cloth, flesh and ordinary Elite materials keep their own response. The current
combined Chief visor receives the original .4/.2 facing/glancing strengths;
its full response/tint math remains with the current combined shader.

Eight focused tests pass, including 5,632 new armor/combined-consumer GPU cases
and the existing 8,192 environment cases. All nine native captures pass.
Original matches v6 exactly in the real match and all five photos, and every
photo restores it exactly after switching. The gameplay shadow control changes
11,794 pixels only toward darker output, with identical simulation/RNG inputs.
Removing only Arbiter's extra specular changes 5,891 group pixels / 15,572
Arbiter-view pixels only toward brighter output. Source controls and texture
masks remain intact; this is an explicit later-asset art treatment.

Additional native root-vehicle diagnostics pass on Warthog, Banshee and Spectre.
Their independent caster controls only darken dynamic receivers, specular
controls only brighten them, and Original restores exactly. These selected
body/LOD checks use actual netgame constructors and palettes, with declared
camera/light inputs; child turret owners and original-frame parity are outside
this diagnostic. Spectre uses the source vehicle reference at index 5 via
variant choices 76/77=5. See `asset-final-controls/verification.json`.

The fresh screenshots are staged native Cairo Station captures, with current
actors and a declared shared point light: 2560x1920 output, 5120x3840 internal
stencil/color/depth targets. They are not matched original 2003 frames.
See `analysis/pstencil/live-v7/{verification,validation}.json`, per-view controls,
and `focused-tests.log`. The group and visor crops are below.

![Current characters under one point light](../../analysis/pstencil/live-v7/armor-group/closeup.png)

![Current visor close-up](../../analysis/pstencil/live-v7/armor-visor/closeup.png)

Full 2003 bump/texture/utility accumulation, complete combined response math,
scenario/function lights and original ISQ/manifold/object admission are still
open. [Original capture status](REFERENCE_CAPTURE_STATUS.md) records the local
original-build boot blocker. This delivery does not establish full 1:1 parity.

## Independent caster coverage and opacity (live-v4)

The [caster/face audit](CASTER_FACE_PARITY.md) traces the original predicate,
object and authored manifold gates, separate shadow alpha, and raster stipple.
Partial opacity uses the original 17-pattern bank. All 256 byte choices match
unchanged original code; GPU tests verify the pattern/phase, zero-opacity no-op,
and untouched Original depth/stencil. The current light-query opacity is an
adaptation: it does not yet reproduce 2003's separate shadow-LOD fade producer.

Validation under `analysis/pstencil/live-v4/`:

- Six focused tests pass, including real Zanzibar/Coagulation caster/receiver
  distinctions and 43k posed vertex records across current character, vehicle,
  weapon and prop families. Known casters no longer require shader layer 10.
- Seventy-two GPU cases pass, with source camera cap alignment, analytic light
  occlusion, the original RGB 152 darkening, fractional stipple, resizing and
  exact Original restoration.
- The actual Lockout capture retains a caster without a color draw. Original's
  image is byte-identical to v3. Omitting only volume draws exposes 27,291 darker
  pStencil channels / 11,794 pixels, with no added-light channels. Simulation,
  detonation and RNG controls agree.
- Five high-resolution staged armor/visor captures pass live Original round
  trips and isolated point-shadow controls. The group changes 1,029,327 channels
  only toward darker output. All three armors have measured direct specular;
  the visor reflection control changes 48,230 pixels. These are actual Cairo
  Station assets under declared photo lighting, with character-only casters,
  2560x1920 output and 5120x3840 intermediate stencil/color/depth targets.

See `verification.json`, `focused-tests.log`, `gpu.log`, `capture-status.json`,
`caster-original-final.json` and the per-capture `scene.txt`/`fixture.txt` files.
Full 2003 bump/texture accumulation, response/environment math, scenario/function
lights, authored manifold compatibility, separate shadow fade and all spatial/
two-sided admission remain unfinished. Preserve the matching current material
inputs for later equipment; do not substitute numeric faction/material presets.

The material adapter completes each current shader's actual layer 13 sphere
specular pass, including its selector, brightness, color/mask remaps, texture
bindings and destination-alpha blend. It runs under that light's stencil test
following diffuse. Resident specular programs are shared across sphere light
identities and matching model sections; only draw uniforms vary. Existing
authored environment passes remain active, including the visor's combined
light/environment pass and other materials' independent environment stages.
No blanket gloss, replacement Phong exponent or speculative material values
are introduced. The adapter applies by authored shader/pass, rather than a list
of character or vehicle filenames.

Direct untimed constant `ligh` attachments have a separate pStencil-only pool.
They use their authored RGB/radius, current WORLD marker/root, source sphere
update, spatial registration, view selection and receiver collection. A moving
light retains its full generation; missing/deleted owners retire it. Grenade
presentation snapshots also supply the current real projectile lifetime/frame,
including replicas. Attached source bodies use their completed WORLD node frame.
The pool supplements the existing `effe` light producer and never adds those
extra direct lights to Original rendering. Plasma glow uses the current asset's
blue/cyan RGB (0.254902,0.980392,0.960784), radius 0.8; its explosion light has
its separate authored RGB and timed fade. No light is inferred from a filename.

Each view groups receivers by light domain, actual light identity and position. A private
D24S8 attachment receives a copy of Original's depth; its stencil is cleared to
`0x80` for each light. Capped Carmack counting uses depth LESS, opposite wrapping
z-fail operations, depth clamping, no color writes and no depth writes.
The existing authored light/material passes then test stencil EQUAL `0x80`
without writing it. The Original framebuffer resumes before later effects.
Original's depth/stencil never serves as the volume counter. Renderer resize
and replacement retire the separate pStencil resources.

## Reference and boundaries

The reference is the user's read-only May 3, 2003 build:
`/mnt/SSD4TB/Leaks/Halo 2/2003/game/halo2debug.xbe`, with its matching
`tools/bin/halo2debug.map`. XBE SHA-256:

```text
5a9edb4c874a782594c86020db49b0086f44ae38f53ff2276b3192a2c2383d18
```

Linker section offsets resolve through individual XBE sections. Inspection
retains 227 renderer functions and ten literal shadow vertex programs `89..92`
from XBE `004BEB20`. The native adaptation follows these identified boundaries:

| Original XBE | Retained behavior |
| --- | --- |
| `001773D0` | Doubled near/far point indices and original stitch ordering |
| `001CE3A0` | Ordinary shadow-state branch, LESS depth, stencil clear `80` |
| `00093BC0`, `00093EB0` | Capped Carmack wrapping z-fail counting |
| `000F7F20` | Per-light shadow/utility/lighting ordering and extrusion `1024.0` |
| `000E88D0`, `000E8CA0` | Receiver stencil EQUAL `80`, KEEP operations, zero write mask |
| Programs `89`, `8F` | Position minus normalized light direction times extrusion distance |

This is a working hybrid adaptation of the shadow and material stages, not a
complete 2003 renderer port or Xbox framebuffer-parity claim. Current baked
lighting and authored light/material inputs remain in use. Active native sphere
lights and admitted constant direct attachments cast stencil shadows; without
such lights, the extra direct-light stages make no contribution. No 2003 level,
mesh or texture replaces current content. The 2003 bump/texture accumulation
pipeline and exact historical response curves remain separate work. Direct
attachments requiring named source functions, animated/HSV light definitions or
frustum shapes retain explicit admission gaps rather than invented values.
Caster coverage still depends on native BSP/light and current model admission;
off-camera or otherwise unavailable owners are not claimed. Native campaign
collection and first-person light receivers retain their own existing coverage
limits. Depth clamp and D24S8 are required.

[The visual-identity audit](VISUAL_IDENTITY_AUDIT.md) records additional 2003
stages and material-by-material adaptation of later vehicles, weapons and props,
with primary-source links and unresolved historical identities.

## Retained higher-detail/darkening delivery (live-v3)

This prior delivery used `analysis/pstencil/live-v3/build/halo2_native`.
The retained source manifest, runtime/shader hashes and validation manifest live
beside it. Older verification sections below describe the historical v2 evidence.

The 2003 lightmap-shadow stage was missing from the initial hybrid. It builds
one combined model mask, then multiplies baked color by **152/255**, once, before
environment/emission and all dynamic lights. Alpha stays unchanged; overlapping
casters do not compound the multiplier. The default byte comes from execution
of the original `000E81A0` and `__ftol2` under the original 53-bit precision,
including all 14 render-state writes. See [the source reconstruction](SHADOW_LIGHTING_REFERENCE.md).
The point-light path separately rejects blocked diffuse/specular contributions.
It does not apply the baked multiplier for every grenade.

Current visible player and vehicle submissions supply their actual interpolated
`ObjectLightmapSample.incident`, independently of retail's limited primary or
projected-shadow direction. The adapter uses the original far-origin construction
and 2-unit extrusion. Current authored layer-10 availability admits caster
triangles. This is an adaptation: it does **not** reproduce 2003's model-manifold
flag, shadow-only ISQ sections, all face/type filters or off-camera admission.
Exact caster coverage remains unfinished. Current corpse, campaign and first-person
coverage must not be inferred from the visible player/vehicle checks.

pStencil temporarily requests native authored LOD5 for current players during
main model selection. The quality flag is restored before leaving that model
view; other flag changes are preserved. An actual 45-tick third-person game run
checked all 45 player draws at LOD5 and checked flag restoration after every
view. Original uses its existing quality selection. The existing antialiasing
setting controls the game target resolution; the staged photos explicitly use
2x SSAA per axis, rather than claiming that every game session does so.

Raw point-light c8..11 and raster regions now match the physical SSAA target,
including the unscaled half-pixel bias. Stencil caps use the same actual c0..3,
physical viewport and 1/16-pixel quantization as the armor. Using a separate GLM
projection had caused striped false self-shadows. The specular vertex upload
upper bound also includes all extra pStencil ranges before buffer allocation.

Final v3 evidence:

- Five focused tests pass, including nine real-map/view/SSAA layouts and the live
  Original/pStencil selector. `analysis/pstencil/live-v3/final-cap-focused-tests.log`.
- **72 Vulkan cases** pass with W/Z depth, ordinary/source viewports, analytic
  floor occlusion, lit near-cap checks, baked RGB 152 multiplication, overlapping
  masks, alpha preservation, Original D24S8 preservation and exact restoration
  after baked shadows. `analysis/pstencil/live-v3/gpu-final.log`.
- The old cap projection fails the explicit lit near-cap check;
  `gpu-native-camera-red-v2.log`. Omitting the baked pass fails the exact
  multiplier check; `gpu-baked-red.log`. These are intentional negative controls.
- Actual Lockout first-person Original pixels match the previous delivered v2
  exactly. The new pStencil/control runs have identical simulation/detonation/RNG
  inputs; omitting only volume draws reveals 38,648 darkened channels across
  16,514 pixels, with no brightened channels. `gameplay-verification.json`.
- `analysis/pstencil/armor-scene-capture/cairo-delivery-{group,chief,arbiter,elite,visor}-v1/`
  contains actual staged Cairo Station captures: current Xbox level and models,
  native constructors/current palettes, authored LOD5, **5120x3840** internal
  stencil/color/depth and **2560x1920** output. Individual controls isolate baked
  darkening, direct specular and the visor cube. The cube control changes 48,230
  pixels only within the visor's reported bounds. `delivery-measurements.json`.
- `cairo-roundtrip-group-v1` additionally captures exact Original restoration
  after the real high-resolution three-character shadow scene.

These photos use declared positions, one steady authored-program point light,
neutral character fill, and character-only volume casters; BSP still receives
shadows. Resident PRT owners without a native cached incident use the current
BSP query module at their actual owner sphere. These choices are documented
photo staging, not ordinary campaign behavior or a matched 2003 frame. PNGs
and crops retain the rendered colors; comparison captions sit outside the crops.

Full 2003 bump/texture accumulation, exact response/environment math, placed
scenario lights and complete admission/face parity are still separate unfinished
work. The original reference and current game assets are unchanged.

## Verification

All checks below ran on the native NVIDIA RTX 5080 Vulkan device.

Current v2 controlled native-owner checks use actual TU1.5 biped construction,
selected source model sections and completed live palettes. Camera/light fixture
inputs are declared; these isolated model captures are functional controls, not
matched 2003 frames. Independent controls remove raw caster ranges, then remove
only base-model casting eligibility while retaining visible geometry. This
proves shadows from the separate dynamic-caster path on dynamic receivers.

| Owner | Positive specular channels | Independent dynamic-shadow channels | Additional controls |
| --- | ---: | ---: | --- |
| MJOLNIR | 2,773 | 12,679 | Body2,521; visor252; muting only the authored visor environment cube changes554 channels |
| Elite | 4,627 | 20,696 | Torso1,022; legs884; arms1,734; head166; helmet815; teeth6 |
| Arbiter | 4,366 | 18,677 | Helmet1,048; armor3,078; authored flesh/cloth/teeth remain separate |

All those model checks restore exact Original pixels. Final captures and logs
are retained under `analysis/pstencil/live-v2/{chief,elite,arbiter}-final/`. The constant plasma-light fixture checks exact
authored RGB/radius,69 moving frames, spatial registration, stable identity,
retirement and object-slot reuse. Live gameplay captures and their declared
initial inventory are retained beside the current runtime.

Current v2 live-game checks use actual OG Xbox English TU1.5 gameplay and
current Lockout,1280×720,tick45, identical inputs/spawn/cadence/RNG in both modes.
The test explicitly supplies two plasma grenades as initial inventory; normal
gameplay then owns selection, throwing, motion, effects, lighting and rendering.
The observed projectile is type1/definition`E7B2062D`; its direct light is
`E7B50630`, with the authored blue/cyan RGB. Both modes end with identical
primary/auxiliary RNG state. pStencil contributes49,041 pixels/136,057 channels.
The frame submits12 raw light ranges,12 authored specular draws,10,438 current
dynamic caster triangles and130,824 volume vertices. See
`analysis/pstencil/live-v2/screenshots/Original-vs-pStencil-plasma.png`.

A separate capture-only Vulkan interceptor omits only capped wrapping z-fail
volume draws. The same light, receivers, specular, game state and RNG remain.
Compared with that control, real pStencil shadows reduce120,315 channels across
44,494 pixels, increase none, maximum reduction222. The control records24 omitted
volume draws/3,137,352 vertices over the match. This establishes visible in-game
plasma-light stencil shadows independently of the added blue illumination.
See `screenshots/plasma-stencil-shadow-proof.png`, the two comparison JSONs,
`tests/pstencil/volume_control.cpp` and `plasma-volume-control.json`. This library
is never linked or preloaded by the normal launcher.

The final seven-test settings/UI/renderer/original-code suite and36 GPU cases
pass. The delivered v2 launcher verifies the runtime and62 shader hashes and
renders its existing HUD/pause assets under Vulkan validation.

The following retained v1 checks establish the original backend and UI baseline;
the final v2 test log reruns changed renderer/UI paths:


- **Real Lockout gameplay**, authenticated OG Xbox English TU1.5 gameplay,
  authored spawn, fixed input/cadence/RNG, tick61 explosion: 18 raw receiver
  ranges, one light, 7,176 volume vertices. Compared with Original, pStencil
  reduced 85,152 color channels, increased none, maximum reduction105.
  See `analysis/pstencil/live-v1/gameplay/comparison61-front.png` and the paired
  logs/PPMs. The earlier away-facing comparison correctly produced no change.
- **36 GPU cases**, both integer-W and Z depth; outside, inside-volume and
  near-plane cameras; one/two lights; clipped views and odd dimensions:
  shadows rendered, Original D24S8 stayed byte-identical, and returning to
  Original restored exact color/depth/stencil. No validation diagnostics.
- **Six real-map renderer layouts**, actual Cache class0/1 geometry and authored
  light passes, single/four views and resizing: Original→pStencil→Original
  restored exact pixels, additive passes and view isolation passed. These
  selected planar parts are round-trip/isolation controls, not positive-shadow
  evidence; the real match and volume cases provide that evidence.
- **Live F1 UI mouse input** selected both modes, updated shared session state,
  survived active pStencil resizing, and restored exact unlit-scene pixels.
  Reopening pause settings adopts the current F1 selection; the pause suite passed.
- **Video Settings** live adapter, persistence/reload, invalid preference and
  reset/default checks passed. The existing optional F1/CRT GPU suite passed.
- **Posed geometry**: 14 retained original samples across four captured vertex
  classes, 42 exact world-position words, 14 local-position negative controls.
- **Unchanged May 2003 i386 execution**: 1,280 stitch comparisons, 16 ordered
  state writes, both light schedules and original extrusion argument passed.
- **Delivered launch route**: hash-verified executable/shaders, actual game
  startup with existing HUD/pause assets, three rendered frames and screenshot
  passed under Vulkan validation.

These checks establish native functionality and measured behavior; they do not
establish a complete campaign playthrough, every map/light, other GPU vendors,
or complete 2003 rendering parity.

## Reproduce focused checks

```sh
.tools/python/bin/python tools/pstencil/verify_original.py \
  --build '/mnt/SSD4TB/Leaks/Halo 2/2003' \
  --native build-pstencil-2003/libpstencil_original_bridge.so \
  --output analysis/pstencil/original-shadow-new.json
cmake -S tests/pstencil -B build-pstencil-gpu
cmake --build build-pstencil-gpu -j2
# Use a fresh output directory; validation layer must be discoverable.
build-pstencil-gpu/pstencil_gpu analysis/pstencil/gpu-new
ctest --test-dir analysis/pstencil/live-v2/build \
  -R 'pstencil_|retail_frontend_vista_settings|optional_debug_panel_gpu' \
  --output-on-failure
```

The retained build logs include the validation-layer environment used locally.
`tools/pstencil/prepare_runtime.py` freezes sources and reuses compatible local
objects without changing the seed build. `wire_renderer.py` applies only unique
shared-renderer anchors, rejecting incompatible or partial integration. The
final source manifest identifies the exact retained build inputs.
`wire_surfaces.py` and `wire_point_lights.py` retain the small shared renderer and
match-loop seams. Both retained source directories must remain available while
their shared-resource symlinks are in use.


The campaign delivery is tracked in [opening acceptance](OPENING_CAMPAIGN.md).
Its current-engine source cohort includes the native projected-light and
scenario-light bridge; the dedicated campaign launcher pins the tested
campaign-v2 visor overrides. The general gameplay launcher retains its live-v14 source cohort with the cap fix.
Complete original presentation and a finished recording remain active work.
